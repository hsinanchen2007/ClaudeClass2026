// =============================================================================
//  課程 2.3：傳遞參數給執行緒5.cpp  —  decay-copy 的陷阱:懸空的 char*
// =============================================================================
//
// 【本檔含一個「真正的未定義行為(UB)」示範,預設不執行 —— 見下方說明】
//
// 【主題資訊 Information】
//   問題語法  : std::thread t(printStr, buffer);   // buffer 是 char[6]
//   標準版本  : C++11
//   標頭檔    : <thread>、<string>
//   關鍵規則  : std::thread 對每個參數做 decay-copy ——
//               陣列會退化成指標,「複製的是指標,不是內容」
//   錯誤分類  : 真正的未定義行為(存取已結束生命週期的物件),
//               不是「標準規定的中止」,因此結果無法預測、不可斷言
//
// 【詳細解釋 Explanation】
//
// 【1. 這個 bug 為什麼這麼隱蔽】
//     void printStr(const std::string& s);
//     char buffer[] = "Hello";
//     std::thread t(printStr, buffer);
//
// 看起來 printStr 收的是 std::string,那應該會複製一份字串吧?——不會。
// 實際發生的事情分成兩個階段:
//   (階段一,在原執行緒)std::thread 對參數做 decay-copy。
//                        char[6] 退化成 char*,存進執行緒的參數 tuple。
//                        **此時完全沒有建立任何 std::string。**
//   (階段二,在新執行緒)真正呼叫 printStr 時,才用那個 char*
//                        建構出臨時的 std::string。
//
// 問題就在兩個階段之間:如果 danger() 已經返回,buffer 這塊堆疊記憶體
// 早就歸還了,階段二讀取的是「已結束生命週期的物件」——未定義行為。
//
// 【2. 為什麼它特別難發現】
// 那塊堆疊記憶體通常還「看起來」是有效的(頁面仍映射、內容可能還沒被覆寫),
// 所以程式常常照常印出 "Hello",測試也就過了。
// 但它隨時可能因為時序、最佳化層級、堆疊使用狀況改變而變成亂碼、
// 空字串、甚至崩潰。本機實測就曾印出「空行」而不是 "Hello" ——
// 同一支程式、同一台機器。
//
// ⚠️ 這正是為什麼本檔「不把任何一次執行結果當成預期輸出」:
//    UB 沒有正確答案,任何觀察到的結果都不能被寫成「應該長這樣」。
//
// 【3. 修法有三種,理由各不相同】
//
//   (a) 在呼叫端就轉成 std::string(本檔 safeExplicit)
//         std::thread t(printStr, std::string(buffer));
//       階段一複製的就是一個完整的 std::string 物件(擁有自己的記憶體),
//       和 buffer 再也沒有關係。這是最直接的修法。
//
//   (b) 改用 join() 而不是 detach()(本檔 safeByJoin)
//         std::thread t(printStr, buffer);
//         t.join();
//       join() 保證執行緒在函式返回「之前」就跑完,buffer 那時還活著。
//       ⚠️ 但這個修法很脆弱 —— 它依賴「呼叫者永遠記得 join」這個約定,
//          任何人日後把它改成 detach,UB 就悄悄回來了。
//
//   (c) 一開始就別用 char[](最佳解)
//         std::string buffer = "Hello";
//         std::thread t(printStr, buffer);      // 複製整個 string
//       用擁有自己記憶體的型別,decay-copy 複製的就是完整的值。
//       這也是為什麼現代 C++ 建議少用 C 風格字串陣列。
//
// 【4. 判斷準則:複製之後還剩什麼】
// 這是本課三個範例檔(3、4、5)共同的主線。std::thread 一律「複製參數」,
// 所以唯一要問的問題是:**複製之後,得到的東西還依賴原本那個物件嗎?**
//   * 複製 int / std::string → 完全獨立,安全。
//   * 複製 int* / char*      → 仍指向原物件,原物件死了就懸空。
//   * 複製 reference_wrapper → 同上,仍指向原物件。
//   * 複製 shared_ptr        → 參考計數 +1,原物件保證不死,安全。
//
// 【概念補充 Concept Deep Dive】
//
// (A) decay-copy 的正式定義
//   標準規定 thread 對參數執行 decay-copy:先做 std::decay
//   (陣列 → 指標、函式 → 函式指標、去除 cv 與參考修飾),再複製。
//   關鍵在於「decay 發生在複製之前」——所以陣列在被複製時,
//   它已經不是陣列了,只是一個指標。
//
// (B) 為什麼標準不乾脆依照函式的參數型別去轉換
//   因為 std::thread 的建構子是模板,它在建立執行緒的當下並不知道
//   (也不該知道)目標函式的參數型別 —— 目標可能是多載的函式、
//   泛型 lambda、或吃可變參數的東西。統一用 decay-copy 是唯一
//   一致且可預測的規則,代價就是使用者必須自己理解它。
//
// (C) 為什麼 detach 讓一切變糟
//   join() 把執行緒的生命週期綁回呼叫端的作用域,所以區域變數天然安全;
//   detach() 則完全解除這個綁定,執行緒可以活得比任何區域變數都久。
//   準則:一旦決定 detach,執行緒就不可以碰任何「呼叫端作用域」的東西 ——
//   所有需要的資料都必須以「值」或 shared_ptr 的形式完整帶走。
//
// 【注意事項 Pay Attention】
// 1. 這是真正的未定義行為,不是標準規定的中止。不可以斷言它會印什麼、
//    也不可以斷言它一定會崩潰。「測起來正常」完全不代表它是對的。
// 2. 傳字面值 "Hello" 給執行緒是安全的 —— 字串字面值有靜態儲存期,
//    活到程式結束。危險的只有「區域的 char 陣列」。
// 3. detach 的執行緒不得存取呼叫端的區域變數,包含以指標/參考捕獲的。
// 4. 用 join 來修這個 bug 只是把風險藏起來,不是根治。
// 5. 本檔的 UB 示範預設「不執行」,要親眼看它必須自己加上 -DDEMONSTRATE_UB
//    重新編譯(見檔尾編譯指令)。這樣可以確保一般編譯出來的程式是乾淨、
//    可重現的。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】decay-copy 與懸空指標
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void printStr(const std::string&); 搭配 char buffer[] = "Hello";
//        寫成 std::thread t(printStr, buffer); 有什麼問題?
//     答：std::thread 對參數做 decay-copy,char[6] 會先退化成 char*,
//         複製的是那個「指標」而不是字串內容;真正建構 std::string
//         是在新執行緒裡呼叫 printStr 的那一刻才發生。
//         若這中間 buffer 所在的函式已經返回,就是在讀取已結束生命週期
//         的記憶體 —— 未定義行為。
//     追問：那為什麼我測起來都正常印出 Hello?
//         → 因為那塊堆疊記憶體通常還沒被覆寫,看起來「還是對的」。
//           這是 UB 最危險的形式:平常都對,某天時序一變就出錯。
//           本機實測就曾印出空行而不是 Hello。
//
// 🔥 Q2. 這個問題有哪些修法?各自的優劣?
//     答：三種。(a) 在呼叫端就轉成 std::string,讓複製的是完整物件 ——
//         最直接;(b) 把 detach 改成 join,讓執行緒在函式返回前跑完 ——
//         有效但脆弱,依賴呼叫者永遠記得 join;(c) 根本不要用 char[],
//         一開始就用 std::string —— 最佳解。
//     追問：傳字串字面值 "Hello" 也有這個問題嗎?
//         → 沒有。字串字面值有靜態儲存期,活到程式結束,
//           所以那個 const char* 永遠有效。危險的只有「區域的 char 陣列」。
//
// ⚠️ 陷阱. 「printStr 的參數是 const std::string&,傳進去時就會自動
//         建構一個 std::string 副本,所以是安全的。」哪裡錯了?
//     答：錯在「什麼時候建構」。std::string 確實會被建構,
//         但那是在「新執行緒實際呼叫 printStr 的那一刻」才發生,
//         而不是在你建立 std::thread 的那一行。
//         std::thread 那一行只做了 decay-copy —— 存下一個 char*。
//         這兩個時間點之間,原本的 buffer 完全可能已經消失。
//     為什麼會錯：用「呼叫一般函式」的直覺去想 —— 平常寫 printStr(buffer)
//         時,轉換確實立刻發生在呼叫點,所以絕對安全。
//         但 std::thread 是「先儲存參數、稍後在別的執行緒才呼叫」,
//         它把「準備參數」和「使用參數」硬生生拆成了兩個時刻,
//         中間隔著一整段不確定的時間 —— 所有生命週期問題都由此而生。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

void printStr(const std::string& s) {
    say("    收到字串: [" + s + "]");
}

#ifdef DEMONSTRATE_UB
// -----------------------------------------------------------------------------
// ⚠️ 未定義行為示範 —— 預設「不編譯、不執行」
//    要親眼看它,請加上 -DDEMONSTRATE_UB 重新編譯(見檔尾)。
//    這裡刻意隔離起來,是因為它的輸出本質上不可預測:
//    可能印出 Hello、可能是空字串、可能是亂碼,也可能崩潰。
//    把不可預測的結果寫進「預期輸出」是錯誤的示範,所以預設關閉。
// -----------------------------------------------------------------------------
void dangerousDetach() {
    char buffer[] = "Hello";

    // 危險!thread 只複製了 char*,std::string 要到新執行緒裡才建構,
    // 而那時 buffer 可能已經隨著本函式返回而消失。
    std::thread t(printStr, buffer);
    t.detach();
}   // buffer 在這裡被銷毀 —— 但執行緒可能還沒開始跑
#endif

// -----------------------------------------------------------------------------
// 修法 (a):在呼叫端就明確轉成 std::string
//   複製進執行緒的是一個擁有自己記憶體的完整字串,和 buffer 徹底脫鉤。
// -----------------------------------------------------------------------------
void safeExplicit() {
    char buffer[] = "Hello";

    std::thread t(printStr, std::string(buffer));   // ✅ 階段一就複製了完整的值
    t.join();
}

// -----------------------------------------------------------------------------
// 修法 (b):改用 join(),讓執行緒在 buffer 消失前就跑完
//   ⚠️ 有效但脆弱:它依賴「呼叫者永遠記得 join」這個約定,
//      任何人日後改成 detach,UB 就無聲無息地回來了。
// -----------------------------------------------------------------------------
void safeByJoin() {
    char buffer[] = "World";

    std::thread t(printStr, buffer);   // 仍然只複製了 char*……
    t.join();                          // ……但 join 保證它在 buffer 死之前用完
}

// -----------------------------------------------------------------------------
// 修法 (c):一開始就不要用 char[](最佳解)
// -----------------------------------------------------------------------------
void safeBestPractice() {
    std::string buffer = "Modern";     // 擁有自己的記憶體

    std::thread t(printStr, buffer);   // 複製的是完整的 std::string
    t.detach();                        // 即使 detach 也安全
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】背景送出通知:detach 的任務必須「完整帶走」所有資料
//   情境: 使用者下單後,主流程要立刻回應,通知信改到背景送。
//         背景任務會比 placeOrder() 活得久,所以它需要的每一項資料
//         都必須以「值」的形式完整複製進去 —— 不能有任何一個欄位
//         還指向 placeOrder() 的區域變數。
//   為什麼用本主題: 這是【概念補充 C】的準則在真實系統中的樣子,
//                   也是 detach 唯一安全的用法。
// -----------------------------------------------------------------------------
struct Notification {
    std::string to;        // 全部用擁有記憶體的型別,不用 const char*
    std::string subject;
    std::string body;
};

void sendNotification(Notification n) {   // 按值接收 —— 執行緒擁有自己的一份
    say("    [通知] 寄給 " + n.to + " 主旨「" + n.subject + "」");
}

void placeOrder(const std::string& user, int orderId) {
    char legacyTemplate[] = "您的訂單已成立";   // 假設來自舊有的 C API

    Notification n;
    n.to      = user;
    n.subject = "訂單 #" + std::to_string(orderId);
    // ✅ 在這裡就把 char[] 轉成 std::string,而不是把 legacyTemplate 帶進執行緒
    n.body    = std::string(legacyTemplate);

    std::thread t(sendNotification, std::move(n));   // 整包移動進執行緒
    t.detach();                                      // 安全:n 的內容已完整轉移
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔的主題是「物件生命週期與未定義行為」,LeetCode 不會、也無法考這個:
//   它的評測框架只檢查輸出是否正確,而 UB 的可怕之處正是「常常輸出也是對的」。
//   並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   全部由框架建立執行緒並保證物件存活,參數傳遞的坑根本不會出現。
//   本課第 7 個範例檔會示範真正對應得上的 1116,此處從缺。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 修法 (a):呼叫端明確轉成 std::string ===" << std::endl;
    safeExplicit();

    std::cout << "\n=== 修法 (b):改用 join(),讓執行緒在 buffer 死前跑完 ===" << std::endl;
    safeByJoin();
    std::cout << "  (有效,但依賴「永遠記得 join」的約定,脆弱)" << std::endl;

    std::cout << "\n=== 修法 (c):一開始就用 std::string(最佳解) ===" << std::endl;
    safeBestPractice();

    std::cout << "\n=== 實務:detach 的背景通知,資料完整帶走 ===" << std::endl;
    placeOrder("alice@example.com", 1001);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::cout << "\n=== 未定義行為示範 ===" << std::endl;
#ifdef DEMONSTRATE_UB
    std::cout << "  (已啟用 -DDEMONSTRATE_UB,接下來的輸出無法預測)" << std::endl;
    dangerousDetach();
    std::this_thread::sleep_for(std::chrono::seconds(1));
#else
    std::cout << "  已跳過(預設關閉)。" << std::endl;
    std::cout << "  該示範會讀取已結束生命週期的堆疊記憶體,結果無法預測 ——" << std::endl;
    std::cout << "  可能印出 Hello、可能是空字串、可能是亂碼,也可能崩潰。" << std::endl;
    std::cout << "  正因為沒有正確答案,它不適合被寫進「預期輸出」。" << std::endl;
    std::cout << "  要親眼看它:g++ -std=c++17 -Wall -Wextra -pthread"
                 " -DDEMONSTRATE_UB 本檔 -o ub_demo" << std::endl;
#endif

    return 0;
}

// 編譯(預設,安全且輸出可重現):
//   g++ -std=c++17 -Wall -Wextra -pthread "課程 2.3：傳遞參數給執行緒5.cpp" -o pass_str
//
// 編譯(啟用 UB 示範,輸出不可預測,僅供實驗):
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB "課程 2.3：傳遞參數給執行緒5.cpp" -o ub_demo
//
// 進階驗證:用 AddressSanitizer 可以讓這個 UB 現形(它會明確報出
//   stack-use-after-return,而不是安靜地印出看似正確的結果):
//   g++ -std=c++17 -fsanitize=address -g -pthread -DDEMONSTRATE_UB 本檔 -o ub_asan

// 注意:以下為「預設編譯」(未啟用 -DDEMONSTRATE_UB)的實際執行結果,
//       每次執行都相同。若你自行加上 -DDEMONSTRATE_UB,最後一段的輸出
//       將無法預測 —— 那正是未定義行為的定義,不要把任何一次的結果
//       當成「正確答案」。

// === 預期輸出 ===
// === 修法 (a):呼叫端明確轉成 std::string ===
//     收到字串: [Hello]
//
// === 修法 (b):改用 join(),讓執行緒在 buffer 死前跑完 ===
//     收到字串: [World]
//   (有效,但依賴「永遠記得 join」的約定,脆弱)
//
// === 修法 (c):一開始就用 std::string(最佳解) ===
//     收到字串: [Modern]
//
// === 實務:detach 的背景通知,資料完整帶走 ===
//     [通知] 寄給 alice@example.com 主旨「訂單 #1001」
//
// === 未定義行為示範 ===
//   已跳過(預設關閉)。
//   該示範會讀取已結束生命週期的堆疊記憶體,結果無法預測 ——
//   可能印出 Hello、可能是空字串、可能是亂碼,也可能崩潰。
//   正因為沒有正確答案,它不適合被寫進「預期輸出」。
//   要親眼看它:g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB 本檔 -o ub_demo
