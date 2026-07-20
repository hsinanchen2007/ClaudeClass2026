// =============================================================================
//  課程 2.1：第一個多執行緒程式2.cpp  —  join() 的阻塞語意：誰在等誰
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：  #include <thread>（std::thread、std::this_thread::sleep_for）
//             #include <chrono>（std::chrono::seconds —— 見【注意事項】1）
//   標準版本：C++11
//   核心簽名：
//     void join();                                  // 阻塞「呼叫端」直到目標執行緒結束
//     template<class Rep, class Period>
//     void sleep_for(const chrono::duration<Rep, Period>& d);  // 至少睡 d，可能更久
//   複雜度／成本：
//     join() 阻塞期間不佔 CPU（Linux 上是 futex 等待，執行緒被移出 run queue）
//     喚醒延遲 ~1–3 µs；sleep_for 的實際睡眠時間 ≥ 指定值（受排程精度影響）
//   編譯：g++ -std=c++17 -Wall -Wextra -pthread
//
// 【詳細解釋 Explanation】
//
// 【1. join() 阻塞的是「呼叫者」，不是「被 join 的執行緒」】
//   這是初學者最常見的方向性誤解。`t.join()` 寫在 main 裡，被卡住的是 **main**，
//   t 完全不受影響，它照自己的步調跑完 sleep 與輸出。
//   把它讀成一句白話會清楚很多：「main 說：我在這裡等你 t 做完再繼續。」
//   所以 join() 不會「加速」「催促」或「中斷」目標執行緒，它純粹是等待。
//
// 【2. 阻塞不等於空轉 —— join() 期間 CPU 是釋放出來的】
//   join() 不是 busy-wait（while 迴圈猛問「你好了沒」）。在 Linux 上它最終走到
//   futex 系統呼叫：main 這個 task 被標記為 blocked、移出 run queue，
//   排程器把 CPU 讓給別人；等 t 結束時核心才把 main 喚醒。
//   因此本檔跑完約 1 秒，但這 1 秒幾乎不消耗 CPU 時間——
//   用 `time ./prog` 觀察會看到 real ≈ 1.0s，而 user + sys ≈ 0.00s。
//   這正是 wall-clock time 與 CPU time 的差別，也是「I/O 密集型工作適合開執行緒」
//   的根本原因：等待的執行緒不吃核心。
//
// 【3. 哪些順序是「保證」的，哪些只是「幾乎總是」】
//   實測輸出為：
//       Waiting...        ← (a)
//       Work done!        ← (b)
//       Thread finished   ← (c)
//   但這三行的保證強度**並不相同**，這是本檔最值得學的一點：
//     • (b) 一定在 (c) 之前 —— **有保證**。因為 (c) 寫在 t.join() 之後，
//       join() 建立 happens-before，work() 的所有動作都排在它前面。
//     • (a) 在 (b) 之前 —— **沒有保證**，只是機率上幾乎必然。
//       main 印 "Waiting..." 只需要幾微秒，而 work() 要先睡滿 1 秒；
//       但兩者之間沒有任何同步關係，若 OS 剛好把 main 換出超過 1 秒
//       （極端負載、單核心、被 SIGSTOP 等），順序就會顛倒。
//   結論：**sleep 不是同步原語**。用 sleep 製造出來的順序是「賭排程器」，
//   不是「保證」。真正要保證順序，得用 join()、mutex、condition_variable
//   或 atomic 建立 happens-before。
//
// 【概念補充 Concept Deep Dive】
//   (A) sleep_for 的精度是實作定義的
//       標準只保證「至少睡這麼久」（may be longer），從不保證準時醒。
//       實際延遲取決於核心的 timer 解析度與排程延遲：一般 Linux 桌面
//       約有幾十微秒到幾毫秒的誤差，負載重時更久。
//       另外 sleep_for 使用 steady_clock 語意，不受系統時間被調整（NTP、
//       手動改時鐘）影響；這也是週期任務不該用 wall clock 的原因。
//
//   (B) 為什麼 sleep_for 需要 <chrono> 而不是秒數
//       `sleep_for(1)` 編譯不過——參數型別是 std::chrono::duration，
//       強型別讓「1 到底是秒、毫秒還是奈秒」不可能寫錯，這是 C++11
//       chrono 函式庫刻意的設計：單位是型別系統的一部分。
//       C++14 起可用字面值：`using namespace std::chrono_literals; sleep_for(1s);`
//
//   (C) join() 期間的執行緒狀態
//       用 `ps -eLo pid,tid,stat,comm` 觀察，join 中的 main 會處於 S
//       （interruptible sleep）狀態，而不是 R（running）。這證實了它沒有空轉。
//       對照組是 spin lock：那才會顯示 R 並吃滿一個核心。
//
// 【注意事項 Pay Attention】
//   1. 本檔使用 std::chrono::seconds，嚴格說應該 #include <chrono>。原始碼能編過，
//      是因為 libstdc++ 的 <thread> 傳遞性地包含了 <chrono> —— 這是**實作細節，
//      不是標準保證**，換一套標準函式庫就可能編譯失敗。本檔已補上該 include，
//      這也是 IWYU（Include What You Use）原則的實例。
//   2. join() 只能呼叫一次；對 joinable() == false 的物件再 join 會拋
//      std::system_error。習慣寫法是 `if (t.joinable()) t.join();`。
//   3. join() 沒有 timeout 版本。標準不提供 try_join_for，想要「等最多 N 秒」
//      必須改用 std::future/std::async 的 wait_for，或自己用 condition_variable。
//   4. 若 work() 內部拋出未捕捉的例外，例外**不會**傳播到 join() 的呼叫端，
//      而是直接呼叫 std::terminate()。要跨執行緒傳遞例外，需用 std::promise
//      /std::packaged_task/std::async（其 future 會保存 exception_ptr）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】join() 的阻塞語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. t.join() 阻塞的是哪一條執行緒？阻塞期間會吃 CPU 嗎？
//     答：阻塞的是**呼叫 join() 的那條執行緒**（本檔是 main），被 join 的 t
//         不受任何影響。阻塞期間不吃 CPU：Linux 上走 futex，呼叫者被移出
//         run queue，直到目標結束才被喚醒。用 time 量會看到 real ≈ 1s 而
//         user+sys ≈ 0。
//     追問：那如果想「等最多 2 秒」怎麼辦？→ std::thread::join() 沒有 timeout
//           版本，要改用 std::async 取得 future 後呼叫 wait_for()，或自己用
//           condition_variable::wait_for 搭配旗標。
//
// 🔥 Q2. 執行緒函式裡拋出的例外，可以在 join() 的地方 catch 嗎？
//     答：不行。例外若在執行緒進入點函式中未被捕捉，會直接呼叫 std::terminate()，
//         程式當場中止，join() 根本等不到。要跨執行緒傳遞例外，必須用
//         std::promise / std::packaged_task / std::async —— future::get() 會把
//         儲存的 exception_ptr 重新拋出。
//     追問：那正確寫法是什麼？→ 在執行緒函式最外層 try/catch，把例外存進
//           std::exception_ptr（std::current_exception()），主執行緒再 rethrow。
//
// ⚠️ 陷阱. 「Waiting...」一定會比「Work done!」先印出來嗎？
//     答：**沒有保證**，只是機率上幾乎必然。work() 要先睡 1 秒，main 印一行只要
//         幾微秒，所以實務上總是看到 Waiting... 在前；但兩者之間沒有任何同步
//         關係，理論上 main 若被排程器換出超過 1 秒，順序就會顛倒。
//         相對地，「Work done!」一定在「Thread finished」之前——**這個有保證**，
//         因為後者寫在 join() 之後。
//     為什麼會錯：把 sleep_for 當成同步工具。sleep 只是「賭對方來不及」，
//         它不建立 happens-before。真實系統中（虛擬機超賣、單核心、CPU 節流）
//         這種賭注會輸，而且是難以重現的偶發 bug。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <chrono>   // std::chrono::seconds（勿依賴 <thread> 的傳遞性引入）

void work() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Work done!" << std::endl;
}

int main() {
    std::thread t(work);
    std::cout << "Waiting..." << std::endl;
    t.join();  // 阻塞，直到 t 完成
    std::cout << "Thread finished" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread 課程\ 2.1：第一個多執行緒程式2.cpp -o join_demo

// === 預期輸出 ===
// Waiting...
// Work done!
// Thread finished
//
// 程式總耗時約 1 秒（work() 的 sleep_for(1s)）。
// 本機實測 `time` 結果：real 0m1.002s / user 0m0.000s / sys 0m0.002s
//   → 證明 join() 阻塞期間**不吃 CPU**（wall-clock 1 秒，CPU 時間趨近 0）。
//   ⚠️ 上面的耗時數字每次執行都略有不同（受排程延遲與 timer 精度影響）。
//
// 順序保證的強度並不一致（見【詳細解釋 3.】）：
//   • 「Work done!」在「Thread finished」之前 —— **有保證**（join() 的 happens-before）。
//   • 「Waiting...」在「Work done!」之前 —— **沒有保證**，只是機率上幾乎必然
//     （main 印一行只要幾微秒，work() 要先睡 1 秒；但 sleep 不是同步原語）。
// 實測 exit code = 0。
