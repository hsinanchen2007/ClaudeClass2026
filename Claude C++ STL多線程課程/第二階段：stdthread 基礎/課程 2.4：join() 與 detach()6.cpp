// =============================================================================
//  課程 2.4：join() 與 detach()6.cpp  —  對同一個執行緒呼叫兩次 join()
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例:它一定會 abort,這就是本課重點】
//
// 【主題資訊 Information】
//   相關函式  : std::thread::join()、joinable()
//   標準版本  : C++11
//   標頭檔    : <thread>;std::system_error 在 <system_error>
//   標準依據  : join() 的前置條件是 joinable() == true;
//               不滿足時丟出 std::system_error(錯誤碼 invalid_argument)
//   實測結果  : exit code 134 (SIGABRT),stderr 印出
//               "terminate called after throwing an instance of 'std::system_error'"
//               "  what():  Invalid argument"
//
// ⚠️ 這【不是】未定義行為,而是標準規定的行為:
//    第一次 join() 之後,t 已不再 joinable();
//    對 non-joinable 的 thread 再呼叫 join(),會拋出 std::system_error。
//    本例沒有 try/catch,例外未被接住 → std::terminate() → 程式中止。
//    → 一定會發生,不是隨機。因此本檔可以斬釘截鐵地描述結果。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼第二次 join() 會失敗】
// join() 的語意是「回收這條執行緒的所有權」。第一次呼叫之後,
// 所有權已經交還,thread 物件變成一個不關聯任何執行緒的空殼
// (joinable() == false)。第二次呼叫等於要求回收一個你已經沒有的東西 ——
// 這在邏輯上就是錯的,標準因此規定丟出例外。
//
// 值得注意的是:失敗的原因「不是」執行緒還在跑或已經結束,
// 而純粹是「這個物件已經沒有東西可以 join 了」。
// 這再次呼應課程 2.4/4 的重點:joinable() 描述的是所有權,不是執行狀態。
//
// 【2. 為什麼沒接住的例外會導致程式中止】
// 這一步比第一步更值得理解。標準規定:
// **例外不能跨越執行緒邊界傳播。**
// 一般函式丟出例外時,它會沿著呼叫堆疊往上找 handler;
// 但執行緒的進入點函式沒有「上一層」——它就是那條執行緒的堆疊底部。
// 標準對這種情況的規定很直接:呼叫 std::terminate()。
//
// 本例的 join() 是在 main() 裡呼叫的,例外會沿著 main 往上,
// 但 main 之外也沒有 handler,同樣以 std::terminate() 收場。
// 兩種情形的結局相同:程式立刻中止,exit code 134。
//
// 【3. 三種正確的處理方式】
//
//   (a) 根本不要呼叫第二次(最好)
//       如果你的程式邏輯清楚,根本不會發生重複 join。
//       出現重複呼叫,通常代表所有權設計不清楚 —— 那才是要修的地方。
//
//   (b) 用 joinable() 先檢查(見本課第 5 個範例檔)
//           if (t.joinable()) { t.join(); }
//       這是 RAII 守衛類別的標準寫法,讓解構子可以無腦呼叫。
//
//   (c) 用 try/catch 接住(最少用)
//       能防止中止,但它只是把症狀壓下來 ——
//       會走到這裡通常代表邏輯已經出錯了,靜默吞掉反而更難除錯。
//
// 【4. 這個錯誤在真實程式裡怎麼發生】
// 沒有人會像本檔一樣連寫兩行 t.join()。真實情況通常是:
//   * 手動 join 之後,RAII 守衛的解構子又 join 一次
//     (所以守衛一定要用 (b) 的檢查寫法);
//   * 迴圈或錯誤處理路徑重複執行了同一段清理程式碼;
//   * 兩條執行緒同時管理同一個 thread 物件(TOCTOU,見第 5 個範例檔)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼是 std::system_error 而不是 std::logic_error
//   std::system_error 用來表示「來自作業系統或底層系統設施的錯誤」,
//   它帶有一個 std::error_code。這裡的錯誤碼是 invalid_argument,
//   對應底層 pthread_join 回傳的 EINVAL。
//   選擇這個型別,是為了讓使用者能透過 e.code() 取得精確的原因,
//   而不只是一段字串訊息。
//
// (B) 為什麼標準不讓第二次 join() 靜默成功
//   靜默成功會掩蓋一個真正的邏輯錯誤:你以為手上還有一條執行緒要等,
//   實際上並沒有。在複雜的程式裡,這種誤解會導致「你以為已經等到工作完成,
//   其實根本沒等」——那是比崩潰更難查的 bug。及早失敗是正確的選擇。
//
// (C) 收集這種程式的輸出要用 stdbuf -o0
//   std::cout 預設有緩衝,程式被 abort 時緩衝區內尚未沖出的內容會直接消失,
//   讓你誤判執行進度。用 stdbuf -o0 關掉緩衝才看得到真實情況。
//   (本檔的預期輸出正是這樣收集的。)
//
// 【注意事項 Pay Attention】
// 1. join() 只能成功呼叫一次;第二次丟 std::system_error。
// 2. 例外無法跨執行緒傳播;逸出執行緒函式即 std::terminate()。
// 3. 想把執行緒中的例外帶回主執行緒,要用 std::promise/std::future
//    或 std::exception_ptr。
// 4. detach() 也一樣:對 non-joinable 的 thread 呼叫會丟同樣的例外。
// 5. 這是標準規定的確定行為,不是未定義行為,可以放心斷言。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】重複 join() 與例外的邊界
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 對同一個 std::thread 呼叫兩次 join() 會怎樣?
//     答：第二次會丟出 std::system_error(錯誤碼 invalid_argument),
//         因為 join() 的前置條件是 joinable() == true,而第一次呼叫後
//         所有權已交還,物件變成不關聯任何執行緒的空殼。
//         本例沒有接住例外,因此以 std::terminate() 收場,
//         實測 exit code 134。
//     追問：那要怎麼安全地寫?
//         → if (t.joinable()) { t.join(); }。這也是 RAII 守衛類別
//           解構子的標準寫法,讓它能容忍使用者已經手動 join 過。
//
// 🔥 Q2. 為什麼執行緒函式裡沒接住的例外會讓整個程式中止?
//     答：因為例外不能跨越執行緒邊界傳播。執行緒的進入點函式就是那條
//         執行緒堆疊的底部,沒有「上一層」可以往上找 handler,
//         標準因此規定直接呼叫 std::terminate()。
//     追問：那怎麼把執行緒裡的例外帶回主執行緒?
//         → 用 std::promise/std::future(future.get() 會重新丟出那個例外),
//           或自己用 std::exception_ptr 搭配 std::current_exception()
//           捕捉、再到主執行緒用 std::rethrow_exception() 丟出。
//           std::async 也內建這個機制。
//
// ⚠️ 陷阱. 「第一次 join() 已經等到執行緒結束了,第二次 join() 沒有東西
//         可以等,那它應該直接返回就好,為什麼要丟例外?」
//     答：因為「沒有東西可等」正是一個需要被告知的錯誤狀態。
//         標準的立場是:你呼叫 join(),代表你相信手上有一條執行緒要等。
//         如果實際上沒有,那你對程式狀態的認知已經和事實不符了 ——
//         靜默返回會讓這個誤解繼續存在,最後變成「你以為已經等到工作完成,
//         其實從來沒等過」這種極難追查的 bug。
//     為什麼會錯：從「這次呼叫本身無害」的角度看問題,而不是從
//         「為什麼會走到這裡」的角度看。防禦性 API 設計的原則是:
//         當呼叫者的假設與現實不符時,及早、大聲地失敗,
//         而不是體貼地幫他把錯誤藏起來。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <system_error>
#include <thread>

int main() {
    std::cout << "=== 正確作法 1:用 joinable() 檢查 ===" << std::endl;
    {
        std::thread t([]() {});

        if (t.joinable()) {
            t.join();
            std::cout << "  第一次:已 join" << std::endl;
        }
        if (t.joinable()) {
            t.join();
        } else {
            std::cout << "  第二次:joinable 為 false,安全地跳過" << std::endl;
        }
    }

    std::cout << "\n=== 正確作法 2:用 try/catch 接住(不建議當常規手段) ===" << std::endl;
    {
        std::thread t([]() {});
        t.join();

        try {
            t.join();   // 第二次 → 丟 std::system_error
        } catch (const std::system_error& e) {
            std::cout << "  接住了 std::system_error" << std::endl;
            std::cout << "    what()      : " << e.what() << std::endl;
            std::cout << "    code()      : " << e.code() << std::endl;
            std::cout << "    code().value(): " << e.code().value() << std::endl;
        }
        std::cout << "  ↑ 程式沒有中止 —— 但會走到這裡通常代表邏輯已經出錯了"
                  << std::endl;
    }

    std::cout << "\n=== 錯誤示範:沒有接住例外 ===" << std::endl;
    std::cout << "(接下來程式會被 std::terminate() 中止 —— 這是標準規定的行為)"
              << std::endl;

    std::thread t([]() {});

    t.join();
    t.join();  // 💀 t 已不是 joinable → 拋出 std::system_error → terminate()

    std::cout << "這一行永遠不會被執行" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.4：join() 與 detach()6.cpp" -o double_join
// 執行: stdbuf -o0 ./double_join    ← 一定要關緩衝,否則中止時輸出會遺失

// 注意:以下為某一次實際執行的結果(以 stdbuf -o0 收集)。
//   本檔的輸出每次執行都相同,而且程式必定以 exit code 134 (SIGABRT) 中止 ——
//   這是標準規定的確定行為,不是未定義行為,也不是隨機的。

// === 預期輸出 ===
// === 正確作法 1:用 joinable() 檢查 ===
//   第一次:已 join
//   第二次:joinable 為 false,安全地跳過
//
// === 正確作法 2:用 try/catch 接住(不建議當常規手段) ===
//   接住了 std::system_error
//     what()      : Invalid argument
//     code()      : generic:22
//     code().value(): 22
//   ↑ 程式沒有中止 —— 但會走到這裡通常代表邏輯已經出錯了
//
// === 錯誤示範:沒有接住例外 ===
// (接下來程式會被 std::terminate() 中止 —— 這是標準規定的行為)
// (以下為 stderr,程式中止時的訊息)
// [stderr] terminate called after throwing an instance of 'std::system_error'
// [stderr]   what():  Invalid argument
