// 檔案：lesson_3_4_exception_escapes_thread.cpp
// 說明：執行緒內拋出的例外，無法被建立它的執行緒接住
//
// 【本檔是「刻意示範錯誤」的範例，執行時一定會 abort，這就是本課重點】
//
// ⚠️ 這【不是】未定義行為，而是標準規定的行為：
//    例外若從執行緒的進入點函式往外傳播（沒有在該執行緒內被接住），
//    會直接呼叫 std::terminate()，程式立刻中止。
//    例外【不會】跨執行緒傳播，所以 main 的 catch 根本沒有機會執行。
//    → 一定會發生，不是隨機。
//
//    實測結果：exit code 134 (SIGABRT)，stderr 顯示
//    "terminate called after throwing an instance of 'std::runtime_error'"
//    "  what():  執行緒內的錯誤！"
//    而 "捕獲: ..." 這行【永遠不會】被印出來。
//
// ✅ 正確作法：在執行緒函式【內部】用 try/catch 包住，
//    再透過 std::exception_ptr + std::current_exception() 把例外帶回主執行緒，
//    或改用 std::promise / std::future（future.get() 會重新拋出）。
//    本課後續檔案（3.4 系列）會逐一示範這些作法。

// =============================================================================
//  課程 3.4：執行緒例外處理1.cpp  —  例外從執行緒進入點逃逸 → std::terminate()
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：   <thread>（std::thread）、<stdexcept>（std::runtime_error）
//   標準版本： C++11 起（std::thread 與本規則同時引入，至今未變）
//   規範出處： [thread.thread.constr]
//              "If the invocation of the copy of f terminates with an
//               uncaught exception, std::terminate is called."
//              → 「呼叫 std::terminate」是【標準明文規定】，不是未定義行為。
//   行為鏈：   例外逃逸 worker() → std::terminate() → terminate handler
//              → 預設 std::abort() → SIGABRT
//   實測退出碼：134（= 128 + 6，6 是 SIGABRT；這是 shell 慣例，非 C++ 標準）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼例外「不能」跨執行緒傳播 —— 從堆疊的物理結構講起】
//   例外傳播（propagation）的本質是【堆疊回溯 stack unwinding】：從 throw 點
//   往【當前執行緒自己的呼叫堆疊】一層一層往回找 catch handler。
//
//   每一條執行緒都有【自己獨立的堆疊】（Linux/pthread 預設 8 MB，見 ulimit -s）。
//   worker() 的呼叫堆疊長這樣：
//
//        [worker 執行緒的堆疊]              [main 執行緒的堆疊]
//        ┌──────────────────┐             ┌──────────────────┐
//        │ worker()   ← throw│             │ main()           │
//        ├──────────────────┤             │  └ t.join()  ← 阻塞中
//        │ std::thread 內部  │             ├──────────────────┤
//        │  的 invoke 包裝層 │             │ __libc_start_main│
//        ├──────────────────┤             └──────────────────┘
//        │ start_thread     │
//        │ (pthread 進入點) │              兩個堆疊【互相獨立】，
//        └──────────────────┘              沒有任何 caller/callee 關係。
//
//   關鍵在於：main 並【不是】 worker 的呼叫者。`std::thread t(worker)` 不是
//   「呼叫 worker」，而是「請 OS 另外開一條執行緒去跑 worker」。回溯走到
//   worker 執行緒堆疊的最底層（pthread 進入點）就【到頂了】，下面沒有 main
//   的框架可以繼續找 —— 例外無處可去。
//
// 【2. 為什麼 t.join() 的 try/catch 攔不到】
//   最高頻的誤解是把 join() 想成「等於呼叫 worker()」。它不是。join() 的語意
//   只有一個：「阻塞，直到那條執行緒結束」。它是【同步原語】，不是函式呼叫。
//
//   而且時序上更殘酷：worker() 可能在 main 還沒執行到 `t.join()` 這一行之前
//   就已經拋出例外並 terminate 了。程式在那個瞬間整個死掉，main 的 try 區塊
//   根本沒機會進入。
//
// 【3. 標準為什麼選擇 terminate，而不是「幫你存起來、join 時再拋」？】
//   這是刻意的設計取捨，理由有三：
//     (a) 執行緒不一定有人 join。detach() 之後根本沒有接收端，例外要存到誰身上？
//         要存活多久？誰負責釋放？沒有合理答案。
//     (b) join() 的契約會被汙染。若 join() 可能丟出「任意型別」的例外，那所有
//         呼叫 join() 的地方（包括解構子裡的 join）都得準備接住任意例外，
//         RAII 包裝將寸步難行。
//     (c) 「回傳結果的通道」本來就該是另一個抽象。標準把這件事交給
//         std::future / std::promise —— 它們【顯式建模】了一個 shared state
//         來承載「值 或 例外」。想要例外跨執行緒？就用那個工具，別讓
//         std::thread 兼差。
//
// 【4. 三種正確作法（本課後續檔案逐一示範）】
//     (1) 檔案 3：執行緒內 try/catch(...) + std::current_exception() 存進
//         std::exception_ptr，主執行緒 join() 後 std::rethrow_exception()。
//     (2) 檔案 6：std::promise::set_exception()，future.get() 時自動重拋。
//     (3) 檔案 5：std::async —— 回傳的 future 自動搬運例外，最省事。
//
// 【概念補充 Concept Deep Dive】
//
// (A) std::terminate 到底做了什麼
//     它呼叫【當前的 terminate handler】（std::get_terminate() 取得，可用
//     std::set_terminate() 抽換）。預設 handler 是 std::abort()。
//     但 libstdc++ 實際裝的是 __gnu_cxx::__verbose_terminate_handler，它會先
//     用 RTTI 印出例外型別名與 what()，才 abort —— 這就是我們看到
//     "terminate called after throwing an instance of 'std::runtime_error'"
//     的來源。⚠️ 這段訊息是【實作定義】的，MSVC 不會印 what()。
//
// (B) Itanium C++ ABI 的實際流程
//     throw → __cxa_throw() → _Unwind_RaiseException() 開始兩階段回溯：
//       Phase 1（search）：只走訪、不破壞，找有沒有 handler。
//       Phase 2（cleanup）：真的回溯，執行區域物件解構子。
//     本例在 Phase 1 就走到堆疊頂端仍找不到 handler，回傳
//     _URC_END_OF_STACK → __cxa_throw 直接呼叫 std::terminate()。
//     ⚠️ 注意：Phase 2【從未執行】。
//
// (C) 【實測】區域物件的解構子不會執行
//     [except.handle]/9 規定：找不到 handler 時呼叫 std::terminate，
//     「whether or not the stack is unwound before this call is
//      implementation-defined」（是否先回溯堆疊由實作決定）。
//     本機實測（g++ 15.2.0 / libstdc++）：在 worker 內放一個有解構子的區域
//     物件，例外逃逸時該解構子【不會】被呼叫 —— 也就是 GCC 選擇【不回溯】。
//     實務意義：這種情況下 RAII 不保證生效，檔案不會被關、鎖不會被釋放。
//     反過來說也正因為如此，「讓例外逃逸」在有資源的程式裡格外危險。
//
// (D) 為什麼收集輸出要用 stdbuf -o0
//     std::cout 在導向檔案/管線時是【全緩衝】的。abort() 不是正常結束，
//     【不會 flush】stdout 緩衝區 —— 已經 << 進去但還沒沖出來的內容會直接
//     隨程序消失。用 `stdbuf -o0` 關掉緩衝才能看到程式死前真正印了什麼。
//     （本檔 main 在死前沒有輸出，但這個習慣在別的 abort 示範會差好幾行。）
//
// (E) 退出碼 134 的算法
//     被訊號殺死的程序，shell 回報 128 + 訊號編號。SIGABRT = 6，故 134。
//     若在程式裡 std::set_terminate() 換成自訂 handler 並呼叫 std::exit(1)，
//     退出碼就會變成 1 —— 退出碼不是本行為的固定特徵，terminate 才是。
//
// 【注意事項 Pay Attention】
//
// 1. ⚠️ 這【不是】未定義行為（UB），是【標準保證】的確定行為。
//    「一定會 abort」這句話在本檔是【正確】的，不要改寫成「可能」。
//    UB 的例子是 data race 或迭代器失效 —— 那種才不可預測。
//
// 2. try/catch 包住 join() 並非全然無用：join() 自己會丟 std::system_error。
//    本機實測的兩個情況：
//      * 對 non-joinable 的 thread 呼叫 join() → "Invalid argument"
//      * 在執行緒內部 join 自己         → "Resource deadlock avoided"
//    所以那個 catch 抓得到【join 自己的錯】，只是永遠抓不到 worker 的例外。
//
// 3. 改成 t.detach() 也一樣 terminate —— 逃逸點在 worker，與 join/detach 無關。
//
// 4. 別以為加 noexcept 有幫助。給 worker 加 noexcept 只會讓它在拋出當下
//    就直接 terminate（連逃逸都省了），不會變好。
//
// 5. 其他【同樣是標準保證】的 terminate 場景（別跟 UB 混為一談）：
//      * 解構一個仍 joinable 的 std::thread（沒 join 也沒 detach）
//      * 例外逃出標成 noexcept 的函式
//      * 堆疊回溯期間，解構子又拋出例外
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】例外與執行緒邊界
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼主執行緒的 try/catch 包住 t.join() 卻接不到子執行緒的例外？
//     答：例外傳播是沿著【拋出者自己那條執行緒的堆疊】做 unwinding，而 main
//         的堆疊和 worker 的堆疊完全獨立，兩者沒有 caller/callee 關係。
//         join() 只是「等它結束」的同步原語，不是函式呼叫。例外在 worker
//         堆疊回溯到頂端仍無 handler，標準規定直接 std::terminate()。
//     追問：那 t.join() 本身有可能拋例外嗎？
//         → 會，丟 std::system_error。實測：對 non-joinable thread join 得到
//           "Invalid argument"；在執行緒裡 join 自己得到 "Resource deadlock
//           avoided"。所以那個 catch 不是完全沒用，只是抓錯對象。
//
// 🔥 Q2. 要把子執行緒的例外帶回主執行緒，有哪些做法？
//     答：三種。(1) 執行緒內 catch(...) 後用 std::current_exception() 存成
//         std::exception_ptr，join 後 std::rethrow_exception()；
//         (2) std::promise::set_exception()，消費端 future.get() 時重拋；
//         (3) std::async —— 回傳的 future 自動搬運例外，最省事。
//         共同點：都是「先在執行緒內接住」，再用一個明確的通道搬運。
//     追問：std::exception_ptr 跨執行緒複製安全嗎？
//         → 安全。它是型別抹除的共享句柄，實作以 atomic refcount 管理例外
//           物件生命週期，可跨執行緒複製、儲存、重拋，且重拋會保留原始動態型別。
//
// ⚠️ 陷阱 1. 「程式 abort 了，所以這是未定義行為吧？」
//     答：不是。這是 [thread.thread.constr] 明文規定的行為，100% 確定會發生，
//         每次執行結果都一樣（terminate → SIGABRT → 退出碼 134）。
//     為什麼會錯：多數人腦中把「程式爆掉」和「UB」畫上等號。實際上 C++ 裡有
//         一整類【標準保證的中止】：joinable thread 解構、例外逃出 noexcept、
//         回溯期間解構子再拋。這些都是可預測的；真正的 UB（data race、
//         迭代器失效）反而常常「看起來跑得好好的」。分清楚這兩者，才能正確
//         判斷一段程式是「一定壞」還是「哪天會壞」。
//
// ⚠️ 陷阱 2. 「例外逃逸前，worker 裡的區域物件解構子總會先跑完吧？RAII 會保護我。」
//     答：不保證。[except.handle]/9 明訂「找不到 handler 時，是否先回溯堆疊
//         由實作定義」。本機實測 g++ 15.2.0 選擇【不回溯】：區域物件的解構子
//         完全沒有執行。
//     為什麼會錯：大家記得的是「例外會觸發 unwinding，RAII 保證釋放資源」，
//         卻漏了那個前提 —— 【要先找得到 handler】。Itanium ABI 的 Phase 1
//         若回報 _URC_END_OF_STACK，負責跑解構子的 Phase 2 根本不會啟動。
//         實務後果：檔案沒關、鎖沒放、暫存檔沒刪。這正是「別讓例外逃出執行緒」
//         的硬性理由，而不只是「程式會中止」而已。
// ═══════════════════════════════════════════════════════════════════════════
//
// 📌 本檔是刻意的錯誤示範：main() 保持原樣，不加任何 LeetCode／實務函式，
//    以維持示範的純粹性。正確作法請見同課 3、5、6 號檔案。

#include <iostream>
#include <thread>
#include <stdexcept>

void worker() {
    // 💀 例外從執行緒進入點逃逸 → std::terminate()
    throw std::runtime_error("執行緒內的錯誤！");
}

int main() {
    std::thread t(worker);

    try {
        t.join();
    } catch (const std::exception& e) {
        // 💀 這裡捕獲不到！程式在 worker() 拋出的當下就已經中止了。
        std::cout << "捕獲: " << e.what() << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.4：執行緒例外處理1.cpp" -o thread_exception_escape
// 執行: stdbuf -o0 ./thread_exception_escape; echo "exit=$?"
//       （用 stdbuf -o0 關掉輸出緩衝，否則 abort 不會 flush，死前的輸出會遺失）

// === 預期輸出 ===
// ⚠️ 本程式【一定會中止】，不會正常結束 —— 這正是本檔要示範的行為，不是 bug。
//    以下為實測結果（g++ 15.2.0 / libstdc++ / Ubuntu，每次執行都相同）：
//
// 以下兩行輸出到 stderr（不是 stdout）：
// terminate called after throwing an instance of 'std::runtime_error'
//   what():  執行緒內的錯誤！
//
// 接著程序被 SIGABRT 殺死，shell 顯示「中止 (核心已傾印)」：
// exit=134        ← 128 + 6(SIGABRT)
//
// 📌 注意 main() 裡那行 "捕獲: ..." 【永遠不會出現】。程式在 worker() 讓例外
//    逃逸的當下就已經 terminate，主執行緒的 catch 區塊根本沒有機會執行。
//
// 📌 "terminate called after throwing..." 這段文字是 libstdc++ 的
//    __verbose_terminate_handler 印的，屬於【實作定義】行為；
//    MSVC 等其他實作不會印出 what() 內容。但「呼叫 std::terminate 而中止」
//    這件事本身是【標準保證】的，跨平台一致。
