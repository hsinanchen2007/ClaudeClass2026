// =============================================================
// 20_coroutines_pool.cpp  --  C++20 coroutines + thread pool executor
// =============================================================
//
// 本課目標:
//   1. 寫出最小可用的 task 型別 (帶 promise_type),讓函式可以
//      使用 co_await。
//   2. 寫出一個 awaiter (schedule_on),讓 co_await 把 coroutine
//      *轉送到* 一個 thread pool 上繼續執行。
//   3. 看到「同一個函式」一段在主執行緒、一段在 pool worker」
//      ── 這正是現代 C++ 寫非同步程式碼的核心模式。
//
// 注意:這是一支教學版,簡化過。生產環境寫真實 coroutine
// 應用,請使用 cppcoro、libunifex、folly::coro,或等 C++26
// 的 std::execution。
//
// 編譯方式 (需要 C++20):
//     g++ -std=c++20 -O2 -pthread 20_coroutines_pool.cpp \
//         -o 20_coroutines_pool
//
// 執行方式:
//     ./20_coroutines_pool
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     C++20 coroutines + thread pool 作為 executor
// 前置課程: lesson 05, 06, 08, 12
// 觀念詞彙:
//   - coroutine         ── 可暫停/恢復的函式
//   - promise_type      ── 一個 coroutine 的「合約」型別
//   - awaiter           ── 描述「co_await X 時要做什麼」的物件
//   - executor          ── 決定「在哪、何時恢復」coroutine 的東西
//   - schedule_on       ── 「把當前 coroutine 排到 X 上恢復」的 awaiter
// 新介紹 API:
//   #include <coroutine>
//   co_await / co_yield / co_return  coroutine 三大語法
//   std::coroutine_handle<>          可恢復 coroutine 的 handle
//   std::suspend_never / std::suspend_always   標準 awaiter
//   awaiter 三件套:
//     await_ready()    回 true 不暫停 (繼續)
//     await_suspend(h) 取 handle,自己決定何時呼叫 h.resume()
//     await_resume()    恢復後 co_await 表達式的回傳值
// 何時使用:
//   - I/O 密集程式 (一條 OS thread 代理數千個 coroutine)
//   - 想用同步寫法表達非同步控制流
// 何時不要用:
//   - CPU 密集純運算 → 仍用 thread pool / parallel STL
//   - 學習曲線陡峭 → 先把 lesson 05/08 練熟再來
// 常見錯誤:
//   - coroutine handle 沒人 resume → frame 永遠洩漏
//   - 跨 co_await 持鎖 → 恢復後可能在不同 thread,語意混亂
//   - 假設 thread_local 跨 co_await 不變 → 會變
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── C++20 coroutines + executor 整合
// =============================================================
//
// 1. C++20 coroutine 是「stackless」── 何意?
//    傳統 thread / fiber 各有自己的 stack (8 MB 或 ~64 KB)。stackless
//    coroutine 沒有自己的 stack ── 它把所有 *跨 suspension 點需要保留的
//    區域變數* 編譯期分析出來,放進 heap 上配置的 *coroutine frame*
//    (~100-500 bytes)。執行時借用呼叫者的 stack 跑。
//    → 創建/銷毀比 thread 便宜 1000×,適合 1M+ 並行任務 (網路伺服器)。
//
// 2. 三個關鍵字
//      co_await expr   ── 呼叫 expr 的 awaiter,由 awaiter 決定是否 suspend
//      co_yield val    ── 等同 co_await promise.yield_value(val)
//      co_return val   ── 結束 coroutine,等同 co_await promise.final_suspend()
//    一個函式 *只要* 有任一個 co_*,就被編譯器標記為 coroutine。
//
// 3. promise_type / awaiter 三層抽象
//    coroutine 的回傳型別 (例如 Task<T>) 必須含巢狀 promise_type,
//    promise_type 提供:
//      - get_return_object()      ── coroutine 一啟動就呼叫,產出 caller
//                                    收到的物件 (Task)。
//      - initial_suspend()        ── 啟動就 suspend?還是直接跑?回 awaiter。
//      - final_suspend()          ── 結束時的 awaiter (通常 noexcept,
//                                    決定 frame 何時銷毀)。
//      - return_value(T) / return_void()
//      - unhandled_exception()
//
//    awaiter (對 co_await x 而言) 是 x 的型別 (或 await_transform 後)
//    需提供:
//      - bool await_ready()                       ── true → 不 suspend
//      - void/bool/handle await_suspend(handle h) ── 真要 suspend 時做什麼
//      - T await_resume()                         ── resume 後傳回給 co_await 的值
//
// 4. schedule_on(pool) 的工作原理
//    一個 awaiter 拿到當前 coroutine handle h,把「h.resume()」當作 task
//    submit 進 thread pool,然後 await_suspend 返回 → coroutine 暫停。
//    pool 的 worker 拉到這個 task → 呼叫 h.resume() → coroutine 從 suspend
//    點繼續,但已經跑在 *worker thread*。從寫程式者角度看就是「co_await
//    schedule_on(pool); 之後的 code 會在 pool 上跑」。
//    這是「executor」的最小範例。完整版 = std::execution (P2300, C++26)。
//
// 5. 跨 co_await 的執行緒身分變動
//    co_await 之後,coroutine 可能在 *任何* thread 上 resume。意涵:
//      - 跨 co_await 持有的 std::lock_guard<mutex> ── 釋放時的 thread 不
//        是 lock 時的 thread → C++ mutex 規定「誰 lock 誰 unlock」,
//        違反 = UB。
//      - thread_local 變數的值會跳。例如 thread_local id 印出 5,
//        co_await 後印出 12。
//    解決:在 co_await 前釋放鎖、co_await 後重抓 (並重檢條件)。
//
// 6. coroutine frame 的生命週期
//    initial_suspend 返回 std::suspend_always → 一啟動就 suspend,
//    需要外部呼叫 coro.resume() 才會跑。返回 suspend_never → 立即跑到
//    第一個 co_await。
//    final_suspend 返回 suspend_always → coroutine 結束後 frame 仍存活,
//    需手動 coro.destroy()。返回 suspend_never → frame 自動銷毀。
//    寫法選擇影響:Task<T> 一般 final_suspend 設 suspend_always,讓
//    awaiter 能讀結果再 destroy。
//
// 7. 何時用 coroutine,何時不用
//    用:
//      - 高 fan-out async I/O (10k+ 並發連線)
//      - 演算法本身有「依步驟延遲」需求 (爬蟲、parser)
//      - generator (惰性序列)
//    不用:
//      - 純 CPU-bound 大批次工作 → 直接 thread pool / parallel STL
//      - 簡單的 fire-and-forget → std::thread / async
//
// 8. 與其他語言對照
//    Python async/await ── 模型一致 (event loop + awaitable)
//    C# async/await     ── 同上,Task<T> = Task
//    Rust async         ── stackless,Future + executor
//    Go goroutines      ── *stackful* M:N,概念不同
//    JS async/await     ── 同模型,單線程 event loop
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── coroutine 是高階組合,
//     LC 沒考。
//   → 但 lesson 30 Q8 Web Crawler 是 coroutine 的天作之合:每個 URL
//     的 fetch 是 I/O bound,coroutine 讓「N 個 URL 並發」不需 N 條
//     thread,1-2 條 thread 即可運行 1000+ 連線。實戰常見:cppcoro、
//     unifex、folly::coro 提供完整 awaitable 庫。
//
// 主要 API 對照 (cppreference):
//   - <coroutine>                       https://en.cppreference.com/w/cpp/coroutine
//   - std::coroutine_handle<P>          https://en.cppreference.com/w/cpp/coroutine/coroutine_handle
//   - std::suspend_always               https://en.cppreference.com/w/cpp/coroutine/suspend_always
//   - std::suspend_never                https://en.cppreference.com/w/cpp/coroutine/suspend_never
//   - 關鍵字:co_await / co_yield / co_return
//                                       https://en.cppreference.com/w/cpp/language/coroutines
//
// 練習建議:
//   - 把 lesson 30 Q8 Web Crawler 改成 coroutine 模型:每個 URL 是
//     一個 task,co_await 在 fetch 處掛起。對比 thread-per-URL 的
//     資源消耗。
// =============================================================


/*
補充筆記：coroutines_pool
  - C++20 coroutine 是 stackless coroutine；暫停點需要保存的區域狀態會放在 coroutine frame，而不是每個 coroutine 有一條完整 OS stack。
  - 函式只要出現 co_await、co_yield 或 co_return，就會被編譯器轉成 coroutine，回傳型別必須提供 promise_type 合約。
  - awaiter 的 await_ready、await_suspend、await_resume 決定 co_await 是否暫停、暫停後交給誰恢復、恢復後表達式回傳什麼。
  - 把 coroutine 排到 thread pool 恢復時，要特別注意恢復點可能在不同 thread；跨 co_await 持有 mutex lock 通常是危險設計。
  - coroutine frame 的生命週期必須有人管理；若 handle 沒有 destroy 或最終 owner，frame 會洩漏。
  - coroutine 讓非同步流程寫起來像同步程式，但不會自動提供平行效能；CPU-bound 工作仍需要 executor/thread pool 實際排程。
  - thread_local、參考捕獲、指向 stack 物件的指標跨過 co_await 時要重新檢查生命週期與執行緒假設。
  - 教學版 coroutine type 常省略 cancellation、exception propagation、backpressure；工作程式應使用成熟框架或完整處理這些邊界。
*/

#include <coroutine>
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <latch>
#include <atomic>
#include <sstream>

// -------------------------------------------------------------
// 一個簡化版的 thread pool —— 比 lesson 08 還短,單純讓本課
// 自我包含。
// -------------------------------------------------------------
class ThreadPool {
public:
    explicit ThreadPool(int n)
    {
        for (int i = 0; i < n; ++i) {
            workers_.emplace_back([this]{ worker_loop(); });
        }
    }
    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& w : workers_) w.join();
    }
    void enqueue(std::function<void()> fn)
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            tasks_.push(std::move(fn));
        }
        cv_.notify_one();
    }
private:
    void worker_loop()
    {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this]{ return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                job = std::move(tasks_.front());
                tasks_.pop();
            }
            job();
        }
    }
    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mtx_;
    std::condition_variable           cv_;
    bool                              stop_ = false;
};


// =============================================================
// task 型別 ── coroutine 的「回傳」物件
//
// 這就是讓某個函式變成 coroutine 的「許可證」:函式回傳型別
// 必須有對應的 promise_type。下面的 task 是「fire-and-forget」
// 風格 ── 我們不從外面拿結果,只在 coroutine 內部用 latch 來
// 等所有 task 結束。
//
// 寫一個更通用的 task<T>::get() 版本要處理:同步 / 跨執行緒
// 的結果傳遞、例外的 rethrow、生命週期管理 (handle 與 task
// 物件之間的 ownership)。為了教學聚焦,本課省略這些。
// =============================================================
struct task {
    struct promise_type {
        // 由 compiler 呼叫,用來建立 task 物件回傳給呼叫者。
        task get_return_object() { return {}; }

        // initial_suspend 控制「coroutine 是 lazy 還是 eager」:
        //   suspend_always —— 建立後先暫停,呼叫者自行 resume
        //   suspend_never  —— 建立後立刻開始執行 (eager)
        // 本課用 eager。
        std::suspend_never initial_suspend() noexcept { return {}; }

        // final_suspend 控制「coroutine 結束後是否暫停」。
        //   suspend_never  —— 結束時自動清掉 (適合 fire-and-forget)
        //   suspend_always —— 結束後保留,讓人讀結果再清掉
        // 我們選 never。
        std::suspend_never final_suspend() noexcept { return {}; }

        // co_return; 對應 return_void();
        // 若有 co_return value; 則改為 return_value(...)。
        void return_void() {}

        // 例外處理。本課簡化:任何例外都終止程式。
        void unhandled_exception() { std::terminate(); }
    };
};


// =============================================================
// schedule_on ── 讓 co_await 把當前 coroutine 轉送到指定的
// thread pool 上繼續執行。
//
// 三個核心成員:
//   await_ready()    —— 是否已可繼續執行?回傳 false 代表「不,
//                       請暫停我」。
//   await_suspend(h) —— 拿到正在暫停的 coroutine handle,自己
//                       決定何時 / 在哪裡呼叫 h.resume()。我們
//                       把 h.resume() 包成一個 lambda 丟給 pool。
//   await_resume()   —— 恢復後執行的「回傳值」(無回傳時為 void)。
// =============================================================
struct schedule_on {
    ThreadPool& pool;

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) const
    {
        // ★ 這就是「把 coroutine 的延續 (continuation) 排到
        // 另一個 executor 上」這件事的全部意思。
        pool.enqueue([h]{ h.resume(); });
    }

    void await_resume() const noexcept {}
};


// 印出當前執行緒 id 為短字串 (避免長 hex 看起來嚇人)。
static std::string short_tid()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    auto s = ss.str();
    return s.size() > 6 ? s.substr(s.size() - 6) : s;
}


// =============================================================
// 實戰範例: 「跳到 pool 做重活, 再跳回」的兩段式 coroutine
// =============================================================
// 應用場景: web handler 想把 CPU 重的部分丟到 worker pool, 算完
// 還想回到原 thread 寫 socket。模式就是
//   co_await schedule_on{pool};    // 跳到 pool 做事
//   ... heavy work ...
//   co_await schedule_on{ui};      // 跳回另一個 executor
// 這裡示範前半段; 完整 round-trip 需要兩個 executor (例如
// pool + main loop), 主流框架 (folly::coro / asio) 都這麼用。
// =============================================================
task compute_then_log(ThreadPool& pool, int id, std::latch& done)
{
    std::cout << "  [compute " << id << "] starts on tid=" << short_tid() << '\n';
    co_await schedule_on{pool};   // 跳到 pool worker

    // 假設這是 CPU bound 的工作 (例如壓縮、加密、解析)
    long long sum = 0;
    for (int i = 0; i < 100'000; ++i) sum += i;
    std::cout << "  [compute " << id << "] sum=" << sum
              << " on tid=" << short_tid() << '\n';

    done.count_down();
    co_return;
}


// =============================================================
// 一個 coroutine ── 在 main 上開始,co_await 之後在 pool worker
// 上繼續執行。
// =============================================================
task work(ThreadPool& pool, int id, std::latch& done)
{
    std::cout << "  [task " << id << "] start on tid=" << short_tid() << '\n';

    // ★ 這一行就把當前 coroutine 切到 pool worker。
    co_await schedule_on{pool};

    std::cout << "  [task " << id << "] resumed on tid=" << short_tid()
              << "  (now on a pool worker)\n";

    // 在 pool 上做點「真實工作」
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    std::cout << "  [task " << id << "] done\n";
    done.count_down();
}


// =============================================================
// MAIN
// =============================================================
int main()
{
    ThreadPool pool(4);

    constexpr int N = 6;
    std::latch done(N);

    std::cout << "[main] launching " << N << " coroutines on tid="
              << short_tid() << "\n";

    for (int i = 0; i < N; ++i) {
        work(pool, i, done);
        // work() 是 eager start;一進入 work() 就開始跑,
        // 在 co_await 那行掛起,把延續交給 pool。所以這個
        // for 迴圈本身不會阻塞。
    }

    std::cout << "[main] all coroutines spawned, waiting for done\n";
    done.wait();
    std::cout << "[main] all coroutines completed\n";

    // ---------- 實戰: compute_then_log 兩段式 ----------
    {
        std::cout << "\n[demo] compute_then_log (跳到 pool 做 CPU bound)\n";
        std::latch done2(3);
        for (int i = 0; i < 3; ++i) compute_then_log(pool, i, done2);
        done2.wait();
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：coroutine 跟 thread pool 比,優劣在哪?
    //    A：thread pool 一條 OS thread 同時只能跑「一個 task 的全部步驟」,
    //       task 一旦阻塞 (I/O、wait) 整條 thread 跟著閒。coroutine 是
    //       stackless,suspend 時把 frame 留在 heap (~100-500 bytes),
    //       OS thread 立刻釋放給別的 coroutine 用 → 一條 thread 可服務
    //       上千個並發連線。CPU-bound 工作不受惠,I/O-bound 才是主場。
    //
    //  Q2：coroutine 的 scheduler 在哪?跟誰決定 resume 在哪條 thread 上跑?
    //    A：C++20 標準*沒有*內建 scheduler ── 完全靠你自己寫的 awaiter 決定。
    //       本課的 schedule_on::await_suspend 把 h.resume() 投到 ThreadPool,
    //       這就是 scheduler。其他形式:Asio 的 strand、libunifex 的
    //       static_thread_pool、folly::coro::Task 都各自實作。C++26 的
    //       std::execution (P2300) 將標準化 sender/receiver/scheduler 介面。
    //
    //  Q3：為什麼跨 co_await 不能持有 std::lock_guard?
    //    A：co_await 之後 coroutine 可能 resume 在「另一條 thread」上。但
    //       std::mutex 的 spec 規定「lock 的 thread 必須自己 unlock」(POSIX
    //       pthread_mutex 也一樣) ── lock_guard 的解構子在 resume 後的 thread
    //       上跑 → unlock 不是 lock 的人 → UB。同理 thread_local 變數的值會
    //       變、stack 上的 thread-id 會跳。正確做法:co_await 前 unlock,
    //       resume 後重抓並重檢條件。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. C++20 coroutines 是「函式級別」的暫停/恢復機制。任何
//    包含 co_await / co_yield / co_return 的函式,只要它的
//    回傳型別有 promise_type,compiler 就會把它改寫成可暫停
//    的狀態機。
//
// 2. 寫 coroutine 函式庫的核心其實是兩件事:
//      (a) 設計 task<T> / generator<T> 等回傳型別 + promise_type。
//          這決定「coroutine 怎麼啟動、怎麼結束、回傳值怎麼
//          傳出去」。
//      (b) 設計 awaiter (像本課的 schedule_on)。awaiter 決定
//          「co_await X 時要不要暫停、暫停期間要做什麼、
//          恢復時 co_await 表達式會評估出什麼」。
//
// 3. schedule_on 模式 (本課重點) 是 *最重要的一個 awaiter*。
//    它把 coroutine 的「下一段」搬到另一個 executor 上跑。
//    所有 async 框架 (cppcoro、libunifex、Asio 的 awaitable
//    支援、folly::coro) 內部都有這個 building block,只是名字
//    不同 (co_spawn、resume_in、boost::asio::post...)。
//
// 4. 跟 std::async / future 的對比:
//      std::async      -> 啟動一個 task,得到一個 future,要
//                         等結果就阻塞 .get()
//      coroutines     -> 啟動一個 task,co_await 一個 awaitable
//                         不阻塞 *作業系統執行緒*,只暫停
//                         *當前 coroutine*,執行緒可以拿來跑
//                         別的事
//    這就是為何 coroutines 在 *I/O 密集* 場景特別划算 ──
//    一條 OS 執行緒可以代理數千個 coroutine 等網路 I/O。
//
// 5. 三個容易踩的坑:
//      (a) coroutine handle 的生命週期:從 await_suspend 收到
//          的 handle 必須還活著的時候才能 resume。如果你忘了
//          它、handle 沒人 resume,coroutine 會洩漏 (frame 在
//          heap 上)。
//      (b) initial_suspend 與 final_suspend 的選擇直接影響
//          coroutine 的生命週期語意。錯了會 use-after-free
//          或洩漏。
//      (c) co_await 之後執行緒可能變了 ── thread_local、 stack
//          上的 RAII guards 仍是同一個 coroutine frame,但
//          作業系統執行緒已經不同。寫之前先想清楚:這段
//          critical section 跨 co_await 嗎?
//
// 6. C++26 的方向:std::execution (P2300) ── senders / receivers
//    模型,標準化 schedule_on 與更多 awaiter 模式。在那之前,
//    folly::coro 與 libunifex 是最值得學的兩個函式庫。
// =============================================================
