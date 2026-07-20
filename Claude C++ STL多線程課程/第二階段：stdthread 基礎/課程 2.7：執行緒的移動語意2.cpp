// =============================================================================
//  課程 2.7：執行緒的移動語意 2  —  移動建構：所有權轉移與 moved-from 狀態
// =============================================================================
//
// 【主題資訊 Information】
//   thread(thread&& other) noexcept;             // 標準：C++11 起
//   標頭檔：<thread>                              標準依據：[thread.thread.constr]
//
//   標準對移動建構的規定（逐字語意）：
//     Effects: 建構一個 thread 物件，取得 other 的關聯執行緒；
//              other 被置為 **default constructed state**。
//     Postconditions: other.get_id() == id()（即 std::thread::id{}）
//                     且新物件的 get_id() 等於 other 移動前的 get_id()。
//
//   關鍵字 noexcept：移動建構保證不丟例外（本機實測
//   is_nothrow_move_constructible_v<std::thread> == true）。
//
// 【詳細解釋 Explanation】
//
// 【1. 移動到底做了什麼：只有握把換手，執行緒毫無感覺】
// std::thread t2 = std::move(t1); 這一行**沒有**做以下任何一件事：
//   ✗ 沒有建立新執行緒        ✗ 沒有暫停或喚醒執行緒
//   ✗ 沒有複製執行緒的堆疊     ✗ 沒有任何系統呼叫
//
// 它實際做的事情，用 libstdc++ 的實作講白話就是：
//     t2._M_id = t1._M_id;          // 把 pthread_t 抄過來（8 bytes）
//     t1._M_id = thread::id{};      // 來源清成預設值
// 就這兩步。所以移動建構是 O(1)、noexcept、而且對正在跑的執行緒完全透明 ——
// 那條 OS 執行緒不知道也不在乎自己的握把換了主人，它只管繼續執行。
//
// 「移動」在 C++ 裡的正確理解一向不是「搬東西」，而是
// **「轉移責任」**。這裡轉移的責任具體就是：誰必須呼叫 join()。
//
// 【2. moved-from 狀態：不是「未定義」，是標準明文指定的】
// 一般的 C++ 型別（例如 std::string、std::vector）被移動之後，標準只保證它處於
// 「valid but unspecified state」—— 可以安全解構、可以賦新值，但**內容不保證**
// （被移動的 string 是不是空字串？標準沒說，實作可以自己決定）。
//
// std::thread 是**例外**，它的 moved-from 狀態被標準精確指定為
// 「default constructed state」，因此以下三件事都是可以依賴的保證：
//     t1.joinable()  == false
//     t1.get_id()    == std::thread::id{}
//     t1.~thread()   不會呼叫 terminate（因為 joinable 為 false）
//
// 為什麼標準要在這裡加碼給出強保證？因為 thread 的解構子有殺傷力：
// 「若 joinable() 為真則呼叫 std::terminate()」。如果 moved-from 狀態是
// unspecified，那麼「移動之後來源物件解構時會不會 abort」就變成不可預測，
// 移動語意將完全無法使用。強保證是必要的，不是慷慨。
//
// 【3. 移動之後，來源物件仍然完全可用】
// 這點常被誤會成「moved-from 物件是垃圾，碰不得」。對 std::thread 而言，
// 它只是回到「剛預設建構」的乾淨狀態，你可以：
//     t1.joinable();               // OK，回傳 false
//     t1 = std::thread(f);         // OK，重新賦予一條新執行緒（安全，因為
//                                  //     此時 t1 是 non-joinable，見檔 6）
// 但**不可以**：
//     t1.join();                   // 丟 std::system_error，errc::invalid_argument
//     t1.detach();                 // 同上
// 因為 join/detach 的前置條件是 joinable() 為真。
//
// 【4. std::move 本身不移動任何東西】
// std::move(t1) 只是一個 static_cast<thread&&>，它是**編譯期的型別轉換**，
// 執行期什麼都不做、不產生任何指令。真正做事的是接下來被選中的那個多載 ——
// 因為運算式現在是 rvalue，多載決議會挑到 thread(thread&&) 而不是被 delete 的
// thread(const thread&)。所以正確的說法是：
//   std::move 是「請求以移動方式處理」，實際移動由移動建構子/移動賦值完成。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼移動建構必須是 noexcept
// 這不只是「順便標一下」。std::vector 在重新配置時，會用
// std::move_if_noexcept 決定要移動還是複製元素：
//   * 若移動建構為 noexcept → 用移動（快）
//   * 若移動可能丟例外、而且型別可複製 → 用複製（為了維持強例外保證）
// std::thread 的複製已被 delete，所以 vector 別無選擇只能移動；但正因為
// 移動是 noexcept，vector 的 push_back 才能維持強例外保證。
// 若移動建構會丟例外，vector 在搬到一半時失敗，就會有一堆執行緒握把
// 卡在「搬了一半」的狀態 —— 那是無法回復的災難。noexcept 是必要條件。
//
// (B) thread::id 的印法是實作定義的
// std::thread::id 支援 operator<<，但**輸出格式由實作決定**，標準只要求
// 相等的 id 印出相同結果。本機 libstdc++ 把預設 id 印成文字
// "thread::id of a non-executing thread"，把有效 id 印成一個大整數
// （就是 pthread_t 的值）。MSVC 則印數字 0。所以：
//   * 不要 parse 這個輸出；
//   * 判斷「是不是空的」請用 t.get_id() == std::thread::id{} 或直接 joinable()。
//
// (C) joinable() 的真正定義
// joinable() 並不是「執行緒還在跑嗎」。它的定義是
// 「這個 thread 物件是否關聯到某條執行緒」。所以：
//   * 執行緒早就跑完了，但你還沒 join → joinable() 仍是 true
//     （執行緒的結束狀態還沒被回收，你仍然欠一次 join）
//   * 執行緒還在跑，但你已經 detach → joinable() 是 false
// 要問「跑完了沒」，std::thread 沒有提供 API，得用 std::future 或
// atomic flag 自己表達。
//
// (D) 一行搞定的所有權轉移，其實有三種寫法
//     std::thread t2 = std::move(t1);   // 移動建構（本檔示範）
//     std::thread t2(std::move(t1));    // 同上，直接初始化
//     t2 = std::move(t1);               // 移動賦值（t2 必須 non-joinable！見檔 6）
// 前兩者一定安全（t2 是新物件），第三者是本課最大的陷阱。
//
// 【注意事項 Pay Attention】
// 1. 移動之後**只有 t2 能 join，也只有 t2 必須 join**。對 t1 呼叫 join()
//    會丟 std::system_error（invalid_argument），不是 UB、不是 crash。
// 2. 移動不會等待執行緒。t2 拿到握把的當下，執行緒可能還在跑、
//    可能已經跑完 —— 移動與執行緒的進度完全無關。
// 3. 若在移動之後、join 之前發生例外並逃出作用域，t2 的解構子會看到
//    joinable() == true → std::terminate()。這正是需要 RAII 包裝
//    （scoped_thread / C++20 jthread）的理由，見檔 7。
// 4. 多執行緒同時對 cout 輸出時，各段文字可能交錯；這是 race condition
//    （順序不保證），**不是 data race**（標準保證對同步過的標準串流
//    並行輸出不產生 data race，見 [iostream.objects]）。
// 5. 本檔輸出順序是非決定性的：子執行緒的 "執行中" 何時出現，
//    取決於作業系統排程。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動建構與 moved-from 狀態
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread 被 std::move 之後，來源物件處於什麼狀態？
//     答：標準明文規定是 default constructed state ——
//         joinable() == false、get_id() == std::thread::id{}。
//         這比一般型別的「valid but unspecified」更強，是可以依賴的保證。
//     追問：為什麼 thread 要給比 string/vector 更強的保證？
//           → 因為 thread 的解構子在 joinable 時會呼叫 terminate。
//             若 moved-from 狀態不確定，移動之後程式會不會 abort 就變成
//             不可預測，移動語意等於不能用。
//
// 🔥 Q2. std::move(t) 這行程式碼在執行期做了什麼？
//     答：什麼都沒做。std::move 只是 static_cast<T&&>，是編譯期的型別轉換，
//         產生零條指令。它的作用是讓運算式變成 rvalue，使多載決議挑中
//         移動建構子；真正搬握把的是那個移動建構子。
//     追問：那 std::move 之後物件一定被搬空嗎？→ 不一定。若接手的型別
//           沒有移動建構子（只有複製），會靜默退回複製，來源完全不變。
//
// ⚠️ 陷阱. 「移動之後對來源 t1 呼叫 join() 會是未定義行為（UB）」
//     答：錯，這不是 UB。join() 的前置條件是 joinable() 為真，違反時
//         標準規定丟 std::system_error（errc::invalid_argument）。
//         本機 libstdc++ 實測會丟例外並印出 "Invalid argument"，
//         是有定義的錯誤行為，不是 UB。
//     為什麼會錯：把「違反前置條件」一律等同於 UB。C++ 標準庫其實分成
//         兩類：一類是 UB（例如對空 vector 呼叫 front()），另一類明文
//         規定丟例外（例如這裡的 join、以及 at() 的 out_of_range）。
//         要分清楚，否則寫錯誤處理時會判斷失準。
//
// ⚠️ 陷阱 2. 「t1 移動走之後就報廢了，不能再用」
//     答：錯。t1 回到的是預設建構狀態，是完全合法可用的物件。
//         你可以立刻 t1 = std::thread(f); 重新指派一條新執行緒 ——
//         而且這個移動賦值是安全的，因為此時 t1 恰好是 non-joinable。
//     為什麼會錯：把 C++ 的 moved-from 誤解成 Rust 的 move（來源在
//         編譯期就不可再存取）。C++ 的 moved-from 物件仍然活著、
//         仍會被解構，只是內容變了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>

// -----------------------------------------------------------------------------
// 【日常實務範例】把已啟動的背景任務交給更長壽的擁有者
// 情境：HTTP server 收到一個「匯出報表」的請求，處理函式當場啟動一條背景
//       執行緒開始跑，但函式必須立刻回應 202 Accepted 給客戶端，不能等它跑完。
//       執行緒的所有權因此必須從「短命的 request handler」轉移到
//       「長命的 JobRegistry」，否則 handler 一離開作用域，
//       thread 的解構子看到 joinable == true 就會 terminate 整個 server。
// 關鍵：所有權轉移用移動建構完成，全程沒有第二條執行緒、沒有複製。
// -----------------------------------------------------------------------------
class JobRegistry {
    std::thread worker_;   // 長命物件持有握把
public:
    // 以值接收 + 移入成員：呼叫端必須 std::move，意圖非常清楚
    void adopt(std::thread t) {
        if (worker_.joinable()) worker_.join();   // 先收掉舊的，避免 terminate
        worker_ = std::move(t);                   // 此時 worker_ 保證 non-joinable
        std::cout << "  [Registry] 已接管背景任務\n";
    }
    ~JobRegistry() {
        if (worker_.joinable()) {
            worker_.join();
            std::cout << "  [Registry] 解構時 join 完成\n";
        }
    }
};

int main() {
    std::cout << "=== 原始示範：移動建構與 joinable 變化 ===\n";
    std::thread t1([]() {
        std::cout << "執行中" << std::endl;
    });

    std::cout << "t1 joinable: " << t1.joinable() << std::endl;
    std::cout << "t1 id      : " << t1.get_id() << std::endl;

    // 移動所有權：握把從 t1 換手到 t2，執行緒本身毫無感覺
    std::thread t2 = std::move(t1);

    std::cout << "移動後:" << std::endl;
    std::cout << "t1 joinable: " << t1.joinable() << std::endl;
    std::cout << "t2 joinable: " << t2.joinable() << std::endl;

    // 標準保證：moved-from 的 id 等於預設建構的 id
    std::cout << "t1 id == thread::id{} ? " << std::boolalpha
              << (t1.get_id() == std::thread::id{}) << std::endl;
    std::cout << "t1 id 印出來是: " << t1.get_id()
              << "  （格式為實作定義，本機 libstdc++）" << std::endl;

    t2.join();  // 由 t2 負責 join

    std::cout << "\n=== 對 moved-from 物件 join()：丟例外，不是 UB ===\n";
    try {
        t1.join();   // t1 早已 non-joinable
    } catch (const std::system_error& e) {
        std::cout << "捕捉到 std::system_error: " << e.what() << "\n";
    }

    std::cout << "\n=== moved-from 物件可以重新使用 ===\n";
    t1 = std::thread([]() { std::cout << "t1 被重新指派了一條新執行緒\n"; });
    std::cout << "重新指派後 t1 joinable: " << t1.joinable() << "\n";
    t1.join();

    std::cout << "\n=== 實務：所有權轉移給長命的 JobRegistry ===\n";
    {
        JobRegistry reg;
        std::thread job([]() { std::cout << "  [Job] 報表匯出中...\n"; });
        reg.adopt(std::move(job));   // 必須 move，否則編譯錯誤
        std::cout << "  handler 立刻回應 202 Accepted（不等任務跑完）\n";
    }   // reg 解構 → join

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.7：執行緒的移動語意2.cpp" -o move2

// 註：以下為某次實際執行結果。**每次執行都不同** —— 子執行緒的
//     t1 的 id 數值每次執行都不同（是 pthread_t 的值）。

// === 預期輸出 ===
//     "執行中"、"[Job] 報表匯出中..." 會出現在哪一行取決於 OS 排程，
//     可能提前、可能延後、也可能與主執行緒的輸出交錯在同一行附近。
//     只有 join() 之後的順序才是有保證的。
//
// === 原始示範：移動建構與 joinable 變化 ===
// t1 joinable: 1
// t1 id      : 128733367809728
// 移動後:
// t1 joinable: 0
// t2 joinable: 1
// t1 id == thread::id{} ? true
// t1 id 印出來是: thread::id of a non-executing thread  （格式為實作定義，本機 libstdc++）
// 執行中
//
// === 對 moved-from 物件 join()：丟例外，不是 UB ===
// 捕捉到 std::system_error: Invalid argument
//
// === moved-from 物件可以重新使用 ===
// 重新指派後 t1 joinable: true      ← 注意：前面設過 std::boolalpha，之後都印 true/false
// t1 被重新指派了一條新執行緒
//
// === 實務：所有權轉移給長命的 JobRegistry ===
//   [Registry] 已接管背景任務
//   handler 立刻回應 202 Accepted（不等任務跑完）
//   [Job] 報表匯出中...
//   [Registry] 解構時 join 完成
