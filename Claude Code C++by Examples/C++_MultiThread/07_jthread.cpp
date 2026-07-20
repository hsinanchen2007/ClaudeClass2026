// =============================================================
// 07_jthread.cpp  --  std::thread 的兩個地雷,以及 std::jthread
//                     (C++20) 是如何修好它們的
// =============================================================
//
// 本課目標:
//   1. 明確指出 std::thread 的兩個最大地雷:
//        (a) 忘了 join() -> std::terminate。
//        (b) 沒有乾淨的方法去「請求」一個執行緒停止。
//   2. 使用 std::jthread (C++20) —— 它把兩個地雷都修掉。
//   3. 使用 std::stop_token + std::stop_source 進行「協作式
//      取消 (cooperative cancellation)」—— 這是讓 worker 乾淨
//      關閉的標準做法。
//
// 編譯方式 (需要 C++20):
//     g++ -std=c++20 -O2 -pthread 07_jthread.cpp -o 07_jthread
//
// 執行方式:
//     ./07_jthread
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     std::jthread + std::stop_token (C++20),修掉 thread 兩大地雷
// 前置課程: lesson 01
// 觀念詞彙:
//   - jthread        ── 解構子自動 join 的 thread
//   - stop_token     ── 可以查詢「是否被請求停止」的 token
//   - stop_source    ── 控制端,呼叫 request_stop() 觸發
//   - cooperative cancellation ── worker 自己檢查並退出 (非搶佔式)
// 新介紹 API:
//   std::jthread                  同 thread,但解構自動 join
//   std::stop_token st            傳入 worker function 的第一個參數
//   st.stop_requested()           查詢是否該停止
//   t.request_stop()              對 jthread 內建 stop_source 觸發
//   std::stop_source              獨立的停止控制器
//   std::stop_callback            被觸發時自動執行 callback
// 何時使用:
//   - 想要「會自己 join、可以乾淨取消」的 thread (新程式碼預設)
// 何時不要用:
//   - 需要 detach 的 fire-and-forget 場景
//   - C++17 以下的環境 → 退回 std::thread + atomic flag
// 常見錯誤:
//   - worker function 忘了把 stop_token 放在 *第一個參數* → 不會自動注入
//   - stop_token 是建議性的:沒人輪詢就沒效果
//   - 想用 SIGKILL 風格強制取消 → C++ 不允許,設計層級避開
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── jthread 的設計哲學
// =============================================================
//
// 1. cooperative cancellation ── 為什麼不能「強制殺」
//    其他語言常見的「強制中止 thread」(Java Thread.stop、Windows
//    TerminateThread) 在 C++ 與 modern Java 都被棄置,理由:
//      - thread 可能持有 mutex,被殺後鎖永遠不放 → 全程式 deadlock
//      - 持有的 heap allocation 沒解構,記憶體 leak
//      - 中斷在不確定的指令邊界,複合資料結構處於半改狀態
//    解法:cooperative cancellation ── 設旗號,讓被中止的 thread *自己*
//    在安全點檢查並退出。stop_token 就是這個旗號的標準封裝。
//
// 2. stop_source / stop_token / stop_callback 三件套
//    stop_source     擁有「我要停止」這個能力,可以 request_stop()。
//    stop_token      讀取狀態的 view;呼叫 stop_requested() 輪詢。
//    stop_callback   註冊「停止時呼叫此函式」,可以打斷 cv.wait、
//                    close 一個 socket 來打破 read 的阻塞。
//    一個 source 對應多個 token (broadcast)。jthread 自己擁有一個 source,
//    解構子會自動 request_stop。
//
// 3. jthread 解構子的兩步驟
//    ~jthread() 的實作 (簡化版):
//        if (joinable()) {
//            ssource_.request_stop();   // ① 通知該停了
//            join();                    // ② 等 worker 回應退出
//        }
//    比 std::thread (解構直接 terminate) 安全許多。實質上是「destructor
//    幫你做正確的事」,把 RAII 補完。
//
// 4. stop_token 是建議性 ── 必須 *主動輪詢*
//    if (token.stop_requested()) return;
//    這得由 worker 函式定期檢查。寫一條跑死迴圈的工作不檢查 token →
//    request_stop() 沒效果,jthread 解構子會永遠卡在 join。
//    對於阻塞中的呼叫 (cv.wait、I/O read)、用 stop_callback 在停止時
//    打斷:
//        std::stop_callback cb(token, [&]{ cv.notify_all(); });
//    或 cv.wait 的 stop_token 重載:
//        cv.wait(lk, token, []{ return ready; });   // C++20
//    被 token 中斷時 wait 會返回 false。
//
// 5. 與 std::thread 的選擇
//    幾乎所有新程式碼都該用 jthread。例外:你 *真的需要* detach,而 jthread
//    沒提供 detach 那麼乾脆 (其實 jthread 也支援 detach,只是哲學上不鼓勵)。
//    舊程式碼 + 升 C++20 就把 std::thread 換 std::jthread,免去手動 join。
//
// 6. 與其他語言對照
//    Go      context.Context → goroutine 自己 select <-ctx.Done()
//    Rust    tokio CancellationToken / std 沒原生支援 (要靠 channel)
//    Java    Thread.interrupt() + InterruptedException (cooperative)
//    C# .NET CancellationToken (cooperative)
//    全部都是 cooperative。stop_token 是 C++ 在 2020 年補上這個工業標準
//    的最後一塊。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── 因為 LC 題目都把生命週期
//     交給呼叫者,不考你「合作式取消」這種設計議題。
//   → 但 lesson 30 Q8 (Web Crawler) 的 worker 退出機制,本質上就是
//     stop_token 的概念變體 (用 done flag + cv.notify_all 取代)。
//     若改用 std::jthread + stop_token 重寫,worker 不需自己輪詢
//     done,而是 cv.wait 用 stop_token 的重載自動被打斷。
//
// 主要 API 對照 (cppreference):
//   - std::jthread                      https://en.cppreference.com/w/cpp/thread/jthread
//   - std::stop_token                   https://en.cppreference.com/w/cpp/thread/stop_token
//   - std::stop_source                  https://en.cppreference.com/w/cpp/thread/stop_source
//   - std::stop_callback                https://en.cppreference.com/w/cpp/thread/stop_callback
//   - cv_any::wait (stop_token 版)      https://en.cppreference.com/w/cpp/thread/condition_variable_any/wait
//
// 練習建議:
//   - 把 lesson 30 Q8 Web Crawler 的 std::thread 全部換成 std::jthread,
//     刪除手寫的 done flag,改用 stop_token + cv_any.wait(lk, token, pred)。
//     程式碼會短 5-10 行,且更不容易寫錯。
//   - 想理解「為什麼 std::thread 解構是 terminate」可看 lesson 01 #4。
// =============================================================

/*
補充筆記：std::jthread
  - jthread 在 destructor 中自動 request_stop 並 join，比 thread 更適合 RAII。
  - stop_token 只是合作式取消，工作函式必須定期檢查才會停。
  - 不要把 jthread 當強制終止執行緒的工具；C++ 標準不提供安全 kill thread。
  - std::jthread 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::jthread 與 stop_token（C++20 協作式取消）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. jthread 相比 std::thread 好在哪？
//     答：兩個改進。(1) RAII 自動 join：解構子會先 request_stop() 再 join()，不再有
//         「忘記 join 就 terminate()」的問題，且 stack unwinding 時自動等待，例外安全。
//         (2) 內建協作式取消：jthread 自帶 stop_source；若執行緒函式的第一個參數是
//         std::stop_token，jthread 會自動傳入，函式內可輪詢 stop_requested() 或註冊
//         std::stop_callback。
//     追問：condition_variable_any::wait 如何配合 stop_token？（有接受 stop_token 的
//           多載，request_stop 會喚醒等待，這正是 condition_variable_any 存在的理由）
//
// 🔥 Q2. 為什麼 C++ 從不提供「強制終止執行緒」的 API？
//     答：因為無法保證不變式與資源釋放。被強制殺掉的 thread 可能正持有 mutex（永久
//         鎖死其他人）、正處於配置到一半的狀態（記憶體洩漏）、或讓共享資料停在破壞
//         的中間狀態。取消因此必須是協作式的：被取消方自己選擇安全的退出點。
//     追問：那卡在阻塞式 IO 的 worker 怎麼中斷？（stop_token 叫不醒 read()；需要
//           eventfd/self-pipe、socket timeout 或 shutdown() 這類機制）
//
// ⚠️ 陷阱. jthread 的解構子順序是什麼？先 join 還是先 request_stop？
//     答：先 request_stop()，再 join()。順序反過來就會死鎖——若 worker 正在迴圈等待
//         stop 訊號，先 join 等於在等一個永遠不會結束的 thread。
//     為什麼會錯：習慣 std::thread 的人會以為 jthread 只是「自動幫你 join」，於是寫出
//         「無限迴圈但不檢查 stop_token」的 worker，結果解構時卡死。自動 join 只有在
//         worker 真的會回應停止請求時才有意義。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>             // std::thread, std::jthread, std::stop_token
#include <chrono>
#include <stop_token>         // std::stop_source (這個 header 也會被 <thread> 拉進來)
#include <atomic>             // 給 LC 1114 demo 用
#include <functional>         // std::function

// -------------------------------------------------------------
// 地雷 1 —— 忘了呼叫 join()
//
// 如果一個 std::thread 物件還處於 "joinable" 狀態 (也就是你
// 還沒呼叫 .join() 或 .detach()) 就被銷毀,解構子會呼叫
// std::terminate(),整個程式都掛掉。
//
// std::jthread 修好這件事:它的解構子會自動呼叫 .join()。
// 開頭的 "j" 就是 "joining" 的縮寫。
//
// 示範 (這段被註解掉,免得程式真的 crash):
//
//     {
//         std::thread t([]{ /* work */ });
//         // 忘了 t.join() ...
//     }   // <-- 解構子: std::terminate(), abort()
//
// 換成 jthread,同樣的程式碼就沒事:
//
//     {
//         std::jthread t([]{ /* work */ });
//     }   // <-- 解構子:先發出 stop 請求,再 join。
// -------------------------------------------------------------


// -------------------------------------------------------------
// 地雷 2 —— 沒有內建的方式去叫一個執行緒停止
//
// std::thread 完全沒有「取消」的概念。傳統做法是手刻一個
// std::atomic<bool> stop_flag,讓 worker 自己去輪詢。容易
// 寫錯、也容易忘記。
//
// std::jthread 把這件事內建了:
//   - 每個 jthread 內部都擁有一個 std::stop_source。
//   - 如果你的執行緒函式以 std::stop_token 作為 *第一個*
//     參數,jthread 會自動把 token 傳進去。
//   - 呼叫 t.request_stop() (或讓解構子幫你做) 就會把那個
//     token 切換到「已請求停止」。
//   - worker 自行輪詢 token.stop_requested(),然後乾淨地離開。
//
// 這是「協作式」取消:worker 仍然必須自己檢查。C++ 並沒有
// (也不應該有) 「在指令執行到一半時殺掉執行緒」的功能 ——
// 那會破壞鎖、洩漏記憶體,把整個程式搞爛。
// -------------------------------------------------------------

void worker(std::stop_token st, int id)
{
    int n = 0;
    while (!st.stop_requested()) {       // <-- 取消檢查就在這裡
        std::cout << "  [worker " << id << "] tick " << n++ << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    std::cout << "  [worker " << id << "] stop requested, exiting cleanly\n";
}

// -------------------------------------------------------------
// 實戰範例: 可中斷的週期 monitor (heartbeat watchdog)
// -------------------------------------------------------------
// 應用場景: 系統需要每 200ms 檢查一次某項健康狀態 (例如連線、
// 磁碟空間、隊列深度)。jthread + stop_token 讓我們用一個簡單
// 迴圈表達, RAII 結束時自動 request_stop + join, 不會出現
// 「main return 了但 monitor 還在跑」的洩漏。
//
// 注意: 用 stop_requested() 輪詢 + sleep_for 雖然簡單, 但 sleep
// 期間無法立刻響應 stop。生產級寫法要用 condition_variable_any.
// wait_for + stop_token 多載 (可立即叫醒), 這裡為簡單先用 polling。
// -------------------------------------------------------------
void health_monitor(std::stop_token st, int& alerts)
{
    int ticks = 0;
    while (!st.stop_requested()) {
        ++ticks;
        if (ticks % 3 == 0) {                  // 模擬「第 3 次發現異常」
            ++alerts;
            std::cout << "  [monitor] ALERT! tick " << ticks << '\n';
        } else {
            std::cout << "  [monitor] ok    tick " << ticks << '\n';
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
    std::cout << "  [monitor] shutting down, "
              << alerts << " alerts logged\n";
}

void demo_health_monitor()
{
    std::cout << "\n[demo] jthread 週期 health monitor\n";
    int alerts = 0;
    {
        std::jthread mon(health_monitor, std::ref(alerts));
        // 主程式做別的事, 跑 500 ms 後關閉 monitor
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }   // <-- 自動 request_stop() + join()
    std::cout << "  total alerts = " << alerts << '\n';
}

// =============================================================
// LeetCode 1114  Print in Order
// 難度: easy
// =============================================================
// 題意: 三個 thread 各被呼叫一次 first()/second()/third(),
//       無論呼叫順序, 印出順序必須是 first→second→third。
//
// 解題思路: 用兩個 atomic<int> 當「step 旗標」, second/third
// busy-wait 直到上一步完成。最簡單可用 (但 spin)。生產級寫法
// 改用 binary_semaphore (lesson 12) 或 cv (lesson 05)。
//
// 完整解 (semaphore 版) 在 lesson 30 Q1, 這裡示範 atomic 旗標
// 版以呼應本課的 jthread + 簡單同步重點。
// =============================================================
class FooLC1114 {
    std::atomic<int> step_{1};   // 1=可印 first, 2=可印 second, 3=可印 third
public:
    void first(std::function<void()> printFirst) {
        // first 不必等
        printFirst();
        step_.store(2);
    }
    void second(std::function<void()> printSecond) {
        while (step_.load() != 2) std::this_thread::yield();   // spin
        printSecond();
        step_.store(3);
    }
    void third(std::function<void()> printThird) {
        while (step_.load() != 3) std::this_thread::yield();
        printThird();
    }
};

void demo_print_in_order()
{
    std::cout << "\n[demo] LC 1114 Print in Order\n";
    FooLC1114 foo;
    // 故意「亂序」啟動 thread, 看是否仍按 first→second→third
    std::jthread t3([&]{ foo.third ([]{ std::cout << "  third\n";  }); });
    std::jthread t1([&]{ foo.first ([]{ std::cout << "  first\n";  }); });
    std::jthread t2([&]{ foo.second([]{ std::cout << "  second\n"; }); });
    // jthread 解構自動 join
}

int main()
{
    // ---------------------------------------------------------
    // PART 1 —— jthread 自動 join
    //
    // 我們從頭到尾都沒有呼叫 .join()。當 `t` 在這個 block
    // 結尾處離開作用域時,它的解構子會:
    //   1) 對其 stop_source 呼叫 request_stop(),
    //   2) join 這個執行緒。
    // 沒有 std::terminate、沒有洩漏、不必記得做任何事。
    // ---------------------------------------------------------
    std::cout << "[main] PART 1: jthread auto-joins\n";
    {
        std::jthread t(worker, 1);

        // 讓 worker 跑一陣子。
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 這裡沒有呼叫 t.join()。右大括號會處理一切。
    }
    std::cout << "[main] worker 1 cleaned up\n\n";

    // ---------------------------------------------------------
    // PART 2 —— 明確的協作式取消
    //
    // 有時你想在 worker 的作用域結束 *之前* 就停掉它。
    // 自己呼叫 .request_stop() 即可。
    // ---------------------------------------------------------
    std::cout << "[main] PART 2: explicit request_stop\n";
    {
        std::jthread t(worker, 2);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << "[main] asking worker 2 to stop...\n";
        t.request_stop();

        // 右大括號處的解構子也會做一次 stop+join,但明確
        // 呼叫一次能讓 main() 與 worker 的關閉並行進行。
        // 這裡我們就讓解構子完成 join。
    }
    std::cout << "[main] worker 2 cleaned up\n\n";

    // ---------------------------------------------------------
    // PART 3 —— 在多個執行緒之間共用一個停止訊號
    //
    // std::stop_source 是取消的「控制端」。你可以把它的
    // token 交給多個 worker,然後用一次 request_stop() 把
    // 它們全部關掉。
    // ---------------------------------------------------------
    std::cout << "[main] PART 3: one stop_source, three workers\n";
    {
        std::stop_source ss;

        std::jthread t1(worker, 10);   // 每個 jthread 自己內部
        std::jthread t2(worker, 20);   // 都有一個 stop_source...
        std::jthread t3(worker, 30);

        // ...但我們 *也* 想要一個總開關。在 `ss` 上註冊一個
        // callback,把停止請求轉發給每個 jthread。(更簡單的
        // 替代做法:直接把 ss.get_token() 傳進 worker,忽略
        // jthread 自己內部那個。這裡示範一下是想讓你看到
        // std::stop_callback 長什麼樣子。)
        std::stop_callback cb1(ss.get_token(), [&]{ t1.request_stop(); });
        std::stop_callback cb2(ss.get_token(), [&]{ t2.request_stop(); });
        std::stop_callback cb3(ss.get_token(), [&]{ t3.request_stop(); });

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << "[main] flipping the master switch\n";
        ss.request_stop();              // 一次喚醒三個 callback

        // t1/t2/t3 的解構子仍然會 join,所以我們在這裡等。
    }
    std::cout << "[main] all workers cleaned up\n";

    // ---------- 兩個延伸示範 ----------
    demo_health_monitor();
    demo_print_in_order();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：jthread 解構時做的「自動 join」有什麼陷阱?
    //    A：解構順序是 request_stop() → join()。如果 worker 沒檢查
    //       stop_token (例如卡在阻塞 I/O 沒跳出), 解構會永遠 hang。
    //       所以協作式取消的兩半都要做: stop_source 端發訊號, worker
    //       端要在迴圈或可中斷點 stop_requested() 主動退出, 不能假設
    //       jthread 會強制砍 thread (它不會, C++ 沒有 thread cancel)。
    //
    //  Q2：stop_token 跟自己用 std::atomic<bool> 旗標差在哪?
    //    A：(a) stop_token 與 jthread 整合, 解構時自動觸發, 不會忘記。
    //       (b) stop_callback 可以註冊 callback, 在 request_stop() 那一
    //       瞬間執行 (例如關 socket 把 worker 從阻塞 read 拉出來)。
    //       (c) 多個 stop_token 共享同一個 stop_state, 一次叫醒一群
    //       worker。自己的 atomic flag 沒有 callback, 沒法解阻塞。
    //
    //  Q3：jthread 可以 detach() 嗎? 跟 thread::detach() 行為一樣嗎?
    //    A：可以呼叫 detach(), 之後 jthread 就跟 std::thread 一樣 ──
    //       解構不再 join, 也不再自動 request_stop。所以 detach 過的
    //       jthread 失去它的兩大優勢, 跟普通 thread 沒差。實務上 99%
    //       不該 detach (生命週期不可控), 真的需要就直接用 std::thread
    //       表達意圖。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 新程式碼優先使用 std::jthread,不用 std::thread。
//    - 自動 join (沒有 std::terminate 的陷阱)。
//    - 內建的協作式取消 (透過 stop_token)。
//
// 2. 要讓 worker 可以被取消,把 std::stop_token 放在它的
//    *第一個* 參數位置。jthread 會自動把 token 灌進去;
//    呼叫端不必做任何改動。
//
//        void worker(std::stop_token st, ...args);
//        std::jthread t(worker, my_args...);
//
// 3. C++ 中的取消是「協作式」的。worker 必須自己去輪詢
//    st.stop_requested() (或者使用支援 stop_token 的
//    std::condition_variable_any::wait 多載 —— 它在 predicate
//    成立 *或* 收到停止請求時都會回傳)。
//
// 4. 如果你想要「一個開關取消多個 worker」,使用
//    std::stop_source,然後選下列其一:
//       - 把 ss.get_token() 直接傳進每個 worker,或者
//       - 用 std::stop_callback 把現有的 token 串起來
//         (就像 PART 3 示範的)。
//
// 5. std::thread 並沒有被淘汰 —— 你還是會在舊程式碼裡看到,
//    或在需要更細緻控制的場景看到。但對於日常「把這件事丟
//    去另一個執行緒跑」的工作,jthread 嚴格來說都更好。
//
// 6. std::jthread 沒有提供「可以放心忘記」的 detach 等價物。
//    如果你真的想要 fire-and-forget,你也同時放棄了取消的
//    能力;設計時請小心。
// =============================================================
