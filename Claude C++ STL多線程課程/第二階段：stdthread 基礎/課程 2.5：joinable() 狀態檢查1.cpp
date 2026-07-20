// =============================================================================
//  課程 2.5：joinable() 狀態檢查1.cpp  —  四種讓 joinable() 變成 false 的情況
// =============================================================================
//
// 【主題資訊 Information】
//   簽名      : bool joinable() const noexcept;
//   標準版本  : C++11
//   標頭檔    : <thread>;std::move 在 <utility>
//   語意      : 這個 thread 物件目前是否關聯著一條「尚未被回收」的執行緒
//   四種 false: ① 預設建構 ② 已 join ③ 已 detach ④ 被 move 走
//
// 【詳細解釋 Explanation】
//
// 【1. 四種情況的共同本質】
// 表面上是四種不同的操作,但它們指向同一件事:
// **這個物件目前沒有握著任何執行緒的所有權,你不欠它 join 也不欠它 detach。**
//
//   ① 預設建構  std::thread t;
//      從來就沒有關聯過任何執行緒 —— 一個空的 handle。
//
//   ② 已 join   t.join();
//      所有權已交還:執行緒結束、資源已回收。
//
//   ③ 已 detach t.detach();
//      所有權已放棄:執行緒可能還在跑,但不再歸你管,
//      將由執行期系統在它結束時自動回收。
//
//   ④ 被 move 走 std::thread t5 = std::move(t4);
//      所有權轉移給了別人。t4 變成空殼,t5 接手 ——
//      注意這一組是「一個變 false、另一個變 true」,總數守恆。
//
// 【2. 為什麼 std::thread 只能移動、不能複製】
// std::thread 的複製建構子與複製賦值都被刪除了。理由是所有權:
// 一條作業系統執行緒只能被 join 一次、被回收一次。
// 若能複製出兩個 thread 物件指向同一條執行緒,兩者都會嘗試 join 它 ——
// 第二次必定失敗(丟 std::system_error),解構時也會出問題。
// 標準直接在型別層面禁止,讓這種錯誤根本寫不出來。
//
// 這使 std::thread 成為典型的 move-only 型別,和 std::unique_ptr 同一家族:
// 兩者都代表「獨佔的資源所有權」。
//
// 【3. ⚠️ move assignment 的致命細節】
// 這是本檔最需要記住的一點,也是最容易出事的地方:
//
//     std::thread a(work1);
//     std::thread b(work2);
//     a = std::move(b);        // 💀 a 原本還 joinable → std::terminate()!
//
// 對一個「仍然 joinable」的 std::thread 做 move assignment,
// 標準規定會呼叫 std::terminate()。原因和「忘記 join」完全一樣:
// a 原本握著的那條執行緒即將失去唯一的 handle,變成沒有人能回收的孤兒。
// 標準不願意默默地幫你 join(可能阻塞很久)或 detach(可能造成懸空),
// 所以選擇當場中止。
//
// 正確寫法是先把原本的處理掉:
//     if (a.joinable()) a.join();
//     a = std::move(b);
//
// ⚠️ 注意 C++20 的 std::jthread 規則「不同」:
//    對仍 joinable 的 jthread 做 move assignment 時,它會先
//    request_stop() 並 join(),然後正常存活,不會中止。
//    這是兩套不同的規則,不要把「差不多」的印象帶過去 ——
//    std::thread 會死,std::jthread 會活。
//
// 【4. 為什麼「執行緒跑完」不在這四種情況裡】
// 因為執行緒結束後,它的資源仍待回收(類似殭屍行程),
// thread 物件依然握著它,依然 joinable()。
// 讓 joinable() 變 false 的永遠是「你做了某個動作」(join/detach/move),
// 而不是「執行緒那邊發生了什麼」。這一點在課程 2.4/4 有詳細示範。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 被 move 走的 std::thread 處於什麼狀態
//   標準對一般型別的通則是「有效但未指定」,但 std::thread 有明確保證:
//   移動之後,來源物件不再關聯任何執行緒,等同預設建構的狀態
//   (joinable() == false)。所以本檔可以放心斷言 t4.joinable() == false。
//
// (B) 為什麼容器裡可以放 std::thread
//   std::vector<std::thread> 完全合法,因為 vector 只要求元素「可移動」,
//   不要求「可複製」。擴容時它會移動而非複製元素。
//   這也是 emplace_back 建立執行緒池的標準寫法能成立的原因。
//
// (C) 這四種狀態與解構子的關係
//   解構子要求 joinable() == false,否則 std::terminate()。
//   所以每個 std::thread 在生命週期結束前,都必須走到這四種狀態之一。
//   被 move 走的那個(狀態 ④)之所以安全,是因為責任跟著所有權
//   一起轉移給了新的持有者 —— 這正是 RAII 守衛類別能運作的基礎。
//
// 【注意事項 Pay Attention】
// 1. std::thread 不可複製,只能移動;它是 move-only 的所有權型別。
// 2. 對仍 joinable 的 std::thread 做 move assignment 會 std::terminate() ——
//    賦值前必須先 join 或 detach 掉原本那條。
// 3. C++20 的 std::jthread 在同樣情況下會 request_stop()+join() 而正常存活,
//    規則與 std::thread 不同,別混用記憶。
// 4. 「執行緒已結束」不會讓 joinable() 變 false;只有 join/detach/move 會。
// 5. 直接輸出 bool 會印 1/0;要 true/false 請加 std::boolalpha。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】joinable() 的四種 false 與移動語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 哪些情況會讓 std::thread 的 joinable() 是 false?
//     答：四種 —— 預設建構(從未關聯執行緒)、已經 join()、已經 detach()、
//         被 std::move 走(所有權轉移給別人)。共同本質是
//         「這個物件目前沒有握著任何執行緒的所有權」。
//         注意「執行緒已經跑完」不在其中:資源未回收前它仍是 joinable。
//     追問：為什麼 std::thread 不能複製?
//         → 因為一條執行緒只能被 join 一次、回收一次。若能複製,
//           兩個物件都會嘗試 join 同一條執行緒,第二次必定失敗。
//           標準在型別層面禁止,讓這種錯誤寫不出來。
//
// 🔥 Q2. std::thread a(f); std::thread b(g); a = std::move(b); 會發生什麼?
//     答：程式會 std::terminate()。因為 a 原本仍是 joinable,
//         move assignment 會讓它原本持有的那條執行緒失去唯一的 handle,
//         變成沒有人能回收的孤兒。標準的處理方式和「忘記 join」一樣:
//         當場中止。正確寫法是先 if (a.joinable()) a.join(); 再賦值。
//     追問：C++20 的 std::jthread 也一樣嗎?
//         → 不一樣,這是常見的陷阱。jthread 的 move assignment 會先
//           request_stop() 並 join() 原本那條,然後正常存活。
//           std::thread 會死、std::jthread 會活,是兩套規則。
//
// ⚠️ 陷阱. 「被 move 走之後,原本的 t4 就是個壞掉的物件了,
//         碰它會有問題,最好完全不要再用它。」哪裡錯了?
//     答：t4 完全沒有壞掉。標準對 std::thread 有明確保證:移動之後,
//         來源物件等同於預設建構的狀態 —— joinable() 為 false。
//         你可以安全地查詢它、可以把新的執行緒指派給它、
//         它的解構子也能正常運作(因為 joinable 已是 false)。
//         真正不能做的只有「對它呼叫 join()/detach()」,那會丟例外。
//     為什麼會錯：把「被移動走的物件處於有效但未指定的狀態」這條通則,
//         誤記成「被移動走的物件不能碰」。通則的意思是「別假設它的值」,
//         而不是「它壞了」。而且 std::thread 與 std::unique_ptr 一樣,
//         屬於少數有「明確保證」的型別 —— 移動後就是空的,可以放心斷言。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 四種讓 joinable() 變成 false 的情況 ===" << std::endl;

    // 情況 1:預設建構
    std::thread t1;
    std::cout << "  ① 預設建構      : " << t1.joinable()
              << "   (從未關聯任何執行緒)" << std::endl;

    // 情況 2:已經 join
    std::thread t2([]() {});
    std::cout << "     (join 之前     : " << t2.joinable() << ")" << std::endl;
    t2.join();
    std::cout << "  ② join 後       : " << t2.joinable()
              << "   (所有權已交還,資源已回收)" << std::endl;

    // 情況 3:已經 detach
    std::thread t3([]() {});
    t3.detach();
    std::cout << "  ③ detach 後     : " << t3.joinable()
              << "   (所有權已放棄,由系統自動回收)" << std::endl;

    // 情況 4:被 move 走
    std::thread t4([]() {});
    std::thread t5 = std::move(t4);
    std::cout << "  ④ move 後(原)  : " << t4.joinable()
              << "   (所有權已轉移出去,等同預設建構)" << std::endl;
    std::cout << "     move 後(新)  : " << t5.joinable()
              << "    (接手了所有權 —— 總數守恆)" << std::endl;
    t5.join();

    std::cout << "\n=== 被 move 走的物件並沒有「壞掉」 ===" << std::endl;
    std::cout << "  t4 現在 joinable = " << t4.joinable()
              << "(可安全查詢)" << std::endl;
    t4 = std::thread([]() {});          // 可以再指派新的執行緒給它
    std::cout << "  指派新執行緒後   = " << t4.joinable()
              << "  (完全可以重複使用)" << std::endl;
    t4.join();

    std::cout << "\n=== ⚠️ move assignment 的致命細節 ===" << std::endl;
    std::cout << "  std::thread a(f);" << std::endl;
    std::cout << "  std::thread b(g);" << std::endl;
    std::cout << "  a = std::move(b);   // 💀 a 原本仍 joinable → std::terminate()"
              << std::endl;
    std::cout << "  正確寫法:先 if (a.joinable()) a.join(); 再賦值。"
              << std::endl;
    std::cout << "  (該錯誤示範未寫進本檔,否則程式會在這裡中止)" << std::endl;

    // 示範正確的做法
    {
        std::thread a([]() {});
        std::thread b([]() {});
        if (a.joinable()) a.join();     // ← 先處理掉原本那條
        a = std::move(b);               // 現在安全了
        std::cout << "  正確流程實測:先 join 再 move,a.joinable = "
                  << a.joinable() << std::endl;
        a.join();
    }

    std::cout << "\n=== std::thread 是 move-only:可以放進容器 ===" << std::endl;
    {
        std::vector<std::thread> pool;
        for (int i = 0; i < 3; ++i) {
            pool.emplace_back([i]() {
                // 各自做一點事;不輸出以保持本檔輸出完全確定
                volatile int sink = 0;
                for (int k = 0; k < 1000; ++k) sink += i;
            });
        }
        std::cout << "  容器內 3 條執行緒,join 前 joinable:";
        for (const std::thread& t : pool) std::cout << " " << t.joinable();
        std::cout << std::endl;

        for (std::thread& t : pool) t.join();

        std::cout << "  join 後 joinable            :";
        for (const std::thread& t : pool) std::cout << " " << t.joinable();
        std::cout << std::endl;
        std::cout << "  ↑ vector 只要求元素可移動,不要求可複製 ——"
                     "所以 move-only 的 thread 放得進去" << std::endl;
    }

    std::cout << std::noboolalpha;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.5：joinable() 狀態檢查1.cpp" -o joinable_states

// 注意:以下為某一次實際執行的結果,每次執行都完全相同 ——
//       本檔所有的 joinable() 查詢都發生在明確的操作前後,
//       而且刻意不讓工作執行緒輸出任何東西,以避免不確定的交錯。

// === 預期輸出 ===
// === 四種讓 joinable() 變成 false 的情況 ===
//   ① 預設建構      : false   (從未關聯任何執行緒)
//      (join 之前     : true)
//   ② join 後       : false   (所有權已交還,資源已回收)
//   ③ detach 後     : false   (所有權已放棄,由系統自動回收)
//   ④ move 後(原)  : false   (所有權已轉移出去,等同預設建構)
//      move 後(新)  : true    (接手了所有權 —— 總數守恆)
//
// === 被 move 走的物件並沒有「壞掉」 ===
//   t4 現在 joinable = false(可安全查詢)
//   指派新執行緒後   = true  (完全可以重複使用)
//
// === ⚠️ move assignment 的致命細節 ===
//   std::thread a(f);
//   std::thread b(g);
//   a = std::move(b);   // 💀 a 原本仍 joinable → std::terminate()
//   正確寫法:先 if (a.joinable()) a.join(); 再賦值。
//   (該錯誤示範未寫進本檔,否則程式會在這裡中止)
//   正確流程實測:先 join 再 move,a.joinable = true
//
// === std::thread 是 move-only:可以放進容器 ===
//   容器內 3 條執行緒,join 前 joinable: true true true
//   join 後 joinable            : false false false
//   ↑ vector 只要求元素可移動,不要求可複製 ——所以 move-only 的 thread 放得進去
