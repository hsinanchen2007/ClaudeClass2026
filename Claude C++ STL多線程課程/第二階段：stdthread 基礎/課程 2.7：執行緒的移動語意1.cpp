// =============================================================================
//  課程 2.7：執行緒的移動語意 1  —  std::thread 為什麼「不可複製」
// =============================================================================
//
// 【主題資訊 Information】
//   類別：std::thread          標頭檔：<thread>          標準：C++11 起
//
//   thread(const thread&)            = delete;   // 複製建構：標準明文刪除
//   thread& operator=(const thread&) = delete;   // 複製賦值：標準明文刪除
//   thread(thread&& other) noexcept;             // 移動建構：允許
//   thread& operator=(thread&& other) noexcept;  // 移動賦值：允許（有陷阱，見檔 6）
//
//   標準依據：[thread.thread.class]
//   編譯必須加 -pthread（GCC/Clang on Linux），否則連結期找不到 pthread 符號。
//
// 【詳細解釋 Explanation】
//
// 【1. 一句話：thread 物件是「作業系統執行緒的唯一擁有者」】
// 這裡最關鍵的觀念是：std::thread 物件**本身不是執行緒**。
// 執行緒是作業系統的資源（在 Linux 上是一個 task，有自己的 kernel stack、
// 排程實體、tid）。std::thread 只是一個「握把（handle）」，內部通常只存了一個
// pthread_t —— 本機 libstdc++ 實測 sizeof(std::thread) == 8，正好就是一個
// pthread_t 的大小，裡面沒有任何引用計數、沒有任何共享狀態。
//
// 既然它只是握把，「複製握把」在技術上完全做得到（memcpy 8 bytes 就好）。
// 標準禁止複製，不是因為做不到，而是因為**一旦允許，所有權就沒有答案了**。
//
// 【2. 如果可以複製，會壞在哪裡：三個無解的問題】
// 假設 std::thread t2 = t1; 是合法的，於是 t1 與 t2 握著同一條 OS 執行緒：
//
//   (a) 誰負責 join？
//       兩個物件解構時都會檢查自己 joinable。若兩個都 join，第二次 join 就是
//       對一個已經被回收的執行緒再 join 一次 → pthread_join 回傳 ESRCH，
//       libstdc++ 會丟 std::system_error。若約定「只有一個要 join」，
//       那是誰？語言層沒有任何機制能表達這件事。
//
//   (b) 一個 detach 之後，另一個怎麼辦？
//       t1.detach() 把執行緒交還給 runtime 自動回收，但 t2 完全不知情，
//       它仍然認為自己 joinable，解構時照樣去 join 一條已經 detach 的執行緒。
//
//   (c) 兩個都不 join 呢？
//       std::thread 的解構子規定：若 joinable() 為真，呼叫 std::terminate()。
//       所以「都不管」也不是選項，程式會直接 abort。
//
// 換句話說：**共享所有權對執行緒這個資源沒有合理語意**。
// 不像記憶體可以用引用計數決定「最後一個離開的人負責釋放」，
// 執行緒的 join 是一個帶有「等待」語意的動作，它必須恰好發生一次、
// 而且由一個明確的人在明確的時機執行。
//
// 【3. 與 unique_ptr 是同一套設計哲學】
// C++ 標準庫把資源所有權分成兩類，std::thread 屬於前者：
//
//   獨佔所有權 (move-only)：unique_ptr、thread、jthread、lock_guard 的表親
//                            unique_lock、ifstream/ofstream、future、
//                            packaged_task
//   共享所有權 (copyable)  ：shared_ptr、shared_future
//
// move-only 型別的判準是：「這個資源被釋放（或收尾）的動作，可以合理地
// 執行多次嗎？」對 unique_ptr 而言，delete 兩次是 UB；對 thread 而言，
// join 兩次是錯誤。答案是「不行」，所以就設計成 move-only，
// 讓**編譯器**在編譯期擋掉，而不是留到執行期才炸。
//
// 【4. = delete 與「private 但不實作」的差別（C++11 之前的做法）】
// C++11 之前想禁止複製，慣例是把複製建構子宣告成 private 且不給定義：
//     private: thread(const thread&);   // 宣告但不定義
// 這招有兩個缺點：
//   * 錯誤訊息很糟：外部呼叫會報「is private」，但**成員函式或 friend 內部**
//     呼叫會通過編譯，變成連結期才報 undefined reference。
//   * 意圖不明確：讀者看不出是「故意不給」還是「還沒寫」。
// C++11 的 = delete 把它變成語言層級的機制：任何情況下命中都是**編譯期錯誤**，
// 而且錯誤訊息會明確講 "use of deleted function"。
//
// 【概念補充 Concept Deep Dive】
//
// (A) = delete 參與多載決議，不是「不存在」
// 這是很多人會誤解的地方：被 delete 的函式**仍然存在於多載集合中**，
// 只是一旦被選中就是編譯錯誤。順序是：先做多載決議 → 選出最佳者 → 才檢查
// 它是不是 deleted。這個設計是刻意的，因為它讓「刪除」能精準攔截特定型別，
// 而不會被別的多載悄悄接手（例如禁止某型別的隱式轉換）。
//
// (B) 為什麼 delete 複製建構子之後，還要「明確寫出」移動建構子？
// C++ 的特殊成員函式生成規則：一旦你**自己宣告**了任何一個複製/移動操作
// 或解構子，編譯器就不會再自動生成移動建構子。std::thread 宣告了
// 複製建構子（= delete）與解構子，所以移動操作必須由標準明文提供。
// 這條規則也是「Rule of Five」的由來。
//
// (C) 為什麼 delete 的是複製、卻仍留下 default 建構子？
// std::thread 有一個不代表任何執行緒的預設狀態（non-joinable，
// get_id() == std::thread::id{}）。這個「空狀態」是移動語意的必要條件：
// 移動之後，來源必須有地方可去。unique_ptr 移動後變 nullptr，
// thread 移動後變預設 id，兩者是同一個模式。
//
// (D) 編譯期擋下 vs 執行期擋下
// 把「不可複製」做成編譯期錯誤，代價是使用者必須顯式寫 std::move；
// 好處是這一整類 bug 完全不可能出現在正式環境。這是 C++ 型別系統
// 最有價值的用法之一：**讓錯誤的程式碼無法通過編譯**。
//
// 【注意事項 Pay Attention】
// 1. std::thread t2 = t1; 與 std::thread t3(t1); 兩種寫法都是複製建構，
//    都是編譯錯誤；錯誤訊息為
//      error: use of deleted function 'std::thread::thread(const std::thread&)'
// 2. 把 thread 放進容器時，容器**只能**用移動。vector 重新配置時會移動元素，
//    因為 std::thread 的移動建構子是 noexcept（本機實測為 true），
//    所以 vector 可以安全使用它。
// 3. 傳給函式時若寫成 void f(std::thread t) 這種值參數，呼叫端**必須**
//    寫 f(std::move(t))，否則就是複製 → 編譯錯誤。
// 4. 別把 thread 物件宣告成 const：const std::thread 無法被移動，
//    也無法呼叫 join()（join 是非 const 成員函式），等於自廢武功。
// 5. 移動走的只是**握把**，不是執行緒本身。執行緒完全不受影響，
//    它照樣在跑，也不會知道自己的握把換了主人。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::thread 不可複製
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::thread 不能複製？
//     答：因為 thread 物件是 OS 執行緒的唯一擁有者。複製會讓兩個物件都認為
//         自己該負責 join，導致「join 兩次」或「一邊 detach、另一邊還想 join」
//         這類沒有正確答案的情況。所有權必須恰好在一個人手上，
//         所以標準把複製建構與複製賦值都 = delete。
//     追問：那為什麼可以移動？→ 移動是「轉移」不是「共享」，
//           轉移後來源變成 non-joinable 的空狀態，責任仍然恰好在一個人身上。
//
// 🔥 Q2. std::thread 和 std::unique_ptr 的所有權模型有什麼共通點？
//     答：兩者都是獨佔所有權的 move-only 型別，判準相同 ——「收尾動作
//         （delete / join）不能執行多次」。兩者移動後來源都退化成一個
//         明確的空狀態（nullptr / 預設 thread::id），讓解構子能安全地
//         判斷「我還需不需要收尾」。
//     追問：那為什麼沒有 shared_thread？→ 因為 join 帶有「等待」語意，
//           多個持有者同時等待同一條執行緒沒有一致的語意，
//           要這種效果應該用 std::shared_future 或 condition_variable 表達。
//
// ⚠️ 陷阱. 「複製 thread 之所以被禁止，是因為複製會把執行緒也複製一份」
//     答：錯。複製 thread 物件不會、也不可能產生第二條 OS 執行緒 ——
//         它只是複製一個 8 bytes 的握把。真正的問題是**兩個握把、
//         一條執行緒、一次 join 額度**，是所有權衝突，不是資源被複製。
//     為什麼會錯：把 std::thread 想成「執行緒本身」，而不是「指向執行緒的
//         智慧指標」。記住：thread 物件之於執行緒，就像 unique_ptr 之於堆積物件。
//
// ⚠️ 陷阱 2. 「= delete 表示這個函式不存在，所以會挑到別的多載」
//     答：錯。deleted function 仍然參與多載決議，只是被選中時報編譯錯誤。
//         所以你不能靠「刪掉複製建構子」讓呼叫悄悄改走移動版本 ——
//         傳左值就是會挑到複製版本、就是會編譯失敗，必須自己寫 std::move。
//     為什麼會錯：把 = delete 想成 SFINAE 式的「移出候選集」，
//         但它其實是「留在候選集裡，命中即錯」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【日常實務範例】用 static_assert 把「不可複製」寫成專案的編譯期契約
// 情境：團隊自訂的 Worker / Session / DbConnection 這類「持有獨佔資源」的
//       型別，最常見的意外是有人不小心讓它變成可複製（例如多加了一個
//       複製建構子，或忘了 = delete），結果連線被複製、被關兩次。
// 做法：在型別定義旁邊放 static_assert，把「這個型別必須是 move-only」
//       這件事變成編譯期會擋下來的契約，而不是 code review 才發現。
// -----------------------------------------------------------------------------
class Worker {
    std::thread th_;  // 持有獨佔資源 → 整個 Worker 自動變成 move-only
public:
    Worker() = default;
    explicit Worker(std::thread th) : th_(std::move(th)) {}
    ~Worker() { if (th_.joinable()) th_.join(); }

    Worker(const Worker&)            = delete;   // 明文禁止複製
    Worker& operator=(const Worker&) = delete;
    Worker(Worker&&) noexcept            = default;
    Worker& operator=(Worker&&) noexcept = default;
};

// 契約：Worker 必須是 move-only。任何人改壞了，這裡就編譯不過。
static_assert(!std::is_copy_constructible_v<Worker>, "Worker 不該可複製");
static_assert(!std::is_copy_assignable_v<Worker>,    "Worker 不該可複製賦值");
static_assert(std::is_move_constructible_v<Worker>,  "Worker 必須可移動");

int main() {
    std::cout << "=== 原始示範：複製被 delete ===\n";
    std::thread t1([]() { std::cout << "t1 執行中\n"; });

    // std::thread t2 = t1;        // 編譯錯誤！
    // std::thread t3(t1);         // 編譯錯誤！
    //
    // 兩行都是「複製建構」，錯誤訊息（GCC 15.2）：
    //   error: use of deleted function
    //          'std::thread::thread(const std::thread&)'

    t1.join();
    std::cout << "t1 已 join\n";

    std::cout << "\n=== std::thread 的型別特性（編譯期查詢）===\n";
    std::cout << std::boolalpha
              << "is_copy_constructible   : " << std::is_copy_constructible_v<std::thread>    << "\n"
              << "is_copy_assignable      : " << std::is_copy_assignable_v<std::thread>       << "\n"
              << "is_move_constructible   : " << std::is_move_constructible_v<std::thread>    << "\n"
              << "is_move_assignable      : " << std::is_move_assignable_v<std::thread>       << "\n"
              << "is_nothrow_move_constr. : " << std::is_nothrow_move_constructible_v<std::thread> << "\n"
              << "is_default_constructible: " << std::is_default_constructible_v<std::thread> << "\n";

    std::cout << "\n=== 握把有多大（本機 libstdc++ 實測，實作定義）===\n";
    std::cout << "sizeof(std::thread)     = " << sizeof(std::thread)     << " bytes\n";
    std::cout << "sizeof(std::thread::id) = " << sizeof(std::thread::id) << " bytes\n";
    std::cout << "（thread 物件內部只有一個 pthread_t，沒有引用計數 →\n"
                 "  這正是它無法支援共享所有權的具體原因）\n";

    std::cout << "\n=== 實務：Worker 的 move-only 契約 ===\n";
    Worker w{std::thread([]() { std::cout << "Worker 內的執行緒工作中\n"; })};
    // Worker w2 = w;   // 編譯錯誤：Worker 的複製也被 delete 了
    Worker w2 = std::move(w);   // 移動 OK
    std::cout << "Worker 已移動（解構時自動 join）\n";
    (void)w2;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.7：執行緒的移動語意1.cpp" -o move1

// === 預期輸出 ===
// 註：以下為某次實際執行結果。「t1 執行中」與「t1 已 join」的相對順序是
//     確定的（join 保證等待），但若把 join 移到別處則不保證。
//
// === 原始示範：複製被 delete ===
// t1 執行中
// t1 已 join
//
// === std::thread 的型別特性（編譯期查詢）===
// is_copy_constructible   : false
// is_copy_assignable      : false
// is_move_constructible   : true
// is_move_assignable      : true
// is_nothrow_move_constr. : true
// is_default_constructible: true
//
// === 握把有多大（本機 libstdc++ 實測，實作定義）===
// sizeof(std::thread)     = 8 bytes
// sizeof(std::thread::id) = 8 bytes
// （thread 物件內部只有一個 pthread_t，沒有引用計數 →
//   這正是它無法支援共享所有權的具體原因）
//
// === 實務：Worker 的 move-only 契約 ===
// Worker 已移動（解構時自動 join）
// Worker 內的執行緒工作中
