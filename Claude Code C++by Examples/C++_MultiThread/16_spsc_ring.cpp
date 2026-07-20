// =============================================================
// 16_spsc_ring.cpp  --  Lock-free SPSC ring buffer
// =============================================================
//
// 本課目標:
//   1. 自己寫一個「單生產者-單消費者」(SPSC) 的無鎖環形佇列。
//   2. 把 lesson 11 的 release/acquire、lesson 13 的 cache-line
//      padding 全部用上,做出一個「真實世界裡會出現」的資料
//      結構。
//   3. 實測它的吞吐量,看到「沒有任何 mutex / CV」帶來的差距。
//
// 結構:
//   - 容量是 2 的次方 (用 mask 取代 modulo)。
//   - head_ 由 producer 寫,tail_ 由 consumer 寫。
//   - 兩個 index 各自獨佔一條 cache line (lesson 13)。
//   - 自己這邊用 relaxed 讀,讀對方那一邊用 acquire (確保
//     看見對方在 store 前所做的 buf_ 寫入);自己 store
//     用 release (對應另一邊的 acquire)。
//   - 元素資料本身是 *普通* 的非 atomic;它們的可見性是
//     由 head_ 的 release / acquire 配對來保證的 (lesson 11
//     的「發布-訂閱」模式)。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 16_spsc_ring.cpp -o 16_spsc_ring
//
// 執行方式:
//     ./16_spsc_ring
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Lock-free SPSC ring buffer (單生產者-單消費者)
// 前置課程: lesson 04, 11, 13
// 觀念詞彙:
//   - SPSC                ── single producer, single consumer
//   - ring / circular buffer ── 用 mod (或 mask) 環繞的固定大小陣列
//   - publish-subscribe   ── release-store 發布、acquire-load 訂閱
//   - false sharing        ── lesson 13,head/tail 必須隔 cache line
// 新介紹 API:
//   無新 API,全靠 lesson 04+11+13 組合
//   設計重點:
//     alignas(64) on head_, tail_, buf_
//     publish: head.store(seq+1, release)  consume: head.load(acquire)
//     索引用 power-of-two + mask 取代 modulo
// 何時使用:
//   - 1:1 streaming pipeline (audio、log、UI ↔ worker)
//   - 需要每秒上億筆、零同步開銷的場景
// 何時不要用:
//   - 多 producer 或多 consumer → 直接壞,改用 lesson 17 / 27
//   - 容量需要動態擴展 → ring 是固定大小
// 常見錯誤:
//   - 沒 padding head/tail → false sharing 讓效能崩塌
//   - 自己這側用 acquire/release → 浪費,relaxed 即可
//   - 容量不是 2 的次方 → 必須改用 modulo,效能下降
//   - 元素資料寫成 std::atomic → 沒必要,可見性由 head 的 release 帶過來
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── SPSC ring 為什麼能 lock-free
// =============================================================
//
// 1. 「單寫單讀」是這課的特權
//    SPSC = Single Producer, Single Consumer。整個資料結構只有 *一條*
//    thread 寫 head_、*一條* 寫 tail_。
//      - producer 唯一寫 head_、只讀 tail_。
//      - consumer 唯一寫 tail_、只讀 head_。
//    沒有「兩條 thread 同時改同一個 atomic」這種狀況 → 不需要 CAS,
//    relaxed/release/acquire 配對就夠了。MPMC (lesson 17) 沒這種特權,
//    必須用 mutex 或 CAS。
//
// 2. publish/subscribe 套路
//    producer:
//        buf_[head_ & mask_] = value;          // 1. 普通寫元素資料
//        head_.store(head_ + 1, release);      // 2. release-store 推進 head
//    consumer:
//        if (tail_.load(relaxed) == head_.load(acquire)) return false;
//        T v = buf_[tail_ & mask_];             // 3. 普通讀元素
//        tail_.store(tail_ + 1, release);
//    重點:
//      - (1) 與 (3) 是 *普通* 讀寫,不是 atomic ── 因為「head 的 release」
//        到「head 的 acquire」這條 happens-before 邊把資料寫的可見性帶過來。
//      - producer 自己讀 head_ 用 relaxed 即可;它就是寫 head_ 的人,
//        沒人會跟它搶。
//
// 3. 為什麼 capacity 要是 2 的次方
//    索引 wrap-around 兩個選擇:
//      idx % capacity      ── 一條 div 指令,~10-20 cycles。
//      idx & (capacity-1)  ── 一條 and 指令,~1 cycle。
//    熱迴圈每筆都做這個運算,差距明顯。代價是容量必須是 2^k。實務上你
//    不會在乎 1024 vs 1000,所以一定選 power-of-2。
//
// 4. head_ 與 tail_ 必須各自獨佔 cache line
//    head_ 是 producer 唯一寫的;tail_ 是 consumer 唯一寫的。若兩者擠在
//    同一條 cache line:
//      - producer 寫 head_ → invalidate consumer 端的 line
//      - consumer 寫 tail_ → invalidate producer 端的 line
//      - 兩邊永遠在 ping-pong,效能掉 5-10×
//    解法:struct alignas(64) Padded { atomic<size_t> v; }; 各放各的 line。
//    這就是 false sharing (lesson 13) 的同型,在 lock-free 結構特別致命。
//
// 5. 為什麼 SPSC 比有鎖佇列快這麼多
//    無爭用 mutex lock+unlock ~25 ns;有 cv 通知時 ~1-3 µs。
//    SPSC ring 一筆 push 或 pop 約 5-15 ns (純 cache 操作 + 一個 release-
//    store)。差 1-2 個量級。但僅限 1:1 場景。
//
// 6. 何時 SPSC 會慢
//    A. ring 滿:producer 必須等 consumer 拉走才能續推。
//       若這是常態 → 換 unbounded MPMC 或加大 ring size。
//    B. consumer 沒事做時還在 spin → 浪費 CPU。
//       搭一個 atomic.wait() 或 cv 讓 consumer 睡覺,producer push 後 notify。
//    C. 元素過大:每筆 memcpy 主導成本。改傳指標或 small index。
//
// 7. 衍生:bounded vs unbounded ring
//    本課示範的是 bounded (固定 capacity)。要 unbounded 改用 linked list +
//    Michael-Scott queue 或 Vyukov MPMC,設計大幅複雜化。
//    多數高效能應用 (audio buffer、log shipper、networking ring) 都選
//    bounded ── overflow 時直接 drop / block,而非無限累積記憶體。
//
// 8. 與 Disruptor (lesson 27) 的關係
//    Disruptor 把 SPSC ring 推到 SPMC (1 producer, N consumer 廣播)。
//    把這課的 head/tail 概念換成 cursor + per-consumer cursor 即可。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── lock-free 結構是工程
//     主題,LC 不考。
//   → 但 lesson 30 Q5 BoundedBlockingQueue 是 mutex+cv 版的同類結構。
//     在 *純 1 producer + 1 consumer* 場景下,把 Q5 替換成本課的
//     SPSC ring,latency 從 ~1 µs 降到 ~50-100 ns,吞吐 ~10×。
//
// 主要 API 對照 (cppreference):
//   - std::atomic<T>                    https://en.cppreference.com/w/cpp/atomic/atomic
//   - std::memory_order_release/acquire https://en.cppreference.com/w/cpp/atomic/memory_order
//   - alignas                           https://en.cppreference.com/w/cpp/language/alignas
//   - std::hardware_destructive_interference_size
//                                       https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
//
// 練習建議:
//   - 取出 lesson 30 Q5 的測試案例 (1 producer + 1 consumer 連續 push/
//     pop),改用本課 SPSC ring 跑一次,觀察 µs → ns 級的差異。
// =============================================================

/*
補充筆記：spsc_ring
  - spsc_ring 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - spsc_ring 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Lock-free SPSC ring buffer
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. SPSC ring buffer 為什麼可以完全不用 CAS？
//     答：因為只有一個 writer 寫 head、只有一個 reader 寫 tail——每個索引都只有唯一
//         的寫者，不存在「兩個 thread 同時想改同一個索引」的競爭，自然不需要
//         compare_exchange。只要兩個索引是 atomic，用 store(release) / load(acquire)
//         配對即可保證「資料寫入先於索引推進可見」。這是最快的執行緒間佇列。
//     追問：容量取 2 的冪有什麼好處？（可用 & (cap-1) 取代取模，除法／取模在熱路徑
//           上相對昂貴）
//
// 🔥 Q2. push 時 head 的 store 為什麼必須是 release，pop 時的 load 必須是 acquire？
//     答：元素資料本身是普通的非原子物件。producer 先寫 slot，再用 release store 推進
//         head；consumer 用 acquire load 讀到那個新的 head 值時，兩者建立
//         synchronizes-with，於是 release 之前對 slot 的寫入對 consumer 保證可見。
//         若寫成 relaxed，consumer 可能看到新的 head 卻讀到尚未寫入的舊 slot 內容。
//     追問：讀自己這一端的索引可以用 relaxed 嗎？（可以，因為只有自己會寫它，不需要
//           跨執行緒的順序保證）
//
// ⚠️ 陷阱. head 與 tail 相鄰宣告會有什麼問題？
//     答：False sharing。producer 一直寫 head、consumer 一直寫 tail，若兩者落在同一條
//         cache line，每次寫都讓對方的 cache line 失效，跨核 ping-pong 直接吃掉無鎖
//         設計的全部收益。必須用 alignas 把兩個索引放到各自的 cache line。
//     為什麼會錯：無鎖設計者常只檢查「有沒有 CAS、有沒有鎖」，把效能問題全歸給同步
//         原語，忽略了真正的瓶頸往往是記憶體與 cache 的存取模式。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <array>
#include <thread>
#include <iostream>
#include <chrono>
#include <cstddef>

constexpr std::size_t CACHE_LINE = 64;

// -------------------------------------------------------------
// SPSC 無鎖環形佇列
//
// API:
//   try_push(v) -> bool   滿了就回傳 false
//   try_pop(out) -> bool   空了就回傳 false
//
// 限制 (這就是 SPSC 三個字的含意):
//   - 同一時間 *只能有一個* 執行緒呼叫 try_push
//   - 同一時間 *只能有一個* 執行緒呼叫 try_pop
//   - 一個 producer + 一個 consumer 的場景下完全無鎖
//
// 為什麼這麼簡單?
//   - 每個 index 只會被「自己這一邊」寫,所以自己讀自己的
//     index 用 relaxed 即可,沒有競爭。
//   - 跨邊看對方的 index 才需要同步 —— 用 acquire 對應
//     對方的 release。
// -------------------------------------------------------------
template <typename T, std::size_t Capacity>
class SpscRing {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of 2");
    static constexpr std::size_t MASK = Capacity - 1;

public:
    bool try_push(const T& v)
    {
        // 自己的 index,用 relaxed 讀就夠了 —— 沒有人會跟我搶
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = head + 1;

        // 看 consumer 那邊,需要 acquire,以便看到他「pop 走」的
        // 已釋放空間。
        if (next - tail_.load(std::memory_order_acquire) > Capacity) {
            return false;        // 滿
        }

        // 寫資料 —— 普通的 store,沒有 atomic
        buf_[head & MASK] = v;

        // 發布。release 確保上面那次 buf_ 的寫,對於用 acquire
        // 讀 head_ 的 consumer 來說,是可見的。
        head_.store(next, std::memory_order_release);
        return true;
    }

    bool try_pop(T& out)
    {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);

        // 看 producer 那邊,acquire 拿到他「push 進去」之前
        // 的所有寫 (含 buf_)。
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;        // 空
        }

        // 讀資料 —— 普通的 load。可見性已由上面的 acquire
        // 與 producer 的 release 配對保證。
        out = buf_[tail & MASK];

        // 釋放空間給 producer (release,對應 producer 那邊的 acquire)。
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    bool empty() const
    {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

private:
    // ★ 重點:把 head_ 與 tail_ 各放在自己的 cache line。
    // 不這樣的話,producer 寫 head_ 會把 consumer 那邊
    // 包含 tail_ 的 cache line 弄髒,反之亦然 —— 就是
    // lesson 13 的 false sharing。
    alignas(CACHE_LINE) std::atomic<std::size_t> head_{0};
    alignas(CACHE_LINE) std::atomic<std::size_t> tail_{0};
    alignas(CACHE_LINE) std::array<T, Capacity>  buf_{};
};


// =============================================================
// DEMO + Benchmark
// =============================================================
int main()
{
    constexpr std::size_t CAP = 1024;
    constexpr long long   N   = 10'000'000;

    SpscRing<int, CAP> ring;

    auto t0 = std::chrono::steady_clock::now();

    std::thread producer([&]{
        for (long long i = 1; i <= N; ++i) {
            // 滿了就讓出 CPU,稍後重試 (typical busy-wait pattern)
            while (!ring.try_push(static_cast<int>(i & 0x7fffffff))) {
                std::this_thread::yield();
            }
        }
    });

    long long sum = 0;
    long long popped = 0;
    std::thread consumer([&]{
        int v;
        while (popped < N) {
            if (ring.try_pop(v)) {
                sum += v;
                ++popped;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();

    // 預期 sum = 1 + 2 + ... + N = N*(N+1)/2 (對 0x7fffffff 後仍然
    // 等於原值,因為 N <= 2^31-1)
    long long expected = N * (N + 1) / 2;
    std::cout << "[SPSC] passed " << N << " ints in " << ms << " ms"
              << "  (~" << static_cast<long long>(N * 1000.0 / (ms ? ms : 1) / 1'000'000)
              << " M ops/s)\n";
    std::cout << "[SPSC] sum = " << sum
              << "  expected = " << expected
              << "  " << (sum == expected ? "OK" : "MISMATCH!") << '\n';

    // ---------------------------------------------------------
    // 實戰範例 1: audio thread → UI thread 樣本傳遞
    // ---------------------------------------------------------
    // 應用場景: 音訊 callback 在 real-time thread 跑 (低延遲、
    // 嚴禁鎖、嚴禁分配記憶體), 要把樣本 / 計量值送到 UI thread。
    // 這正是 SPSC ring 最經典的用途:
    //   - 一個 producer (audio callback)
    //   - 一個 consumer (UI thread)
    //   - 不可阻塞: 滿了就丟最新一筆而非 sleep (real-time 規則)
    {
        std::cout << "\n[demo] audio→UI SPSC ring (real-time pattern)\n";
        SpscRing<float, 64> audio_ring;
        std::atomic<bool> stop{false};
        std::atomic<int> dropped{0}, consumed{0};

        // 模擬 audio thread: 每 1 ms 產生一個 sample
        std::thread audio([&]{
            for (int i = 0; i < 50; ++i) {
                float sample = i * 0.1f;
                if (!audio_ring.try_push(sample))
                    dropped.fetch_add(1);   // ring 滿就丟, 永不 sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            stop.store(true);
        });
        // UI thread: 每 5 ms 才取一次, 故意比 producer 慢
        std::thread ui([&]{
            float v;
            while (!stop.load() || audio_ring.try_pop(v)) {
                while (audio_ring.try_pop(v)) consumed.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        audio.join(); ui.join();
        std::cout << "  consumed=" << consumed.load()
                  << "  dropped=" << dropped.load()
                  << "  (real-time: 寧可丟也不要 block)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 SPSC ring 比 MPMC queue 快這麼多?
    //    A：SPSC 的 head_ 與 tail_ 各自只有「一條 thread 寫」,完全不需
    //       CAS retry,自己讀自己用 relaxed,看對方再用 acquire 即可。
    //       MPMC 必須用 CAS 或 mutex 解多寫者爭用,熱迴圈裡 cache line
    //       在 N 個核之間 ping-pong;SPSC 一筆 op ~5-15 ns,MPMC 通常 ~50-200 ns。
    //
    //  Q2：head_ 與 tail_ 的 atomic 各該選什麼 memory order 最划算?
    //    A：自己這側讀自己的 index 用 relaxed (沒人會搶);store 自己的
    //       index 必須 release (對應對面的 acquire,把 buf_ 那筆普通寫
    //       publish 出去);讀對方 index 必須 acquire (確保看到對方在
    //       store 前對 buf_ 的寫)。x86 上 release/acquire 幾乎免費,但
    //       語意正確性是 portable 的關鍵 (ARM/POWER 上錯了就 race)。
    //
    //  Q3：為什麼 buf_[i] 元素本身可以是「非 atomic」的普通寫入?
    //    A：因為 producer 在 release-store head_ 之前對 buf_[i] 的寫,
    //       會被 happens-before 邊「打包」進 release;consumer 用 acquire
    //       讀 head_ 之後讀 buf_[i],就能看到那筆寫。這就是 lesson 11
    //       的 publish-subscribe 模式 ── 一條 release/acquire 配對可
    //       同時保護任意數量的非 atomic 資料,不必每筆都做 atomic。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. SPSC ring 是「無鎖編程」的 *入門款*:它不需要 CAS、不需
//    要 hazard pointer、不需要任何複雜技巧 —— 只需要正確配對
//    release/acquire、正確 padding head/tail。但本身就足以
//    在生產環境使用 (audio pipeline、log queue、UI thread
//    與 worker 之間的單向通道...)。
//
// 2. 為什麼不用 mutex+CV?
//      - mutex+CV 在無爭用時也是 ~50ns 級的延遲,進核心
//        sleep 喚醒一輪可達 µs 級。
//      - SPSC ring 在無爭用時是「兩條 atomic + 一個普通寫」,
//        ~5–10ns 級;吞吐量輕鬆衝破每秒上億筆。
//
// 3. 別超出 SPSC 的限制。一旦你有兩個 producer 或兩個
//    consumer,上面的 try_push / try_pop 立刻會壞:兩邊都
//    讀到同一個 head,各自寫不同的資料到同一個 slot。要做
//    MPMC 請看 lesson 17,或進階一點走 hazard pointer +
//    CAS-loop 的方向。
//
// 4. 容量為 2 的次方,讓 (idx & MASK) 取代 (idx % CAP)。
//    現代 CPU 上 mod 也很快,但 mask 是 1 個 cycle 而 div
//    可能 20+ cycles,在 hot loop 累積起來差別明顯。
//
// 5. head_ 與 tail_ 一定要用 alignas 隔開到不同 cache line。
//    這是最常見、最容易忘的優化。沒做的話,就算邏輯正確,
//    效能會被 false sharing 砍 5–10 倍 (lesson 13)。
//
// 6. 元素本身為什麼可以非 atomic?
//      因為它們的可見性綁定在 head_ 的 release-store 與
//      acquire-load 之間。release 之前的所有寫 (含 buf_[i])
//      都被「打包」到那次 release;對應的 acquire 之後的
//      所有讀都能看到那些寫。這就是 lesson 11 的
//      "synchronizes-with" 在實戰裡的應用。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 16_spsc_ring.cpp -o 16_spsc_ring

// === 預期輸出 ===
// [SPSC] passed 10000000 ints in 734 ms  (~13 M ops/s)
// [SPSC] sum = 50000005000000  expected = 50000005000000  OK
//
// [demo] audio→UI SPSC ring (real-time pattern)
//   consumed=50  dropped=0  (real-time: 寧可丟也不要 block)
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
