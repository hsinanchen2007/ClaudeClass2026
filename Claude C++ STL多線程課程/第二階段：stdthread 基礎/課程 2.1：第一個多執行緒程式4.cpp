// =============================================================================
//  課程 2.1：第一個多執行緒程式4.cpp  —  忘記 join()/detach() 的後果
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例:它一定會 abort,這就是本課重點】
//
// 【主題資訊 Information】
//   相關函式  : ~std::thread()、std::thread::joinable()、join()、detach()
//   標準版本  : C++11(std::thread);C++20 起有自動 join 的 std::jthread
//   標頭檔    : <thread>
//   標準依據  : [thread.thread.destr] —— 解構時若 joinable() 為 true,
//               呼叫 std::terminate()
//   實測結果  : exit code 134 (SIGABRT),stderr 印出
//               "terminate called without an active exception"
//
// ⚠️ 這【不是】未定義行為,而是 C++ 標準明確規定的行為:
//    std::thread 解構時若仍是 joinable(既沒 join 也沒 detach),
//    解構函式會呼叫 std::terminate() → 程式中止。
//    → 一定會發生,不是隨機、不是「可能」。因此本檔可以斬釘截鐵地描述結果。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼標準選擇「直接中止」這種極端做法】
// 當 std::thread 物件被解構、而它還關聯著一個執行中的執行緒時,
// 標準委員會面前有三個選項:
//   (a) 自動 join —— 等那個執行緒跑完。
//   (b) 自動 detach —— 放手讓它在背景繼續。
//   (c) 呼叫 std::terminate() —— 直接中止程式。
// 標準選了 (c),而且是刻意的:
//   * 若選 (a),解構子會變成「可能阻塞任意久」的操作。一個函式回傳、
//     一個區塊結束、甚至一次例外展開(stack unwinding),都可能突然卡住數分鐘。
//     解構子隱含地阻塞,是極難除錯的效能與死結來源。
//   * 若選 (b),那個執行緒會繼續存取它捕獲的區域變數 —— 而那些變數
//     正在被銷毀。這會把一個「忘記寫一行」的疏失,變成隨機的記憶體毀損,
//     症狀出現在完全無關的地方,幾乎不可能追查。
//   * 選 (c) 雖然粗暴,但它「立刻、確定、在出錯的那一行」失敗。
// 這是 C++ 的一貫哲學:與其讓錯誤悄悄地變成 UB,不如讓它大聲地當場死掉。
//
// 【2. 為什麼是 terminate 而不是丟例外】
// 解構子在 C++11 起預設是 noexcept 的。若解構子丟出例外,而當時正在
// 進行例外展開,程式一樣會 terminate。既然如此,標準乾脆直接規定
// 呼叫 std::terminate(),語意更明確。
//
// 【3. 這個錯誤在真實程式裡怎麼發生】
// 沒有人會像本檔一樣「明目張膽地忘記 join」。真實情況幾乎都是這樣:
//     std::thread t(work);
//     if (somethingWrong) return;     // ← 這裡提早返回,忘了 join
//     t.join();
// 或者更隱蔽的:
//     std::thread t(work);
//     doSomething();                  // ← 這裡丟出例外
//     t.join();                       // ← 永遠執行不到
// 例外展開時 t 被解構 → terminate。也就是說,只要 join() 和 thread 的建立
// 之間有「任何」可能提早離開作用域的路徑,就有這個風險。
// 正解是 RAII:讓解構子自動處理(見本檔的 ThreadGuard 與 std::jthread)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼「Working...」不一定會印出來
//   main 建立 t 之後立刻執行 return 0,而新執行緒還要經過作業系統排程
//   才會真正開始跑。兩者是賽跑:多數情況下 main 先跑到解構、觸發 terminate,
//   新執行緒還沒印出任何東西。本機實測(GCC 15.2.0)確實沒有印出 "Working..."。
//   但這是排程決定的,不是標準保證 —— 換台機器、加點負載,兩種結果都可能。
//   ⚠️ 注意區分:「Working... 有沒有印出」是不確定的,
//      但「程式一定會 abort」是標準保證的。別把兩者混為一談。
//
// (B) 收集這種程式的輸出時要用 stdbuf -o0
//   std::cout 預設有緩衝。程式被 abort 時,緩衝區裡還沒沖出去的內容
//   會直接消失 —— 這會讓你以為某些輸出「沒有執行到」,其實只是沒沖出來。
//   用 stdbuf -o0 關掉緩衝,才能看到真實的執行進度。
//   (本檔的預期輸出正是用 stdbuf -o0 收集的。)
//
// (C) std::jthread 的差異(C++20)
//   std::jthread 的解構子會先 request_stop() 再 join(),因此
//   「忘記 join」不再是錯誤 —— 它會自動等待並正常結束。
//   這是 C++20 對這個設計缺陷的正式修補。若你的專案能用 C++20,
//   預設就該用 jthread,把 std::thread 留給真的需要手動控制的場合。
//
// 【注意事項 Pay Attention】
// 1. 只要 thread 物件解構時 joinable() 為 true,就一定 terminate。
//    不管執行緒本身是否已經跑完 —— 跑完但沒 join,仍然是 joinable。
// 2. 例外展開路徑最容易漏掉 join;用 RAII 包裝才是根本解法。
// 3. detach() 不是「安全的 join 替代品」,它只是換一種風險
//    (執行緒可能存取已銷毀的資料,見課程 2.4)。
// 4. 這是標準規定的確定行為,不是未定義行為,可以放心斷言。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】忘記 join()/detach()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread 物件解構時仍是 joinable,會發生什麼事?
//     答：解構子會呼叫 std::terminate(),程式立刻中止。這是標準
//         [thread.thread.destr] 明確規定的行為,不是未定義行為,
//         所以每次都會發生。本機實測 exit code 134 (SIGABRT),
//         stderr 顯示 "terminate called without an active exception"。
//     追問：為什麼標準不乾脆自動 join 或自動 detach?
//         → 自動 join 會讓解構子變成可能阻塞任意久的操作,是隱形的死結來源;
//           自動 detach 會讓執行緒繼續存取正在被銷毀的變數,把疏失變成
//           難以追查的記憶體毀損。直接中止雖然粗暴,但錯誤會當場、明確地爆出來。
//
// 🔥 Q2. 這個錯誤在真實專案裡通常怎麼發生?
//     答：幾乎不會是「明目張膽地忘記寫」,而是建立 thread 之後、join 之前
//         有一條提早離開的路徑 —— 例如中間 return、break,或最常見的:
//         中間某個函式丟出例外,例外展開時 thread 被解構。
//     追問：怎麼根治?
//         → 用 RAII:寫一個解構子會檢查 joinable 並 join 的守衛類別,
//           或直接改用 C++20 的 std::jthread(解構時自動 request_stop + join)。
//
// ⚠️ 陷阱. 「執行緒的工作已經做完了,thread 物件解構時應該就沒事了吧?」
//     答：仍然會 terminate。joinable() 判斷的是「這個物件還關聯著一個
//         尚未被回收的執行緒」,而不是「那個執行緒還在不在跑」。
//         執行緒跑完了但沒有人 join 它,它的資源仍未被回收,
//         物件依然 joinable —— 解構照樣中止程式。
//     為什麼會錯：把 joinable() 理解成「還在執行中」。
//         正確的理解是「還沒被 join 也還沒被 detach」,
//         這是所有權狀態,和執行緒跑到哪裡完全無關。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <utility>

void work() {
    std::cout << "Working..." << std::endl;
}

// -----------------------------------------------------------------------------
// 正確作法示範 1:RAII 守衛 —— 解構時自動 join
//   這是 C++20 之前的標準解法,也是《C++ Concurrency in Action》書中的
//   thread_guard。重點在於:即使中途 return 或丟出例外,
//   守衛的解構子仍會在展開過程中被呼叫,join 因此不會被跳過。
// -----------------------------------------------------------------------------
class ThreadGuard {
    std::thread t_;
public:
    explicit ThreadGuard(std::thread t) : t_(std::move(t)) {}

    ~ThreadGuard() {
        if (t_.joinable()) {     // 一定要檢查:對 non-joinable 呼叫 join 會丟例外
            t_.join();
        }
    }

    // 守衛擁有執行緒的所有權,不可複製(否則會有兩個守衛想 join 同一個執行緒)
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

void safeWithGuard() {
    ThreadGuard guard{std::thread(work)};
    std::cout << "[守衛] 即使這裡提早 return 或丟例外,解構子也會 join" << std::endl;
}   // guard 解構 → 自動 join → 不會 terminate

// -----------------------------------------------------------------------------
// 正確作法示範 2:最單純的手動 join
// -----------------------------------------------------------------------------
void safeWithManualJoin() {
    std::thread t(work);
    t.join();
    std::cout << "[手動] join() 完成,t.joinable() 現在是 "
              << std::boolalpha << t.joinable() << std::endl;
}

int main() {
    std::cout << "=== 正確作法 1:RAII 守衛自動 join ===" << std::endl;
    safeWithGuard();

    std::cout << "\n=== 正確作法 2:手動 join ===" << std::endl;
    safeWithManualJoin();

    std::cout << "\n=== 錯誤示範:忘記 join()/detach() ===" << std::endl;
    std::cout << "(接下來程式會被 std::terminate() 中止 —— 這是標準規定的行為)"
              << std::endl;

    std::thread t(work);
    // 💀 忘記呼叫 join() 或 detach()
    return 0;  // 💀 t 解構時仍 joinable → std::terminate(),程式必定中止
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.1：第一個多執行緒程式4.cpp" -o forget_join
// 執行: stdbuf -o0 ./forget_join    ← 一定要關緩衝,否則中止時輸出會遺失

// 注意:以下為某一次實際執行的結果(以 stdbuf -o0 收集)。
//   * 程式必定以 exit code 134 (SIGABRT) 中止 —— 這是標準保證的,每次都一樣。
//   * 但最後那個執行緒的 "Working..." 有沒有來得及印出來,則取決於
//     作業系統排程,每次執行都可能不同。本機這次沒有印出;
//     在別的機器或不同負載下印出來也是完全合法的。
//     ⚠️ 別把「Working... 沒印出」當成保證,那部分是不確定的;
//        確定的只有「一定會中止」。

// === 預期輸出 ===
// === 正確作法 1:RAII 守衛自動 join ===
// [守衛] 即使這裡提早 return 或丟例外,解構子也會 join
// Working...
//
// === 正確作法 2:手動 join ===
// Working...
// [手動] join() 完成,t.joinable() 現在是 false
//
// === 錯誤示範:忘記 join()/detach() ===
// (接下來程式會被 std::terminate() 中止 —— 這是標準規定的行為)
// (以下為 stderr,程式中止時的訊息)
// [stderr] terminate called without an active exception
