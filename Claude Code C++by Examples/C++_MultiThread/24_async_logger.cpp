// =============================================================
// 24_async_logger.cpp  --  Async logger (生產者快、I/O 拖到背景)
// =============================================================
//
// 本課目標:
//   1. 看到「同步 logger」在多執行緒下的代價:每條執行緒都
//      要等到 I/O 結束才能繼續,且全部執行緒爭用同一把
//      cout/file 的 mutex。
//   2. 用一個 *async logger* 把問題拆成兩段:
//        producer (應用執行緒)：把訊息字串「投遞」到佇列,馬上回傳。
//        drain   (背景執行緒)：從佇列拿訊息,真的去做 I/O。
//      應用程式碼幾乎完全不再卡 I/O。
//   3. 看到 throughput 顯著上升 ── 即使是 stdout (其實也是
//      系統呼叫 + tty/管道 cost),差距常常是數倍。
//
// 編譯方式:
//     g++ -std=c++20 -O2 -pthread 24_async_logger.cpp -o 24_async_logger
//
// 執行方式:
//     ./24_async_logger
// =============================================================
//
// 設計選擇:
//   - 我們用 *單佇列 + mutex + cv* (lesson 05 的經典模式)。
//     在每秒幾百萬條 log 的場合,這個 queue 會成瓶頸,實務上
//     會升級成 thread_local 的 SPSC ring (lesson 16) + 一條
//     drain thread 對每個 producer 的 ring 輪詢。但這會多很
//     多程式碼,本課用最小可運作版本演示「為何要 async」。
//   - 訊息存 std::string;每條 log 已經分配好。生產級 logger
//     (spdlog、glog) 會把 format 也延遲到背景做,完全只在
//     producer 端推一個 lightweight struct + 一根原始指標。
//
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Async logger ── 把 I/O 從應用 thread 移到背景
// 前置課程: lesson 03, 05
// 觀念詞彙:
//   - synchronous logging  ── 在 caller thread 直接寫到 sink
//   - asynchronous logging ── caller 只 enqueue,背景 thread 寫
//   - drainer              ── 後台負責排空訊息的 thread
//   - bounded vs unbounded ── queue 是否有上限
// 新介紹 API:
//   std::ofstream              檔案輸出 stream
//   std::queue<std::string>    最簡單的訊息容器
//   std::ostringstream         快速組裝 log 字串
// 何時使用:
//   - log/metric/audit/trace 是延遲關鍵的 caller path
//   - 應用 thread 不想被 fsync / network log 拖住
// 何時不要用:
//   - 小工具、CLI、非延遲敏感的應用 → 同步 logging 更簡單
//   - 「絕不能丟」的 audit log → async 在 crash 時可能丟最後幾筆
// 常見錯誤:
//   - 在 producer 端用 std::endl → 每次都 flush,async 優勢殆盡
//   - 沒 bounded queue → 生產者跑太快,記憶體爆掉
//   - 解構時忘了 join drainer → 訊息丟一半
//   - 假設「資料已寫入磁碟」→ OS page cache 仍可能丟 (fsync 才保證)
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── 為什麼 logger 是異步化的經典案例
// =============================================================
//
// 1. 同步 logger 的問題
//    每次 LOG(msg) 都直接 fwrite + flush:
//      - fwrite 走 stdio → mutex (FILE* 內部) + page cache write
//      - 若 sync = std::endl 或 fflush → 等 OS 真的把資料推給核心
//      - fsync 更慘,要等實際寫到 disk
//    熱迴圈每秒 100k 條 log,每條 ~1 µs 同步成本 → 100 ms/s 浪費,
//    且這 100 ms 還是 *延遲尾巴* 不可預測,p99 飆高。
//
// 2. async logger 的核心:把 I/O 移到專用 thread
//    應用 thread (producer) 只做:序列化訊息成 string、push 進 queue。
//    drainer thread (consumer):從 queue 拉 batch、寫入 file。
//    應用端的 LOG() 變成「push 進 queue」── 約 100-300 ns,跟 atomic
//    store 同量級。寫盤的 latency 100% 移到 drainer。
//
// 3. queue 的設計選擇
//    A. mutex + std::deque + cv     ── 教學版,~500 ns / op,簡單。
//    B. SPSC ring (lesson 16)       ── 100 ns / op;但只支援 1 producer。
//    C. MPSC (Multi-Producer, Single-Consumer) lock-free  ── 主流選擇:
//       多應用 thread + 1 drainer。Vyukov MPMC、moodycamel 都可。
//    D. per-thread ring + drainer 輪掃 ── 完全沒共享 queue,producer
//       零爭用。drainer 多花 CPU 掃 N 個 ring。spdlog 的 async mode 接近這個。
//
// 4. backpressure (滿了怎麼辦)
//    A. drop_oldest        ── 把最舊的訊息丟掉,新的進來。常見於 metrics。
//    B. drop_newest        ── 直接拒絕新訊息。
//    C. block               ── 應用 thread 阻塞等 drainer 拉走。最不傷
//                            (零訊息丟失) 但延遲反向傳到應用端。
//    D. overflow buffer     ── 切換到 fallback 結構 (例如同步寫到備援檔)。
//    工業 logger 通常給使用者選 ── high-stakes audit log 用 block,
//    debug log 用 drop_oldest。
//
// 5. batching 的效益
//    drainer 每次拉一條寫一條 → syscall 數量 = 訊息數量。
//    drainer 拉一批 (例如 1000 條或 64 KB) 才一次 write:
//      - syscall 攤平,~50 µs / batch ≈ 50 ns / 訊息
//      - kernel 端可一次 commit 整個 page,IOPS 友善
//    寫法:drainer 從 queue pop 直到累積到 size/time 閾值才 write,
//    或 timer 每 1 ms 強制 flush。
//
// 6. 持久性與當機保證
//    OS write() 後資料只在 page cache。process crash → 進 OS,資料保留。
//    OS panic / 機器斷電 → page cache 中的資料 *丟*。要保證資料寫到實體
//    磁碟,要呼叫 fsync(fd) (或 fdatasync 跳 metadata)。
//    fsync 很慢 (1-10 ms HDD,~50 µs NVMe)。所以 async logger 不每筆 fsync,
//    通常採:
//      - 應用端 graceful shutdown 時做一次 fsync
//      - 重要訊息 (audit) 標記 sync=true,drainer 看到立刻 fsync
//      - 定期 (每秒) fsync 一次
//
// 7. format 在 producer 端 vs drainer 端
//    傳統:LOG("user {} ts {}", id, now())  ── 在 producer 端用 fmt::format
//    產出 string。代價:format 自己 ~500 ns - 2 µs。
//    優化:把 raw 引數 (id, now()) push 進 queue,drainer 端才 format。
//    producer 路徑剩 ~100 ns。spdlog 的 deferred-format mode、bench 用
//    這招;但要注意引數的生命週期 (string_view 不能跨 thread,要 copy)。
//
// 8. 與 syslog / journald / kafka 的關係
//    OS 內建 syslog 已是 async 模型 (你的 send 呼叫進 ringbuffer,
//    syslogd 排掉)。journald 內建。雲端則常推到 kafka / fluentd → 寫盤
//    完全離開應用 process。本地 async logger 只解決「應用 → 本地檔」這一段。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── async logger 是工程模式,
//     LC 沒考。
//   → 但 async logger 的核心架構 = lesson 30 Q5 BoundedBlockingQueue +
//     一條 drainer thread。若你看完本課,應該能 *獨立* 把 Q5 改造成
//     async logger:application thread push 訊息,drainer thread pop
//     並寫檔。
//
// 主要 API 對照 (cppreference):
//   - std::ostream / std::ofstream      https://en.cppreference.com/w/cpp/io/basic_ofstream
//   - std::endl (注意會 flush!)         https://en.cppreference.com/w/cpp/io/manip/endl
//   - std::format (C++20)               https://en.cppreference.com/w/cpp/utility/format
//   - std::print (C++23)                https://en.cppreference.com/w/cpp/io/print
//   - <chrono> 時間戳                   https://en.cppreference.com/w/cpp/chrono
//   - std::lock_guard                   https://en.cppreference.com/w/cpp/thread/lock_guard
//
// 練習建議:
//   - 拿 lesson 30 Q5 BoundedBlockingQueue,改造成 simple async logger:
//     producer 只 push string,單一 drainer pop 並寫到檔案。比較 vs
//     直接 std::cout << ... << std::endl 的延遲。
//   - 進階:支援多種 backpressure 策略 (drop_oldest / drop_newest / block)。
// =============================================================

/*
補充筆記：async_logger
  - async_logger 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - async_logger 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】高效能非同步 Logger 設計
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 非同步 logger 的設計要點？
//     答：目標是讓業務執行緒幾乎零成本。(1) 前端只做「最少必要的資料收集 + 丟進佇列」，
//         絕不在業務執行緒做 I/O；(2) 用 per-thread（thread_local）buffer 收集，避免
//         所有執行緒撞同一把鎖；(3) 背景 flush thread 定期或按水位「批次」寫檔，把
//         syscall 成本攤平；(4) 佇列滿時的策略必須明確；(5) 關閉時要 flush 殘留訊息。
//     追問：為什麼時間戳一定要在前端取得？（在後端取會是「寫出的時刻」而非「事件發生
//           的時刻」，佇列一積壓就完全失真，事故分析時序會錯亂）
//
// 🔥 Q2. 佇列滿了該怎麼辦？
//     答：這是設計決策，必須明確選擇並寫進文件：阻塞（保證不丟日誌，但業務執行緒會被
//         拖慢，可能把延遲問題傳染到主流程）、丟棄（保證不卡，但事故當下往往正是日誌
//         暴增時，最需要的資料反而被丟）、或降級取樣（保留錯誤等級、對 debug 等級取樣）。
//         沒有普適的最佳解，取決於系統是「不可丟日誌」還是「不可增加延遲」。
//     追問：丟棄時要做什麼？（至少記錄「丟了幾筆」，否則資料缺口會被誤讀成沒有事件發生）
//
// Q3. 如何進一步消除前端的格式化成本？
//     答：延後格式化（binary logging）：前端只把 format string 的位址與參數以二進位
//         形式存入 buffer，真正的字串格式化交給背景執行緒或離線工具做。這能把前端成本
//         壓到接近一次 memcpy。代價是日誌檔不再是人類可讀的純文字，需要配套的解碼工具。
//     追問：這與「用 thread_local buffer」是互斥的嗎？（不互斥，兩者常一起用：
//           thread_local 消除鎖爭用，binary logging 消除 CPU 成本）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <sstream>
#include <string>

// -------------------------------------------------------------
// 同步 logger:每條 log 都直接寫到 sink (這裡是 std::ofstream),
// 共用一把 mutex。
// -------------------------------------------------------------
class SyncLogger {
public:
    explicit SyncLogger(const char* path) : sink_(path) {}

    void log(const std::string& msg)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        sink_ << msg << '\n';
        // 注意:不 flush。如果你做了 << std::endl 還會把延遲再
        // 推大一截 (每條 log 都 flush 一次,糟糕的設計)。
    }

private:
    std::mutex     mtx_;
    std::ofstream  sink_;
};


// -------------------------------------------------------------
// Async logger:應用執行緒只負責「把字串塞進佇列」;背景的
// drain 執行緒負責真正寫入 sink。
// -------------------------------------------------------------
class AsyncLogger {
public:
    explicit AsyncLogger(const char* path)
        : sink_(path)
    {
        drainer_ = std::thread([this]{ drain_loop(); });
    }

    ~AsyncLogger()
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stop_ = true;
        }
        cv_.notify_one();
        drainer_.join();
    }

    void log(std::string msg)
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            q_.push(std::move(msg));
        }
        cv_.notify_one();
    }

private:
    void drain_loop()
    {
        std::vector<std::string> batch;
        batch.reserve(64);

        while (true) {
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this]{ return stop_ || !q_.empty(); });

                // 一次拿一整批出來,降低 mutex 爭用
                while (!q_.empty()) {
                    batch.push_back(std::move(q_.front()));
                    q_.pop();
                }
                if (stop_ && batch.empty()) return;
            }

            // 出鎖之後再 *慢慢* 寫到 sink。整批寫只 flush 一次。
            for (auto& s : batch) sink_ << s << '\n';
            batch.clear();
        }
    }

    std::ofstream            sink_;
    std::mutex               mtx_;
    std::condition_variable  cv_;
    std::queue<std::string>  q_;
    bool                     stop_ = false;
    std::thread              drainer_;
};


// =============================================================
// Benchmark
// =============================================================
template <typename Logger>
static long long bench(const char* label, Logger& logger,
                       int threads, int per_thread)
{
    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> ts;
    for (int i = 0; i < threads; ++i) {
        ts.emplace_back([&logger, i, per_thread]{
            for (int k = 0; k < per_thread; ++k) {
                // 每條 log 模擬一個有點長度的訊息 (典型 100 bytes)
                std::ostringstream oss;
                oss << "[thread " << i << "] iter " << k
                    << "  payload=42  status=OK  user_id=12345"
                       "  request_id=abc-def-1234567890";
                logger.log(oss.str());
            }
        });
    }
    for (auto& t : ts) t.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();
    auto total = static_cast<long long>(threads) * per_thread;
    std::cout << "[" << label << "] " << ms << " ms"
              << "  (" << (total * 1000.0 / (ms ? ms : 1) / 1'000)
              << " K msgs/s, "
              << "throughput from producer side)\n";
    return ms;
}

int main()
{
    constexpr int THREADS    = 8;
    constexpr int PER_THREAD = 50'000;

    std::cout << "Workload: " << THREADS << " producer threads, "
              << PER_THREAD << " messages each, ~100 bytes per message.\n\n";

    long long t_sync, t_async;
    {
        SyncLogger lg("/tmp/log_sync.txt");
        t_sync = bench("SYNC  (in-thread I/O)", lg, THREADS, PER_THREAD);
    }
    {
        AsyncLogger lg("/tmp/log_async.txt");
        t_async = bench("ASYNC (drain thread) ", lg, THREADS, PER_THREAD);
        // dtor 會把 queue 排空
    }

    std::cout << "\n>>> producer-side speedup = "
              << (static_cast<double>(t_sync) / static_cast<double>(t_async))
              << "x\n";
    std::cout << "(這是 *應用執行緒看到的* 加速;ASYNC 的 drainer 仍在背景\n"
                 "工作。檔案大小:wc -l /tmp/log_sync.txt /tmp/log_async.txt 應該一樣)\n";

    // ---------------------------------------------------------
    // 實戰範例: thread-safe metrics aggregator
    // ---------------------------------------------------------
    // 應用場景: 服務每筆 request 都要記錄延遲, 統計成直方圖。
    // 跟 async logger 同一個概念 ── 應用端只把 (latency_us)
    // push 進 queue, drainer thread 慢慢聚合, 避免應用端被 atomic
    // 操作或 mutex 拖慢。
    //
    // 這裡用最小版示意: 一個 std::atomic<long long> 直接累加,
    // 由 reporter thread 每 50 ms 印一次。lesson 13 false-sharing
    // 該 padding 的話會更快, lesson 17 sharding 還能再升一級。
    // ---------------------------------------------------------
    {
        std::cout << "\n[demo] metrics aggregator (request latency)\n";
        std::atomic<long long> sum_us{0};
        std::atomic<long long> count{0};
        std::atomic<bool>      stop{false};

        // reporter: 每 30 ms 印一次目前平均
        std::thread reporter([&]{
            while (!stop.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                long long c = count.load();
                long long s = sum_us.load();
                if (c) std::cout << "  [report] avg = "
                                 << (s / c) << " µs over " << c << " reqs\n";
            }
        });
        // 4 條 worker 模擬 request 處理
        std::vector<std::thread> ws;
        for (int t = 0; t < 4; ++t) ws.emplace_back([&]{
            for (int i = 0; i < 200; ++i) {
                // 假裝每個 request 花了 ~50 µs
                std::this_thread::sleep_for(std::chrono::microseconds(50));
                sum_us.fetch_add(50, std::memory_order_relaxed);
                count.fetch_add(1,  std::memory_order_relaxed);
            }
        });
        for (auto& t : ws) t.join();
        stop.store(true);
        reporter.join();
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：async logger 一定要 lock-free queue 嗎?mutex+cv 不夠快嗎?
    //    A：取決於負載。本課 mutex+cv 版每秒可推幾百萬條 log,大多數應用
    //       (~10k-100k logs/s) 完全夠用。每秒上千萬條時 mutex 才會成為
    //       瓶頸,這時升級成 MPSC lock-free queue (moodycamel) 或
    //       per-thread SPSC ring (lesson 16) + drainer 輪詢。spdlog 的
    //       async mode 走 per-thread SPSC,producer 完全零爭用。但要注意
    //       lock-free queue 並不會變魔法 ── 真瓶頸常在 fsync/磁碟。
    //
    //  Q2：drainer 應該每筆 fsync 還是定期 flush?
    //    A：每筆 fsync 可保證「絕不丟」但極慢 (NVMe ~50 µs、HDD 1-10 ms),
    //       async 加速效果消失殆盡。實務折衷:(a) 一般 log 不 fsync,程
    //       式正常退出 graceful shutdown 時做一次;(b) 每秒定期 fsync;
    //       (c) 標記為「sync=true」的重要訊息 (audit log) 才 fsync。
    //       OS panic 時 page cache 會丟,要絕對保證 audit 正確就走同步
    //       logger 或寫 redo log,別走 async。
    //
    //  Q3：bounded queue 滿了該怎麼選 backpressure 策略?
    //    A：(a) drop_oldest:丟最舊的、保留最新,適合 metric/監控資料;
    //       (b) drop_newest:直接拒絕新訊息,適合 debug log;(c) block:
    //       producer 阻塞等 drainer,零訊息丟失但 latency 反向傳到應用
    //       (audit log 的選擇);(d) overflow 切換到 fallback (同步寫到
    //       備援檔)。spdlog 三種都支援。重點:必須有上限,不然 producer
    //       速率超過 drainer 時記憶體會無限膨脹直到 OOM。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 「應用執行緒被 I/O 拖住」是 server 程式碼最常見的 latency
//    來源之一。把 log/metrics/audit/trace 等寫入工作搬到背景,
//    應用 latency 通常立刻下降 30–80%。
//
// 2. Async logger 的核心交易:
//      省下:應用執行緒的 I/O wait 時間
//      代價:多一條 drainer 執行緒、可能會丟失最後一些 log
//            (如果 process 暴力崩潰,queue 內未 drain 的訊息
//             消失)
//      對策:程式正常退出時 join() drainer 確保 queue 排空;
//            針對「絕不能丟」的 log (例如審計記錄),不要走
//            async 路線。
//
// 3. 進階方向 (本課沒做的):
//      (a) Lock-free queue ── 把 lesson 16 的 SPSC ring 拿來
//          每個 producer 一份,drainer 輪詢所有 ring。
//          spdlog 的 async mode 就是這樣。
//      (b) Thread_local buffer (lesson 15) ── 每條 producer
//          先在自己 buffer 累積 N 條,buffer 滿才一次推進
//          shared queue。降低爭用、提高 cache 友善度。
//      (c) 延遲格式化 ── 不在 producer 端做 stringstream/sprintf,
//          只 push 「fmt + 引數」原料,讓 drainer 真正格式化。
//          fmtlib 的 async 模式、glog 等都是這樣做。
//
// 4. 等 drainer 排空 = 等 *記憶體* 寫入完成,不是 *硬碟* 寫入
//    完成。OS 仍會在 page cache 暫存。如果程式 crash 後 OS
//    一起死(罕見),page cache 會丟。要確保「事件已寫到
//    硬碟」必須 fsync/fdatasync。
//
// 5. 不要把 async logger 當成「免費的午餐」。如果你的 producer
//    速率 > drainer 排空速率,queue 會無上限增長,記憶體爆掉。
//    生產級 async logger 必須有:
//      - 上限 (bounded queue)
//      - 滿了之後的策略:阻塞、丟棄最舊、丟棄最新、回退到
//        sync (spdlog 三種都支援)
//
// 6. 一個常見錯誤:在 logger 內部用 std::endl ── 它會 flush。
//    每條 log 都 flush 等於每條都 syscall + disk barrier,async
//    的優勢被抵消殆盡。寫 '\n' 就好,讓 drainer 結束時或
//    queue 排空時 flush 一次。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 24_async_logger.cpp -o 24_async_logger

// === 預期輸出 ===
// Workload: 8 producer threads, 50000 messages each, ~100 bytes per message.
//
// [SYNC  (in-thread I/O)] 285 ms  (1403.51 K msgs/s, throughput from producer side)
// [ASYNC (drain thread) ] 372 ms  (1075.27 K msgs/s, throughput from producer side)
//
// >>> producer-side speedup = 0.766129x
// (這是 *應用執行緒看到的* 加速;ASYNC 的 drainer 仍在背景
// 工作。檔案大小:wc -l /tmp/log_sync.txt /tmp/log_async.txt 應該一樣)
//
// [demo] metrics aggregator (request latency)
//   [report] avg = 50 µs over 800 reqs
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
