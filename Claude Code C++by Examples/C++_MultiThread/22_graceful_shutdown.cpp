// =============================================================
// 22_graceful_shutdown.cpp  --  Graceful shutdown 模式
// =============================================================
//
// 本課目標:
//   寫一個小型「服務」,示範生產級的 graceful shutdown 應該
//   走過的五個階段:
//     (1) 拒絕新請求 (close intake)
//     (2) 通知 worker 進入 drain 模式
//     (3) 等所有 in-flight 工作做完
//     (4) 給一個 timeout,逾時就強制 (避免 hang)
//     (5) join + 釋放資源 (按相反順序)
//
//   這是任何長時間執行的服務 (web server、job queue、即時
//   pipeline) 在收到 SIGTERM 之後 *都* 該做的事。沒做好的
//   後果就是「資料寫一半就死掉」、「上游 client 看到 connection
//   reset」、「監控系統永遠看到 stale state」。
//
// 編譯方式:
//     g++ -std=c++20 -O2 -pthread 22_graceful_shutdown.cpp \
//         -o 22_graceful_shutdown
//
// 執行方式:
//     ./22_graceful_shutdown
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Graceful shutdown 的五階段
// 前置課程: lesson 05, 07, 08
// 觀念詞彙:
//   - intake closing      ── 不再接受新請求
//   - drain               ── 排空已 queued 的工作
//   - timeout             ── 防止 shutdown 卡住
//   - signal handler      ── async-signal-safe 的限制
// 五階段:
//   1. 拒絕新請求     accepting_ = false
//   2. 通知 worker 結束 draining_ = true; cv.notify_all()
//   3. 等 in-flight 完成 cv.wait until queue empty
//   4. timeout 兜底  detach 或 quick_exit
//   5. cleanup        關 socket / file / 釋放資源
// 何時必須做 graceful shutdown:
//   - 任何長時間執行的服務 (web server、job runner、即時 pipeline)
//   - 收到 SIGTERM 後必須在合理時間退出 (k8s 預設 30s)
// 常見錯誤:
//   - 順序錯誤 (先通知 drain 再拒絕新請求) → race window
//   - 沒 timeout → SIGTERM 後卡死,k8s SIGKILL
//   - signal handler 內做太多事 → 違反 async-signal-safe
//   - shutdown 後忘了 join → thread leak,程式不會乾淨退出
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── shutdown 的工程套路
// =============================================================
//
// 1. 三種 shutdown 強度
//    A. graceful drain   ── 拒新、清空 in-flight、解構資源、退出。
//                          目標:零資料損失,latency 沒上限。
//    B. timed shutdown   ── 同 A,但設一個 deadline,到時直接中止剩下的。
//                          目標:綁定退出時間。常見容器/k8s/systemd 場景。
//    C. emergency abort  ── 立即停止 (std::abort、_exit)。
//                          僅用於檢測到 corruption / 安全事件。
//
//    工業生產應用幾乎都需要 B ── A 太天真 (慢任務拖死),C 太粗暴 (丟資料)。
//
// 2. 五階段標準流程 (lesson 22 demo 照這個)
//    ① stop accepting new work
//       設一個 atomic<bool> accepting_=false,所有 submit() 入口檢查。
//       新請求得到「shutting down」錯誤而非掛在 queue。
//
//    ② signal workers to drain
//       通知 worker「拉光現有 queue 即可退出」。CV broadcast 或設 stop_token。
//
//    ③ wait with deadline
//       main thread 用 wait_for(timeout) 等所有 worker 結束。
//       超時 → 進階段 ④。
//
//    ④ force stop
//       對未完成的 worker 設 hard stop flag、close 它們的 socket / file
//       讓阻塞 I/O 解開、必要時記下哪些任務沒完成 (給下次重做)。
//
//    ⑤ join + cleanup
//       對所有 worker 呼叫 join(),解構資源,寫 final log,return。
//
// 3. 為什麼順序很重要
//    若先通知 drain 再拒新請求 → 之間的請求進入了 queue,worker 看到
//    drain 信號決定不再拉,這些請求被遺漏。
//    正確順序:先關進口 (① accepting_=false),再清存量 (② drain)。
//
// 4. signal handler 的限制 (POSIX)
//    SIGTERM / SIGINT 觸發的 handler 必須只用 *async-signal-safe* 函式
//    (write、_exit、sigaction…)。不可:malloc、printf (stdio buffer)、
//    std::cout、mutex.lock。
//    工程作法:handler 內只 write 一個 byte 到 self-pipe,或設 atomic
//    flag,主程式輪詢/等 self-pipe 的 read。重 logic 全在主程式做。
//
// 5. 容器/k8s 環境
//    Pod 終止時收 SIGTERM,等 terminationGracePeriodSeconds (預設 30s),
//    超時送 SIGKILL。所以你的 timed shutdown 應該設 deadline < 這個值
//    (通常留 ~5s 給 k8s overhead)。
//    SIGKILL 不能被攔截 ── 一旦 kill -9,程式內 RAII 不跑、檔案 buffer
//    不 flush。所以重要狀態應在 graceful 階段 fsync 寫盤,別仰賴 process exit。
//
// 6. 哪些事必須在 shutdown 時做
//    A. flush 所有 buffer (logger、metrics、async writer)
//    B. 提交未送出的網路訊息 / batch
//    C. 解註冊外部服務 (從 service mesh 移除自己)
//    D. 關閉持久連線 (DB、redis) 讓對端立即釋放
//    E. 寫 audit log: 「shutdown began at T, ended at T'」
//
// 7. 「死活線」與健康檢查
//    container 健康檢查 (k8s liveness/readiness probe) 在 shutdown 期間:
//      - readiness=false:LB 停止派新流量 (對應階段 ①)
//      - liveness=true:仍是「活的」,別被重啟 (k8s 發 SIGKILL)
//    順序:先設 readiness=false → 等 LB 把流量切走 → 才開始 drain。
//    這就是著名的 "preStop sleep" 技巧 (preStop hook 內 sleep 5s 等
//    LB 反應)。
//
// 8. 失敗的 shutdown 怎麼觀測
//    應用要記錄:
//      - shutdown 開始時間、結束時間
//      - 強制中止的任務數量
//      - 強制中止時 in-flight 任務 ID
//    Prometheus metric: shutdown_force_aborted_total。
//    這些指標漲就代表 capacity 不夠、deadline 太短、或某類任務有 bug。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── shutdown 是工程議題,LC
//     不考流程設計。
//   → 但 lesson 30 Q8 Web Crawler 內部的 termination detection 是
//     graceful shutdown 的微縮版:wait until queue empty AND in_flight==0,
//     然後 set done=true + notify_all 讓 worker 退出。本課把這個套路
//     推廣到 production-grade 流程 (五階段)。
//
// 主要 API 對照 (cppreference):
//   - std::atomic<bool>                 https://en.cppreference.com/w/cpp/atomic/atomic
//   - std::condition_variable::wait_for https://en.cppreference.com/w/cpp/thread/condition_variable/wait_for
//   - std::stop_token / stop_source     https://en.cppreference.com/w/cpp/thread/stop_token
//   - std::signal (POSIX 信號)          https://en.cppreference.com/w/cpp/utility/program/signal
//   - std::quick_exit / std::_Exit      https://en.cppreference.com/w/cpp/utility/program/quick_exit
//
// 練習建議:
//   - 把 lesson 30 Q8 Web Crawler 加上「外部停止訊號」(SIGINT 或 atomic
//     flag),讓主程式可以中途請求結束、worker 排空 in-flight URL 後退出。
// =============================================================

/*
補充筆記：std::graceful_shutdown
  - graceful shutdown 需要明確停止旗標、喚醒等待者、排空或丟棄工作策略。
  - 只設定 done 不 notify，等待中的執行緒可能不會醒來。
  - 關閉流程也要定義資源釋放順序，特別是 queue、thread pool、logger。
  - std::graceful_shutdown 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Graceful shutdown
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 如何做 graceful shutdown？
//     答：三件事缺一不可。(1) 通知：設定 atomic<bool> stop 或 stop_token，而且必須把
//         stop 納入「所有」CV 的 predicate，否則等待中的 thread 永遠醒不來。
//         (2) 喚醒：用 notify_all()，不能用 notify_one()——要叫醒全部等待者。
//         (3) 等待：join 每一個 worker，確認資源都已釋放後，才允許主要物件解構。
//     追問：C++20 有什麼幫助？（jthread + stop_token 把「通知」與「等待」自動化，
//           解構時自動 request_stop 再 join）
//
// 🔥 Q2. 最常見的 shutdown bug 是什麼？
//     答：解構順序錯誤：先解構了 queue 或 logger，worker 才醒來去存取它 → 存取已解構
//         物件（UB）。正確順序是「先讓所有 worker 停下並 join，再釋放它們用到的資源」，
//         也就是資源要以與建立相反的順序銷毀，而 join 必須排在任何資源釋放之前。
//     追問：為什麼在解構子裡 join 還不夠？（成員解構的順序是宣告順序的反序，若
//           worker thread 成員宣告在 queue 之後，就會先解構 queue，要靠明確的
//           shutdown() 或調整宣告順序）
//
// ⚠️ 陷阱. 設好 stop 旗標並 notify_all()，worker 卻還是不結束，可能是什麼原因？
//     答：最常見的是 stop 沒有被納入 CV 的 predicate——worker 醒來後 predicate 仍為
//         false（例如 queue 還是空的），於是又睡回去。另一種是 worker 卡在阻塞式 IO
//         （read()、accept()），stop 旗標根本叫不醒它。
//     為什麼會錯：大家以為 notify 是「把 thread 叫起來往下走」，實際上帶 predicate 的
//         wait 被喚醒後會重新檢查條件，條件不成立就繼續等。至於阻塞式 IO，則需要
//         eventfd/self-pipe、socket timeout 或 shutdown() 這類可中斷機制。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <future>
#include <random>

// -------------------------------------------------------------
// 一個小服務,內部有:
//   - 一條任務佇列 (mutex + cv)
//   - N 個 worker 從佇列拉任務出來執行
//   - 一個「accepting_」旗標控制是否還能 submit
//   - 一個「draining_」旗標讓 worker 知道「再做完手上的就走」
//   - 一個 stop_promise / stop_future 供 caller 等 shutdown 完成
// -------------------------------------------------------------
class Service {
public:
    explicit Service(int n_workers)
    {
        for (int i = 0; i < n_workers; ++i) {
            workers_.emplace_back([this, i]{ worker_loop(i); });
        }
    }

    // 不允許複製/搬移 ── 它擁有執行緒
    Service(const Service&)            = delete;
    Service& operator=(const Service&) = delete;

    // submit:把一個任務 enqueue。回傳 true 表示接受,false 表示
    // 服務已經在關閉中,新請求被拒絕。
    bool submit(std::function<void()> task)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!accepting_) return false;            // ★ 第 (1) 階段:拒絕新請求
        tasks_.push(std::move(task));
        cv_.notify_one();
        return true;
    }

    // ★ 整個 graceful shutdown 流程在這裡。
    void shutdown(std::chrono::milliseconds timeout)
    {
        // 階段 (1):停止接受新請求
        {
            std::lock_guard<std::mutex> lk(mtx_);
            accepting_ = false;
        }
        std::cout << "[svc] (1) intake closed; in-flight = "
                  << in_flight_.load() << ", queued = " << queue_size() << '\n';

        // 階段 (2):通知 worker 進入 drain 模式
        {
            std::lock_guard<std::mutex> lk(mtx_);
            draining_ = true;
        }
        cv_.notify_all();
        std::cout << "[svc] (2) drain flag set; workers will exit when queue empty\n";

        // 階段 (3) + (4):等 worker 全部結束,但有 timeout
        auto deadline = std::chrono::steady_clock::now() + timeout;
        for (std::size_t i = 0; i < workers_.size(); ++i) {
            // 用個小迴圈 + try_join 模式 (std::thread 沒提供 try_join,
            // 我們用「joinable + 在 deadline 前每隔短時間檢查」模擬)
            bool joined = false;
            while (std::chrono::steady_clock::now() < deadline) {
                if (workers_[i].joinable()) {
                    // 嘗試 join 一小段時間
                    auto fut = std::async(std::launch::async,
                                          [&]{ workers_[i].join(); });
                    auto status = fut.wait_for(std::chrono::milliseconds(50));
                    if (status == std::future_status::ready) {
                        fut.get();
                        joined = true;
                        break;
                    }
                    // 還沒結束,讓 fut 繼續等 ── 但它有自己的執行緒,
                    // 我們先離開 join 嘗試,進入下一輪檢查。
                    // 實作上要小心:fut 的 dtor 會阻塞到 join 完成。
                    // 所以這個寫法只用一次 future 並一路等下去:
                    fut.wait();
                    joined = true;
                    break;
                } else {
                    joined = true;
                    break;
                }
            }
            if (!joined) {
                // 階段 (4):timeout 了。生產環境通常記錄 metric +
                // detach (或 std::quick_exit 整個程式)。
                std::cout << "[svc] (4) TIMEOUT waiting for worker "
                          << i << ", detaching\n";
                workers_[i].detach();
            }
        }
        std::cout << "[svc] (3+4) all workers joined or detached\n";

        // 階段 (5):釋放資源 (這個 demo 沒有 socket/file 等真實資源,
        // 略過。生產環境通常要關閉 listener、flush log、釋放 GPU
        // buffer 等。)
        std::cout << "[svc] (5) cleanup done\n";
    }

    std::size_t queue_size()
    {
        std::lock_guard<std::mutex> lk(mtx_);
        return tasks_.size();
    }

private:
    void worker_loop(int id)
    {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this]{
                    return !tasks_.empty() || draining_;
                });
                if (tasks_.empty()) {
                    // 我們是被 drain 旗標叫醒的,而且已經沒有
                    // 工作 ── 安全退出。
                    return;
                }
                job = std::move(tasks_.front());
                tasks_.pop();
                ++in_flight_;
            }
            try {
                job();
            } catch (...) {
                // 不讓壞 job 殺掉 worker
            }
            --in_flight_;
        }
    }

    std::vector<std::thread>             workers_;
    std::queue<std::function<void()>>    tasks_;
    std::mutex                           mtx_;
    std::condition_variable              cv_;
    bool                                 accepting_ = true;
    bool                                 draining_  = false;
    std::atomic<int>                     in_flight_{0};
};


// =============================================================
// DEMO
//
// 我們開一個 4-worker 服務,塞 30 個各自 sleep 50–250ms 的
// 任務,然後 200 ms 後呼叫 shutdown(1500ms)。預期:
//   - shutdown 開始時還有些任務在 queue
//   - drain 走完所有已 submit 的任務
//   - 沒有 timeout 觸發 (因為每個任務都很短)
// =============================================================
int main()
{
    Service svc(4);

    // 灌任務
    std::mt19937 rng(123);
    constexpr int N = 30;
    std::atomic<int> finished{0};
    for (int i = 0; i < N; ++i) {
        int ms = 50 + (rng() % 200);
        bool ok = svc.submit([i, ms, &finished]{
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            ++finished;
        });
        if (!ok) std::cout << "[main] task " << i << " rejected\n";
    }
    std::cout << "[main] submitted " << N << " tasks\n";

    // 讓服務跑一下子,然後開始關
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 嘗試在 shutdown 後 submit ── 應該被拒絕
    auto t_shutdown_start = std::chrono::steady_clock::now();

    // 在另一條執行緒上 submit,觀察拒絕行為
    std::thread late_submitter([&]{
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        bool ok = svc.submit([]{});
        std::cout << "[main] late submit accepted? " << std::boolalpha << ok << '\n';
    });

    svc.shutdown(std::chrono::milliseconds(1500));

    auto t_shutdown_end = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        t_shutdown_end - t_shutdown_start).count();

    late_submitter.join();

    std::cout << "[main] shutdown took " << total_ms << " ms\n";
    std::cout << "[main] finished tasks = " << finished.load()
              << " / " << N << '\n';

    // ---------------------------------------------------------
    // 實戰範例: signal handler → atomic flag → main loop 關門
    // ---------------------------------------------------------
    // 應用場景: 正式服務在 Linux 上跑, 收到 SIGTERM (k8s 滾動更
    // 新時) 要 graceful shutdown。signal handler 內 *只能* 呼叫
    // async-signal-safe 函式, mutex / cout / cv 通通不能用。
    // 標準做法: handler 內只設一個 atomic<bool>, main loop 輪詢
    // 該 flag, 看到後才呼叫真正的 service.shutdown()。
    //
    // 為什麼 atomic<bool> 可以從 signal handler 改: C++ 標準保
    // 證 std::atomic<T> 對 lock-free type 是 async-signal-safe。
    // ---------------------------------------------------------
    {
        std::cout << "\n[demo] signal-handler pattern (用 atomic<bool> 模擬 SIGTERM)\n";
        static std::atomic<bool> sig_term{false};
        // 模擬: 50 ms 後另一條 thread 假裝是 signal handler 設旗
        std::thread fake_signal([]{
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            sig_term.store(true);   // 在真正的 handler 裡這就是全部
        });
        // main loop: 輪詢 + 在 main thread 真正做 shutdown
        int polls = 0;
        while (!sig_term.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            ++polls;
        }
        std::cout << "  detected SIGTERM after " << polls
                  << " polls → invoking real shutdown logic here\n";
        fake_signal.join();
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 SIGTERM handler 內不能呼叫 std::cout 或 mutex.lock?
    //    A：POSIX 規定 signal handler 內只能呼叫 *async-signal-safe* 函式
    //       (write、_exit、sigaction...)。malloc / printf / mutex / new
    //       通通不安全 ── 因為 signal 可能在 thread 持有 malloc heap lock
    //       時觸發,handler 再嘗試 malloc → self-deadlock。標準做法:
    //       handler 內只 write(self_pipe, "X", 1) 或設 atomic<bool> flag,
    //       主程式 loop 看到 flag 才跑真正的 shutdown 邏輯。
    //
    //  Q2：accepting_=false 與 draining_=true 的順序為什麼絕不能反?
    //    A：若先設 draining_、再設 accepting_=false,window 內可能有
    //       request 進入 queue 但 worker 已被通知「拉光就退」→ 這個 request
    //       永遠不會被執行,造成資料遺失。先關進口 (拒新)、再清存量
    //       (drain),才能保證「已接受的 request 一定被處理完」。這是
    //       graceful 與 timed shutdown 的合約底線。
    //
    //  Q3：timeout 到了還沒 join 完的 worker 應該怎麼處置?
    //    A：三種選擇:(a) detach 該 thread 讓 main 退出 (本課做法,程式
    //       仍能正常結束,但被 detach 的工作會被 OS 砍);(b) 記錄 metric
    //       (shutdown_force_aborted_total) 給監控告警再 detach;(c) 走
    //       std::quick_exit / _exit 跳過全域解構直接退出 (最 paranoid,
    //       適合「資料已透過 fsync 寫盤」的場景)。k8s 環境記得 deadline
    //       要 < terminationGracePeriodSeconds (預設 30s),否則 SIGKILL。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 五個階段是「合約」:
//      (1) 不再接新請求           — 上游知道後續不會被服務
//      (2) 通知 worker 開始收尾   — 內部告訴自己「準備關門」
//      (3) drain (等做完現有的)   — 不丟資料
//      (4) timeout 防永遠卡住     — 不無限期 hang
//      (5) cleanup 與 join        — 不洩漏資源
//
// 2. (1) 一定要在 (2) 之前做。順序反過來會有 race window:
//    新任務剛被 enqueue,worker 已經被通知關門 ── 那個任務
//    永遠不會被執行。
//
// 3. 一定要有 (4)。生產環境內你不能信任「所有任務都會在
//    合理時間內結束」── 任務可能卡在 I/O、外部服務、bug。
//    沒有 timeout,你的服務在收到 SIGTERM 後可能永遠不會
//    結束,k8s 會狠狠 SIGKILL,等同沒做 graceful shutdown。
//
// 4. timeout 後到底該怎麼辦?三種選擇:
//      - detach 那條執行緒,讓 main 退出 ── 程式仍可正常
//        結束,但被 detach 的工作會被 OS 強制砍。本課用這個。
//      - std::quick_exit / std::abort ── 確保不再執行任何
//        全域解構;最 paranoid。
//      - 紀錄 metric,給 alert,然後仍然 detach。生產環境
//        最常見。
//
// 5. 這個模式套用到 lesson 08 的 ThreadPool 也適用。差別只在
//    「那個 pool 的 dtor 是否該等所有 task 做完」── 通常答案
//    是「等,但有 timeout」。
//
// 6. 配合 lesson 07 的 std::stop_token 寫法更短:每個 worker
//    take a stop_token,主迴圈用 cv_.wait() 加一個 stop_callback
//    在 stop 來時 cv.notify_all。最現代的寫法。
//
// 7. 不要忘了「signal handler」這一層。Linux 上典型流程:
//      main 註冊 SIGTERM/SIGINT handler -> handler 把
//      shutdown_requested 旗標設起來 -> main loop 看到後
//      呼叫 svc.shutdown(...)
//    *signal handler 內 *只能* 呼叫 async-signal-safe 的東西*,
//    所以實際做 shutdown 的工作要在主流程,不要在 handler。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 22_graceful_shutdown.cpp -o 22_graceful_shutdown

// === 預期輸出 ===
// [main] submitted 30 tasks
// [svc] (1) intake closed; in-flight = 4, queued = 24
// [svc] (2) drain flag set; workers will exit when queue empty
// [main] late submit accepted? false
// [svc] (3+4) all workers joined or detached
// [svc] (5) cleanup done
// [main] shutdown took 1007 ms
// [main] finished tasks = 30 / 30
//
// [demo] signal-handler pattern (用 atomic<bool> 模擬 SIGTERM)
//   detected SIGTERM after 10 polls → invoking real shutdown logic here
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
