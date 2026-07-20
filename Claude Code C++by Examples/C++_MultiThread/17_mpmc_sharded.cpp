// =============================================================
// 17_mpmc_sharded.cpp  --  MPMC 用「分片」(sharding) 處理爭用
// =============================================================
//
// 本課目標:
//   1. 看到 lesson 08 的「單一共享佇列」在高任務率下會被
//      mutex 爭用拖累 —— scaling 不會線性。
//   2. 學會用 *分片 (sharding)* 把一條佇列拆成 N 條:每個
//      producer 只往「自己分到」那條 push;每個 consumer
//      用 round-robin 嘗試各條。
//   3. 看到分片把 throughput 直接拉起來 —— 工業界 (TBB、
//      Folly、各 job system) 真正在用的招式之一。
//
//   注意:這 *不是* 一個真正的 lock-free MPMC queue。
//   寫一個正確的 lock-free MPMC queue 是一整章的事
//   (Michael & Scott / Vyukov / moodycamel)。Sharding 是
//   95% 場景下足夠好、又只多十幾行程式碼的「夠好就好」做法。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 17_mpmc_sharded.cpp -o 17_mpmc_sharded
//
// 執行方式:
//     ./17_mpmc_sharded
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     用 sharding 把 MPMC queue 的 mutex 爭用拆開
// 前置課程: lesson 03, 05, 13
// 觀念詞彙:
//   - MPMC              ── multi-producer, multi-consumer
//   - sharding          ── 把一個共享資源拆成多份,降低爭用
//   - try_lock          ── 非阻塞嘗試;搶不到就跳下一個 shard
//   - work stealing     ── 進階版:閒的 worker 偷別人的工作
// 新介紹 API:
//   std::unique_lock(m, std::try_to_lock)   嘗試上鎖,不成功就 owns_lock=false
//   thread_local size_t                     每 thread 自己的「上次成功的 shard」
//   alignas(64) struct Shard { mutex; queue }  每 shard 獨立 cache line
// 何時使用:
//   - 多 producer + 多 consumer 的工作佇列
//   - 不需要嚴格 FIFO,各 task 之間獨立
// 何時不要用:
//   - 工作之間有順序依賴 → 改用單佇列 + 較粗工作粒度
//   - 高度抗 latency 的即時系統 → 改用 lesson 27 Disruptor
// 常見錯誤:
//   - shard 數選太少 → 仍然爭用
//   - shard 數選太多 → consumer 試太多次 try_lock,latency 上升
//   - 期待 FIFO → sharding 不保證
//   - 共享 atomic counter 在 producer 端 → 反而成為新的瓶頸
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── 為什麼 sharding 是中等難度的工業解
// =============================================================
//
// 1. 單一 mutex MPMC 的瓶頸
//    最簡單的 MPMC:`std::queue<T> q;  std::mutex mu;`
//    push/pop 都 lock(mu)。問題:
//      - 4 producers + 4 consumers 同時搶同一把 mutex → 8 條 thread 在
//        同一條 cache line 上 ping-pong + futex queue。
//      - 任何時刻只有 1 條真在做事,其餘 7 條在等 → 7/8 浪費。
//    在 16-32 核以上機器,這個結構吞吐 plateau 在 ~10-20 M ops/s,
//    無論機器多大都救不了。
//
// 2. Sharding 的核心思想:把鎖分散
//    把單一 queue 切成 N 個 sub-queue,每個自己一把 mutex:
//        struct Shard { mutex mu; queue<T> q; };
//        vector<Shard> shards(N);
//    push 時挑一個 shard:
//      A. round-robin (atomic counter % N) ── 簡單,但 counter 自己變熱點。
//      B. random (thread_local rng % N)    ── 沒共享熱點;balance 好。
//      C. hash(key) % N                    ── 同 key 落同 shard,有 locality。
//    consumer 同樣選一個 shard pop;空了 try 下一個。
//    爭用機率 ≈ 1/N (兩條 thread 隨機落到同一個 shard 的機率)。
//
// 3. shard 數怎麼選
//    太少 → 仍然爭用,sharding 沒效。
//    太多 → consumer 為了找一個非空 shard 要 try 多次 (linear scan or
//          random pick),latency 上升;也浪費記憶體。
//    經驗法則:N = 2~4 × CPU 核數。lesson 17 的 demo 用 8,適用 8 核。
//    再要更高吞吐 → 換 Vyukov MPMC、moodycamel ConcurrentQueue。
//
// 4. 全域 size() 變難
//    sharded queue 沒辦法 O(1) 知道全 queue 的 size ── 要 sum 所有 shard。
//    若你需要精確 size 做背壓 → 加一個全域 atomic<size_t> total_size,
//    push/pop 時 fetch_add/sub。但這個 atomic 又變熱點。
//    妥協:approximate size + per-shard hint;真的需要精確再 sum。
//
// 5. 失序 (FIFO 失效)
//    sharded queue *沒有全域 FIFO 保證* ── 兩個 producer A、B 把訊息丟到
//    不同 shard,後續 consumer 拉的順序不再是「先丟先拉」。
//    若你的 use case 要 FIFO (例如 audit log)、改用單 mutex queue 或
//    SPSC ring 配 wrapper。
//
// 6. consumer 找不到工作怎麼辦
//    A. 輪詢 (try_pop 失敗就試下一個):簡單,但 spin 浪費 CPU。
//    B. 全域 cv (任何 push 都 notify):復活了單一爭用點。
//    C. per-shard cv:consumer 釘在自己 shard 等;producer push 時 notify
//       *該 shard* 的 cv。最常見的工業解。
//    D. 跨 shard work-stealing (lesson 08 #2):consumer 自己 shard 空了
//       去偷別人。複雜但 scaling 最好。
//
// 7. 為什麼 lesson 17 的加速比看起來不大
//    本機測試只有 4 P + 4 C × 2M items,單 mutex 575 ms vs 8-shard 438 ms,
//    ~1.31×。關鍵:工作量小 + 元素就一個 int → 鎖的爭用沒那麼戲劇化。
//    在「producer 做點實際工作 (例如序列化)、queue 持有時間更長、core 數
//    更多」的場景,加速會擴大到 3-8×。
//
// 8. lock-free MPMC (Vyukov bounded queue) 的後續路線
//    每個 slot 配一個 atomic<seq_t>,producer/consumer 透過 CAS 更新 seq
//    來確認「該我了」。沒有 mutex,但有 CAS retry。在中等爭用下吞吐
//    可達 sharded 的 2-3×。實作複雜,推薦用 moodycamel 已驗證版本。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✓ LC 1188  Design Bounded Blocking Queue
//     - 該題用一把 mutex 即夠 (LC 測試規模小);本課示範若 N 條 producer/
//       consumer 同時搶,如何透過 sharding 提升吞吐 1.3-3×。
//     - lesson 30 Q5 是「教學版」(單一 mutex),本課是「擴充版」(多 shard)。
//   → lesson 30 Q8 Web Crawler 內部的 queue 也可改用 sharded 版本,
//     讓多 worker 不會搶同一把鎖。
//
// 主要 API 對照 (cppreference):
//   - std::mutex                        https://en.cppreference.com/w/cpp/thread/mutex
//   - std::condition_variable           https://en.cppreference.com/w/cpp/thread/condition_variable
//   - std::queue / std::deque           https://en.cppreference.com/w/cpp/container/queue
//   - std::hash<T>                      https://en.cppreference.com/w/cpp/utility/hash
//
// 練習建議:
//   - 把 lesson 30 Q5 BoundedBlockingQueue 換成 N 個 shard (本課寫法)
//     再跑同樣的測試 ── 8 producer + 8 consumer 時應看到顯著加速。
// =============================================================

/*
補充筆記：mpmc_sharded
  - mpmc_sharded 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - mpmc_sharded 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】MPMC 佇列與分片（sharding）降低爭用
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 單一共享 queue 的 thread pool 為什麼無法線性擴展？
//     答：所有 producer 與 consumer 都要取得同一把 mutex，隨著執行緒數上升，鎖成為
//         序列化的瓶頸（Amdahl 定律中的序列部分），而且鎖與 queue 的 head 節點形成
//         cache line 熱點，跨核搬運成本急速上升。核心數越多，爭用越嚴重，甚至可能
//         出現「加核心反而變慢」。
//     追問：怎麼量測是不是卡在鎖上？（perf 看鎖相關的等待時間，或觀察加核心時
//           throughput 的曲線是否走平甚至下滑）
//
// 🔥 Q2. Sharding 為什麼有效？代價是什麼？
//     答：把一條 queue 拆成 N 條，每個 producer 只往自己分到的那條 push，consumer 以
//         round-robin 嘗試各條——爭用被分散到 N 把鎖上，衝突機率大幅下降。代價是
//         失去全域 FIFO 順序、可能出現負載不均（某條積壓、某條空轉），因此通常要搭配
//         work stealing 或隨機挑選來平衡。
//     追問：為什麼不直接寫 lock-free MPMC queue？（正確的 lock-free MPMC 涉及安全記憶
//           體回收與 ABA，複雜度極高；sharding 只多十幾行就能解決大多數場景）
//
// Q3. Lock-free 與 wait-free 差在哪？
//     答：兩者都屬 non-blocking。Lock-free 保證任意時刻「至少有一個」thread 能取得
//         進展（系統整體不停滯），但個別 thread 可能因 CAS 一直失敗而餓死。Wait-free
//         保證「每一個」thread 都能在有限步數內完成，最強但實作複雜、常數成本高。
//         典型的 CAS 迴圈是 lock-free；fetch_add 這種硬體直接支援的 RMW 才是 wait-free。
//     追問：lock-free 一定比有鎖快嗎？（不一定；它真正的價值是延遲可預測性，以及
//           「不怕持鎖者被搶佔或掛掉」）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

constexpr std::size_t CACHE_LINE = 64;

// -------------------------------------------------------------
// 一條普通的「單佇列」MPMC:全部執行緒搶同一把 mutex。
// 這跟 lesson 08 的池內部一樣。在低負載下沒問題,在高
// 任務率下會被 mutex 爭用拖垮。
// -------------------------------------------------------------
class SingleQueue {
public:
    void push(int v)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        q_.push(v);
    }

    bool try_pop(int& out)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (q_.empty()) return false;
        out = q_.front(); q_.pop();
        return true;
    }

private:
    std::mutex      mtx_;
    std::queue<int> q_;
};


// -------------------------------------------------------------
// 分片版:N 條內部佇列,各自一把 mutex。
// 每條子佇列都對齊到自己的 cache line (lesson 13)。
// -------------------------------------------------------------
template <std::size_t Shards>
class ShardedQueue {
    struct alignas(CACHE_LINE) Shard {
        std::mutex      mtx;
        std::queue<int> q;
    };

public:
    void push(int v)
    {
        // 用 round-robin 的 atomic counter 把 push 平均分散到
        // 各 shard。也可以改用 hash(thread_id) % Shards,效果
        // 接近,但 round-robin 在「producer 數量隨機」時更平均。
        std::size_t idx = next_push_.fetch_add(1, std::memory_order_relaxed) % Shards;
        std::lock_guard<std::mutex> lk(shards_[idx].mtx);
        shards_[idx].q.push(v);
    }

    bool try_pop(int& out)
    {
        // 每個 consumer 從自己「上次拿到」的 shard 開始輪一圈。
        // 這個寫法犧牲了嚴格 FIFO,但在分散式 work-stealing 風格
        // 下幾乎不必在意:任務之間是獨立的。
        thread_local std::size_t start = 0;
        for (std::size_t i = 0; i < Shards; ++i) {
            auto& s = shards_[(start + i) % Shards];
            std::unique_lock<std::mutex> lk(s.mtx, std::try_to_lock);
            if (!lk.owns_lock()) continue;        // 別人在用,跳下一個
            if (!s.q.empty()) {
                out = s.q.front();
                s.q.pop();
                start = (start + i) % Shards;     // 下次從這條開始試
                return true;
            }
        }
        return false;
    }

private:
    std::array<Shard, Shards>   shards_{};
    std::atomic<std::size_t>    next_push_{0};
};


// =============================================================
// Benchmark:P 個 producer 共推 TOTAL 件,C 個 consumer 把它們
// 全部撿走。比較 SingleQueue 與 ShardedQueue 的吞吐量。
// =============================================================
template <typename Q>
static long long bench(const char* label, Q& q,
                       int producers, int consumers, long long total)
{
    // 每個 producer 有自己的固定配額,*不要* 共用 atomic counter
    // ── 那會變成另一個熱點,把佇列本身的爭用蓋掉。
    long long per_producer = total / producers;
    long long real_total   = per_producer * producers;   // 整除後的真實總數

    std::atomic<long long> consumed{0};

    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> ps;
    for (int i = 0; i < producers; ++i) {
        ps.emplace_back([&q, i, per_producer]{
            long long base = i * per_producer;
            for (long long n = 0; n < per_producer; ++n) {
                q.push(static_cast<int>(base + n));
            }
        });
    }

    std::vector<std::thread> cs;
    for (int i = 0; i < consumers; ++i) {
        cs.emplace_back([&, real_total]{
            int v;
            while (consumed.load(std::memory_order_relaxed) < real_total) {
                if (q.try_pop(v)) {
                    consumed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : ps) t.join();
    for (auto& t : cs) t.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();

    std::cout << "[" << label << "] " << ms << " ms"
              << "  (" << (total * 1000 / (ms ? ms : 1) / 1000) << "K ops/s)\n";
    return ms;
}

int main()
{
    constexpr int      P = 4;
    constexpr int      C = 4;
    constexpr long long TOTAL = 2'000'000;

    std::cout << "Workload: " << P << " producers, " << C
              << " consumers, total " << TOTAL << " items\n\n";

    SingleQueue        sq;
    ShardedQueue<8>    shq;

    auto t_single  = bench("single queue (1 mutex) ", sq,  P, C, TOTAL);
    auto t_sharded = bench("sharded queue (8 shards)", shq, P, C, TOTAL);

    std::cout << "\nspeedup from sharding = "
              << (static_cast<double>(t_single) /
                  static_cast<double>(t_sharded)) << "x\n";

    // ---------------------------------------------------------
    // 實戰範例: 事件 dispatcher (8-shard 高吞吐)
    // ---------------------------------------------------------
    // 應用場景: 一個高頻交易/監測系統, 多條 acquisition thread
    // 把 event 丟進中央佇列, 多條 handler thread 取出處理。中央
    // 佇列若用單一 mutex queue, 在每秒幾百萬筆時直接成瓶頸。
    // 分 shard 後爭用點分散, 整體吞吐放大 1.5-3×。
    //
    // 重點: round-robin push 比 hash(producer_id) 更均勻; consumer
    // try_pop 跳下一個 shard 確保不被卡死的 shard 拖累。
    {
        std::cout << "\n[demo] event dispatcher (4 producers, 4 handlers)\n";
        ShardedQueue<8> evq;
        std::atomic<int> handled{0};
        constexpr int per = 50'000;

        std::vector<std::thread> producers;
        for (int p = 0; p < 4; ++p) {
            producers.emplace_back([&, p]{
                for (int i = 0; i < per; ++i)
                    evq.push(p * 1'000'000 + i);   // event id
            });
        }
        std::vector<std::thread> handlers;
        std::atomic<bool> stop{false};
        for (int h = 0; h < 4; ++h) {
            handlers.emplace_back([&]{
                int v;
                while (!stop.load() || evq.try_pop(v)) {
                    if (evq.try_pop(v)) handled.fetch_add(1);
                    else std::this_thread::yield();
                }
            });
        }
        for (auto& t : producers) t.join();
        // 等 consumer 把剩下的吃完
        while (handled.load() < 4 * per) std::this_thread::yield();
        stop.store(true);
        for (auto& t : handlers) t.join();
        std::cout << "  handled = " << handled.load()
                  << " (預期 " << 4 * per << ")\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：shard 數要怎麼選才合理?
    //    A：經驗法則 N = 2 ~ 4 × CPU 核數。太少 (例如 2) 仍會爭用,太多
    //       (例如 64 但只有 8 核) consumer 為了找一個非空 shard 要
    //       try_lock 多次,latency 反而上升。NUMA 機器上每個 node 至少
    //       一條 shard 是個好起點。實務上 16-32 對大多數服務夠用。
    //
    //  Q2：若有 hot key (例如 80% 的工作都集中在 user_id = 1234),
    //      怎麼辦?
    //    A：hash(key) % N 會把所有 hot key 都導到同一個 shard,該 shard
    //       仍然爭用。對策:(a) 改 round-robin 或隨機分散,放棄 locality;
    //       (b) 對 hot key 做二級 sharding:先 hash 到 shard,再用
    //       work-stealing 讓閒的 consumer 偷該 shard 的工作;(c) 偵測
    //       hot key 後切割成多個 sub-key (consistent hashing 加權)。
    //
    //  Q3：為什麼 try_lock 失敗就跳下一個 shard 比 lock() 強?
    //    A：用 lock() 一條 thread 卡住 → 後面排隊的 thread 全部跟著卡;
    //       try_lock + 跳下一個 = wait-free,某個 shard 暫時被佔住不會
    //       拖垮整體。代價是失去嚴格 FIFO,但工作之間獨立的場景不在意。
    //       這也是 work-stealing scheduler 的核心思想 (Cilk, TBB)。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 在多生產者/多消費者下,「單一共享佇列 + 一把 mutex」
//    一定會被爭用 —— 這就是 lesson 08 的池在 100k+ tasks/sec
//    時 scaling 變差的根因。
//
// 2. 解決方案的階梯:
//      Sharded queue (本課) ── 95% 場景夠用,實作只多十幾行
//      ↓
//      Lock-free MPMC (Vyukov、moodycamel)── 高速但難自己寫對,
//                                              用現成函式庫
//      ↓
//      Per-thread queue + work-stealing (TBB、Folly) ── 上限最高,
//                                                       實作最複雜
//
// 3. 分片數 (Shards) 怎麼選?
//      - 約等於 producer 數 + consumer 數。
//      - 太少:仍然爭用。
//      - 太多:try_pop 要試的 shard 數量增加,latency 上升。
//      - 在 SMP/NUMA 機器上,每個 NUMA node 至少一條 shard
//        通常是個好起點。
//
// 4. 分片 *不保證 FIFO*。如果你的工作之間有順序依賴,別
//    用分片;改用單佇列 + 較粗粒度的工作 (lesson 08),或
//    在每件工作裡記錄序號自己重排。
//
// 5. try_lock + 跳過下一個 shard,是 *無等待 (wait-free)*
//    的關鍵 —— 一個被 producer 卡住的 shard 不會把所有
//    consumer 都堵住。簡單但很有效。
//
// 6. 把 lesson 08 的池改用這個 ShardedQueue 是個好練習。
//    submit() 改成 push(...);worker_loop 把 cv_.wait()
//    改成 try_pop + 短 sleep / cv_.wait_for。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 17_mpmc_sharded.cpp -o 17_mpmc_sharded

// === 預期輸出 ===
// Workload: 4 producers, 4 consumers, total 2000000 items
//
// [single queue (1 mutex) ] 738 ms  (2710K ops/s)
// [sharded queue (8 shards)] 324 ms  (6172K ops/s)
//
// speedup from sharding = 2.27778x
//
// [demo] event dispatcher (4 producers, 4 handlers)
//   handled = 200000 (預期 200000)
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
