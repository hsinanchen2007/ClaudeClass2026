// =============================================================
// 08_thread_pool.cpp  --  一個小而完整、可實際運作的執行緒池
// =============================================================
//
// 本課目標:
//   把前 7 課中 *每一個* 基本元件組合成一個可運作的元件:
//
//     - std::jthread + std::stop_token   (lesson 07) 給 worker 用
//     - std::mutex + std::condition_variable (lesson 03, 05)
//                                          負責任務佇列
//     - std::queue                         作為任務緩衝
//     - std::packaged_task / std::future   (lesson 06) 讓呼叫者
//                                          能拿到回傳值與例外
//
//   結果是一個 API 很小的類別:
//
//     ThreadPool pool(4);
//     std::future<int> f = pool.submit(work, arg1, arg2);
//     int result = f.get();
//
//   這個形狀和 std::async 一樣,差別在於工作會跑在固定的
//   N 條可重用執行緒上,而不是每次呼叫都生一條新的 OS
//   執行緒。每個 job system、Web 伺服器、平行演算法的內部
//   都是這樣做的。
//
// 編譯方式 (因為 std::jthread 需要 C++20):
//     g++ -std=c++20 -O2 -pthread 08_thread_pool.cpp -o 08_thread_pool
//
// 執行方式:
//     ./08_thread_pool
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     從零組一個 thread pool ── lesson 03/05/06/07 的綜合
// 前置課程: lesson 03, 05, 06, 07
// 觀念詞彙:
//   - thread pool       ── N 條重複使用的 worker thread
//   - task queue        ── 工作的 FIFO 佇列
//   - type erasure      ── 不同 R 的 task 都能塞進同一個 queue
//   - packaged_task     ── 把 callable 包成「未來可填值的 future」
// 新介紹 API:
//   std::function<void()>          被 type-erase 的工作型別
//   std::packaged_task<R()>        把 callable 接到一個 future
//   std::shared_ptr 包裹 packaged_task    讓 lambda 變得 copyable
//   std::invoke_result_t<F, Args...>     模板取得回傳型別
// 何時使用:
//   - 任務多、短、頻繁 → 重複用 thread 比每次開新的便宜
//   - 想統一管理 worker 生命週期、限制並行度
// 何時不要用:
//   - 任務少且粗 (幾十個、各自幾秒) → std::async 直接開即可
//   - 要極限效能 → 改用 oneTBB / Folly executor / lock-free queue
// 常見錯誤:
//   - shutdown 沒 drain queue → in-flight 任務丟失
//   - 在 critical section 內執行 task → 鎖滯整池
//   - 沒處理 packaged_task 是 move-only → 用 shared_ptr 包
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── thread pool 為什麼是必需品
// =============================================================
//
// 1. 為什麼不要 thread-per-task
//    每次來一個 task 開一條 std::thread,直觀但壞:
//      - 建立 thread 約 ~10-50 µs (clone + 設 stack + 啟動);若 task 本身
//        只有 1 µs 工作,98% 時間花在 thread overhead。
//      - OS 對單一 process 的 thread 數量有上限 (Linux 預設 ~30k,可調)。
//      - 多到一定數量後 context switch 主宰一切 (lesson 01 #3),
//        latency 抖動變大。
//    thread pool 的核心:固定 K 條 thread (K ≈ 核數),task 透過 queue
//    進入,K 條 worker 反覆從 queue 拉工作 → 攤平建立成本、限制併發量。
//
// 2. 三種主流 pool 設計
//    A. 單一全域 queue + 多 worker
//       簡單,正確;但 queue 的 mutex 會被所有 worker 搶 → 高任務率時
//       是瓶頸 (lesson 17 sharded MPMC 解這個)。
//
//    B. Per-thread queue + work-stealing
//       每個 worker 有自己 deque (push/pop 無爭用)。worker 自己 queue
//       空了就「偷」別人的另一端。Cilk、Intel TBB、Java ForkJoinPool、
//       Rust Tokio、Go runtime 都用這個。複雜但 scaling 最好。
//
//    C. 雙層 (global queue + local queues)
//       新任務先進 global,worker 從 global 撈到 local 再執行。混合 A/B
//       的折衷。
//
//    本課的 lesson 08 是 A 型 (教學優先)。實戰要 hot path 通常選 B。
//
// 3. task 的容器:std::function vs 自定義
//    std::function<void()>     最簡單,但 type erasure 有 heap 分配且不
//                              支援 move-only callable (C++23 的
//                              std::move_only_function 才補)。
//    自定義 task type           例如 std::packaged_task 包進 shared_ptr,
//                              或自己寫 polymorphic callable。
//    本課用 packaged_task + shared_ptr 因為 std::function 不接受 move-only。
//
// 4. submit() 的回傳值
//    submit 一個 lambda 通常回 std::future<R>,讓 caller 能 .get() 等結果。
//    內部:把 lambda 包成 packaged_task,task->get_future() 拿 future,把
//    task 推 queue。worker 執行 task 時自動 set 它的 future。
//
// 5. shutdown 的兩種策略
//    A. shutdown_drain:不再接新任務,等 queue 排空、worker 自然結束。
//    B. shutdown_now:設 stop flag,worker 看到 flag 立刻退出,queue 中
//       未執行的任務丟棄 (回 std::future 的 broken_promise 例外)。
//    lesson 22 用更完整的「五階段 graceful shutdown」處理這個議題。
//
// 6. 該用幾條 worker?
//    CPU-bound 任務:K = std::thread::hardware_concurrency() (核數)。
//    I/O-bound 任務:K 可遠超核數,因為大多時間都在阻塞等 I/O。但別瘋狂
//    開幾百條,可能反而被 context switch 拖垮 → 真要高 fan-out I/O 改
//    coroutines + io_uring (lesson 20 + future 進階)。
//    混合工作負載:把 CPU-bound 與 I/O-bound 各自放 *不同 pool* 是最常見
//    的成熟模式。
//
// 7. priority queue 與公平性
//    本課用 FIFO queue (queue 一進一出)。實戰常見:
//      - priority queue:用 binary heap,push/pop O(log n)
//      - bounded queue:滿了 producer 阻塞 (背壓)
//      - delayed queue:支援 schedule(after_duration, task)
//    都是同一個結構的變形;先理解 FIFO + thread pool 再加。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✓ LC 1242  Web Crawler Multithreaded
//     - 經典「thread pool + 共享 visited set + termination detection」。
//       本課 thread pool 是 Q8 解的核心 ── 沒 pool 就只能 thread-per-URL,
//       N 個 URL 開 N 條 thread,效能與資源都炸。
//     - 完整解答 → lesson 30 Q8
//   → 此外,*所有* 需要「N 任務丟給有限 worker 跑」的 LC 都可改用 pool;
//     例如 Q5 Bounded Blocking Queue 的 producer/consumer,本身就是
//     thread pool 的精簡退化版 (queue + 固定 consumer 數)。
//
// 主要 API 對照 (cppreference):
//   - 標準目前 *沒有* std::thread_pool ── 是設計題,要自己寫
//   - std::async (近似的 task 啟動)     https://en.cppreference.com/w/cpp/thread/async
//   - std::packaged_task                https://en.cppreference.com/w/cpp/thread/packaged_task
//   - std::future                       https://en.cppreference.com/w/cpp/thread/future
//   - std::move_only_function (C++23)   https://en.cppreference.com/w/cpp/utility/functional/move_only_function
//   - 未來:std::execution / scheduler  (P2300, C++26)
//
// 練習建議:
//   - 讀完本課,直接看 lesson 30 Q8 怎麼用 thread pool 解 LC 1242。
//   - 進階:把本課的單一 queue 換成 lesson 17 sharded MPMC,觀察高負載
//     下吞吐變化。
// =============================================================

/*
補充筆記：std::thread_pool
  - thread pool 把建立執行緒的成本攤平，核心是工作佇列與 worker lifecycle。
  - 提交工作與關閉 pool 必須同步，避免任務遺失或關閉後仍 enqueue。
  - 例外要被 future 捕捉或在 worker 內處理，否則可能終止整個程式。
  - std::thread_pool 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <future>
#include <functional>
#include <memory>
#include <chrono>
#include <stdexcept>
#include <type_traits>      // std::invoke_result_t

// =============================================================
// CLASS  ThreadPool
// =============================================================
//
// 內部結構:
//
//   workers_            N 個 std::jthread 物件,每個都跑
//                       worker_loop()。在解構子自動 join。
//
//   tasks_              型別已被擦除 (type-erased) 的 callable
//                       的 FIFO。每個項目都是
//                       std::function<void()> —— 真正的回傳
//                       型別已經被「藏」進 std::packaged_task
//                       裡,所以這個佇列才能是同質的。
//
//   mtx_, cv_           守護 tasks_,並通知「有新任務」或
//                       「正在關機」(就是 lesson 05 的模式)。
//
// 為什麼佇列存的是 std::function<void()>,而呼叫者送進來
// 的函式可能會回傳 T?因為這個佇列不能是各種不同型別的
// 混合容器。我們在 push 進去前,先把回傳型別吸收到一個
// packaged_task 內。呼叫者拿到的 future 就連到那個同樣的
// packaged_task,所以當 worker 呼叫那個
// std::function<void()>,正確型別的回傳值 (或例外) 自然會
// 流回呼叫者的 future。
// =============================================================
class ThreadPool {
public:
    // ---------------------------------------------------------
    // 建構 N 個 worker。如果呼叫者沒指定,預設用硬體可平行
    // 執行的執行緒數。
    // ---------------------------------------------------------
    explicit ThreadPool(unsigned n = std::thread::hardware_concurrency())
    {
        if (n == 0) n = 1;          // hardware_concurrency() 可能會回傳 0
        workers_.reserve(n);

        // 每個 worker 都是一個 std::jthread。把 stop_token 放在
        // worker 函式的第一個參數,jthread 會自動把 token 灌
        // 進去 —— 這就是 lesson 07 教的慣用做法。
        for (unsigned i = 0; i < n; ++i) {
            workers_.emplace_back([this](std::stop_token st) {
                worker_loop(st);
            });
        }
    }

    // ---------------------------------------------------------
    // 解構子:乾淨地關閉。
    //
    //   1. 翻一個旗標、通知所有人,讓 worker 不再去抓新任務。
    //   2. vector<jthread> 在被銷毀時會 join 每一個 worker。
    //
    // 沒有被 detach 的執行緒、沒有洩漏、沒有意外。
    // ---------------------------------------------------------
    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            shutting_down_ = true;
        }
        cv_.notify_all();

        // 加上保險:對每個 jthread 也呼叫 request_stop()。
        // 這樣即便 worker 卡在 cv_.wait() 而剛好 shutdown 旗標
        // 在那之前就被設了,也能順利離開 (其實上面那個
        // notify_all 已經會把它叫醒,這只是防禦性寫法)。
        for (auto& w : workers_) w.request_stop();

        // workers_ 的解構子會 join 每個 jthread。完成。
    }

    // 池不可複製 —— 它擁有執行緒。
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // ---------------------------------------------------------
    // submit(fn, args...)
    //
    //   - 把 fn(args...) 包進一個 std::packaged_task<R()>。
    //   - 把一個會呼叫該 task 的 void() lambda push 進佇列。
    //   - 回傳該 packaged_task 的 future,讓呼叫者之後能用
    //     .get() 拿到結果。
    //
    // 模板機制拆解一下:
    //
    //   F&&  Args&&...   —— forwarding reference;我們接受
    //                       lvalue、rvalue、各種形狀的 callable。
    //
    //   std::invoke_result_t<F, Args...>  是 fn(args...) 的
    //   回傳型別。我們把它叫 R。
    //
    //   我們需要 packaged_task 比這個函式還活得久,*而且*
    //   我們 push 進佇列的 lambda 必須是 *可複製的*
    //   (因為 std::function 要求是可複製)。但 packaged_task
    //   本身只能 move 不能 copy。把它包進 shared_ptr 是標準
    //   技巧:shared_ptr 是可複製的,每份複本都指向同一個
    //   task。
    // ---------------------------------------------------------
    template <typename F, typename... Args>
    auto submit(F&& fn, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using R = std::invoke_result_t<F, Args...>;

        // 趁我們還知道參數型別時就 bind。std::bind 會把對
        // 的 value category 安全地存放在 packaged_task 內部
        // 的 heap 上。
        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(fn), std::forward<Args>(args)...)
        );

        std::future<R> fut = task->get_future();

        {
            std::lock_guard<std::mutex> lk(mtx_);

            // shutdown 開始之後就不再接受新工作 —— 那種旗標
            // 設了之後才被 push 進來的任務,可能永遠不會被
            // 執行。
            if (shutting_down_) {
                throw std::runtime_error("ThreadPool: submit after shutdown");
            }

            // 這個 lambda 就是被型別擦除後的 "void()" 物件。
            // 它以值的方式捕獲 shared_ptr -> 在 lambda 還活著
            // 期間,task 也活著。呼叫 (*task)() 會執行
            // fn(args...),packaged_task 會把回傳值 (或例外)
            // 灌進 `fut`。
            tasks_.emplace([task]{ (*task)(); });
        }

        // Lesson 05 規則 (c):釋放鎖之後再 notify。
        cv_.notify_one();

        return fut;
    }

    // 純粹給示範程式印統計用。
    std::size_t worker_count() const { return workers_.size(); }

private:
    void worker_loop(std::stop_token st)
    {
        // 就是 lesson 05 那個經典的「消費迴圈」,只多一個
        // 額外的離開條件:來自 jthread 的 stop_token。
        while (true) {
            std::function<void()> job;

            {
                std::unique_lock<std::mutex> lk(mtx_);

                // 等到下列任一條件成立:有任務、正在關機、
                // 收到停止請求。
                cv_.wait(lk, [this, &st] {
                    return !tasks_.empty()
                        || shutting_down_
                        || st.stop_requested();
                });

                // 即使在關機時也把剩下的工作消化完,除非
                // 使用者明確透過 stop 取消。
                if (tasks_.empty()) {
                    return;     // 佇列空,而且我們是因為
                                // shutdown / stop 被叫醒的 -> 離開。
                }

                job = std::move(tasks_.front());
                tasks_.pop();
            }   // <-- 在執行 job 之前 *先* 釋放鎖。
                //     不然長時間執行的任務會擋住其他 worker
                //     去拿自己的任務。

            try {
                job();          // 例外會被 packaged_task 捕獲,
                                // 並送到呼叫者的 future,所以
                                // 我們在這裡不必做任何特別處理。
            } catch (...) {
                // 加上保險:若 std::function<void()> 不是
                // 包著 packaged_task,理論上仍然可能丟例外。
                // 別讓一個壞 job 把整個 worker 殺掉。
            }
        }
    }

    std::vector<std::jthread>           workers_;
    std::queue<std::function<void()>>   tasks_;
    std::mutex                          mtx_;
    std::condition_variable             cv_;
    bool                                shutting_down_ = false;
};

// =============================================================
// DEMO
// =============================================================

// 一個假裝很慢的工作:睡一下,然後回傳 n*n。
int slow_square(int n, int sleep_ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    return n * n;
}

int always_throws()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    throw std::runtime_error("worker exploded");
}

int main()
{
    // ---------------------------------------------------------
    // 池大小:4 個 worker。每個「任務」睡 200 ms。
    // 12 個任務 * 200 ms / 4 個 worker = 牆上時間 ~600 ms。
    // (序列化執行的話會是 12 * 200 ms = 2400 ms。)
    // ---------------------------------------------------------
    ThreadPool pool(4);
    std::cout << "[main] pool with " << pool.worker_count()
              << " workers\n";

    // -------- Part 1: 12 個任務,把所有結果蒐集回來 --------
    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::future<int>> results;
    for (int i = 1; i <= 12; ++i) {
        results.push_back(pool.submit(slow_square, i, 200));
    }

    long long sum = 0;
    for (auto& f : results) sum += f.get();   // 阻塞等到每個都好

    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  t1 - t0).count();

    std::cout << "[main] sum of squares 1..12 = " << sum
              << "   wall = " << ms << " ms"
              << "  (sequential would be ~2400 ms)\n";

    // -------- Part 2: 一個會丟例外的任務 -------------------
    //
    // 例外會被 packaged_task 捕獲,並由 future::get() 重新
    // 丟出,行為和 std::async 完全一致。池內其他任務不受
    // 影響。
    auto bad = pool.submit(always_throws);
    try {
        bad.get();
    } catch (const std::exception& e) {
        std::cout << "[main] caught from pool task: " << e.what() << '\n';
    }

    // -------- Part 3: 帶捕獲的 lambda --------------------
    //
    // submit() 接受任何 callable,所以帶有捕獲的 closure
    // 也沒問題。
    int x = 10;
    auto fx = pool.submit([x](int y) { return x + y; }, 5);
    std::cout << "[main] lambda result = " << fx.get() << '\n';

    // -------- Part 4: 乾淨關閉 ----------------------------
    //
    // main() 結束時會銷毀 `pool`。解構子翻起 shutdown 旗標、
    // 通知所有 worker,然後 vector<jthread> 會 join 每一個。
    std::cout << "[main] exiting, pool will join all workers\n";

    // -------- Part 5: 實戰 ── parallel map-reduce ---------
    //
    // 應用場景: 對一個 vector 平行套用某個慢函式 (例如「對每筆
    // record 做 hash / 解析 / 壓縮」), 再把所有結果加總。這就是
    // 教科書 map-reduce 的最小版本: map 階段 submit 多個 task,
    // reduce 階段 .get() 收回再累加。
    //
    // 為什麼 thread pool 比每筆 submit 一條 std::thread 好得多:
    //   - 1000 筆任務, 你不會想開 1000 條 OS thread (OS 會被打爆)。
    //   - pool 把工作排到固定 4 條 worker 上, peak 記憶體可控。
    {
        std::cout << "\n[demo] parallel map-reduce via pool\n";
        std::vector<int> input(1000);
        for (int i = 0; i < (int)input.size(); ++i) input[i] = i + 1;

        auto t0 = std::chrono::steady_clock::now();
        std::vector<std::future<long long>> futs;
        // 把 input 切成 10 段, 每段 submit 一個 task
        const int chunks = 10;
        const int per = input.size() / chunks;
        for (int c = 0; c < chunks; ++c) {
            int lo = c * per, hi = lo + per;
            futs.push_back(pool.submit([lo, hi, &input]{
                long long s = 0;
                for (int i = lo; i < hi; ++i) s += input[i];
                return s;
            }));
        }
        long long total = 0;
        for (auto& f : futs) total += f.get();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();
        std::cout << "  sum 1..1000 = " << total
                  << " (預期 " << (1000LL * 1001 / 2) << ")"
                  << ", wall = " << ms << " ms\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 thread pool 比「每來一個 task 開一條 thread」好那麼多?
    //    A：建立 OS thread 約 10 µs (Linux clone + stack 8 MB), 銷毀
    //       也要做 cleanup; 加上 thread 數 ≫ 核數時 context switch 主
    //       宰開銷 (1-3 µs/次)。Pool 把 N 個 task 排到固定 K (≈核數)
    //       條長壽 thread 上, 攤掉建立成本, 也避免過度 oversubscription。
    //       每個 job system / web server / async runtime 都這麼做。
    //
    //  Q2：thread pool 的 worker 數該開多少?
    //    A：純 CPU bound 設 hardware_concurrency() (邏輯核數); I/O
    //       bound 可以 2-4× 核數因為大半時間 thread 在睡; 混合工作就
    //       拆兩個 pool 分隔。重點是「不要動態擴張到無限」── 上限失控
    //       會被一波尖峰打爆 (記憶體、檔案描述符、CPU)。Java
    //       ForkJoinPool / Tokio runtime 預設都是 work-stealing + 固
    //       定核數。
    //
    //  Q3：用 packaged_task + future 把 task 結果交還, 為什麼通常比
    //       std::async 更靈活?
    //    A：std::async 把「啟動策略」和「等結果」綁死, future 解構還
    //       會阻塞 (lesson 06 的坑)。packaged_task 把「task 物件」和
    //       「執行者 (pool worker)」分離, 你決定何時、在哪條 thread
    //       上執行, 解構也不會 block。所以 thread pool 通常用
    //       packaged_task 作為傳遞 task + 取結果的單元。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 為什麼要這樣做。每個任務都生一條執行緒,大約要花
//    幾十微秒,還要分配堆疊。執行緒池讓 N 條執行緒永遠
//    重複使用,所以每個任務的成本降到只有「push 進佇列
//    + 一次 CV notify」(數百奈秒等級)。當任務數量很多或
//    很短時就用池。
//
// 2. 佇列型別的小技巧。
//        std::function<void()>          放在佇列上
//        std::packaged_task<R()>        每次提交一個
//        外面再包 std::shared_ptr       讓 lambda 可複製
//    這就是把「型別擦除的工作佇列」與「型別明確的 future
//    API」結合起來的慣用做法。看到要認得出。
//
// 3. 例外處理是免費的。packaged_task 會抓住例外,
//    future::get() 會重新丟出。不要自己寫一條錯誤管道 ——
//    標準函式庫已經幫你做好了。
//
// 4. 關閉。讓 worker 結束的兩個訊號:佇列空 *且* shutdown
//    旗標被設,或者 jthread 的 stop_token 被觸發。解構子
//    把兩種情形都處理了。
//
// 5. 這個池 *不是* 什麼。
//    - 不是 work-stealing —— 一個共用佇列在高並發時會
//      競爭。要追求高任務率,改用「每個 worker 一個佇列
//      + stealing」(Folly executor、TBB、
//      oneTBB::concurrent_priority_queue、Boost.Asio strand) 才
//      能贏。
//    - 沒有上限 —— 失控的生產者可能把記憶體灌爆。加一個
//      「最大佇列長度」+ 第二個 CV 做 backpressure
//      (lesson 05 提到的 bounded buffer 變體)。
//    - 沒有優先序。需要的話把 std::queue 換成
//      std::priority_queue,key 用一個 Task struct。
//    - 不能單獨取消某個任務。把 stop_token 加到任務簽章,
//      在 submit() 中傳遞 token 即可。
//
//    這些都是不錯的延伸練習 —— 上面那個池就是它們的鷹架。
//
// 6. 在生產環境,優先使用經得起考驗的函式庫:oneTBB、
//    Boost.Asio、Folly,或你平台的 job system。自己寫一個
//    (像這個示範) 是為了 *理解*,不是為了上線。
// =============================================================
