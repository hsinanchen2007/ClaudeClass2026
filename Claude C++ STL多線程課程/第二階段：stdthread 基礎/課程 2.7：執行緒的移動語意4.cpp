// =============================================================================
//  課程 2.7：執行緒的移動語意 4  —  把 thread 傳進函式（sink parameter）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：void takeOwnership(std::thread t);      // 以「值」接收 = sink 參數
//         takeOwnership(std::move(t));            // 呼叫端必須顯式 move
//   標頭檔：<thread>                               標準：C++11 起
//
//   四種參數傳遞方式對 move-only 型別的意義：
//     void f(std::thread  t);   // 值   → 接管所有權（sink），呼叫端要 move
//     void f(std::thread& t);   // 左值參考 → 借用，可修改，不轉移所有權
//     void f(const std::thread& t); // const 參考 → 幾乎沒用（join 是非 const）
//     void f(std::thread&& t);  // 右值參考 → 「宣告要接管」但實際上沒有接管，
//                               //             除非函式內再 std::move 一次（陷阱！）
//
// 【詳細解釋 Explanation】
//
// 【1. sink parameter：用型別把「我要接管」講清楚】
// 「sink（匯）」是指這個函式會**吃掉**傳進來的資源，呼叫端交出去之後就不該
// 再使用它。對 move-only 型別而言，以值接收是表達這件事最好的方式，因為：
//
//   * 編譯器強制執行：呼叫端不寫 std::move 就編譯失敗（複製被 delete），
//     所以「所有權要交出去」這件事在**呼叫點就看得見**，不用去翻文件。
//   * 自我文件化：看到 f(std::move(t)) 就知道 t 之後不能再用了。
//   * 呼叫端保有彈性：想保留就別 move（但對 thread 而言複製被禁，
//     所以就是強制交出）。
//
// 這和 std::unique_ptr 的慣例完全一致：想接管就寫
// void consume(std::unique_ptr<T> p)，想借用就寫 void use(T& r)。
//
// 【2. 值參數 vs 右值參考參數：一個重要但常被忽略的差別】
// 這兩個宣告看起來都像「我要接管」：
//     void byValue(std::thread t);      // (A)
//     void byRvalueRef(std::thread&& t);// (B)
// 但語意差很多：
//
//   (A) 值參數：參數 t 是一個**獨立的物件**，由呼叫端的引數移動建構而來。
//       函式一進來，所有權就**已經**在 t 身上了。函式結束時 t 解構，
//       若忘了 join 就會 terminate —— 責任明確地落在函式內。
//
//   (B) 右值參考：t 只是**綁定到呼叫端物件的別名**，所有權**還沒轉移**！
//       如果函式內只是讀它、沒有再做一次 std::move，那麼呼叫端的物件
//       仍然 joinable，責任還在呼叫端。這是個真陷阱：
//           void byRvalueRef(std::thread&& t) {
//               // 忘了 std::move(t) 移入某處
//           }   // 函式結束，什麼都沒發生
//           std::thread a(f);
//           byRvalueRef(std::move(a));   // a 仍然 joinable！
//           // a 解構 → std::terminate()
//       另外注意：在函式**內部**，具名的 t 是 **lvalue**（有名字的東西都是
//       lvalue，即使型別是右值參考），所以要轉交出去必須再寫一次 std::move(t)。
//
//   結論：想接管就用值參數 (A)，語意最清楚、最不容易寫錯。
//
// 【3. 為什麼呼叫端「必須」寫 std::move】
//     takeOwnership(t);              // 編譯錯誤
// t 是 lvalue，多載決議會嘗試用複製建構子初始化參數 → 但複製被 = delete
// → error: use of deleted function。
//     takeOwnership(std::move(t));   // OK
// std::move 把 t 轉成 rvalue，於是選中移動建構子，握把搬進參數。
//
// 這個「必須手寫 std::move」的要求常被抱怨囉唆，但它其實是**功能不是缺陷**：
// 它讓每一次所有權轉移在原始碼上都留下明顯痕跡，
// code review 時用眼睛掃 std::move 就能追蹤資源流向。
//
// 【4. 移動之後，呼叫端的物件怎麼了】
// 移動之後 t 處於 default constructed state：
//     t.joinable()  == false
//     t.get_id()    == std::thread::id{}
// 所以本檔最後印出的 "t joinable: 0" 是**標準保證**的結果，不是巧合。
// 也因此 main 結束時 t 解構不會 terminate —— 它已經沒有執行緒要負責了。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 const std::thread& 幾乎沒有用
// join()、detach()、swap() 全都是**非 const** 成員函式，因為它們會改變物件
// 狀態。const 參考只剩下 joinable()、get_id()、native_handle() 可用。
// 更根本地說：const 的意思是「我不會改變你」，但對執行緒握把來說，
// 你能做的有意義的事幾乎都是「改變它」。所以實務上不會看到這種介面。
//
// (B) 值參數的一次移動有沒有成本
// 有，但是 8 bytes 的複製加一次清零，完全可以忽略。對 thread 這種
// 「握把型別」，值參數是零負擔的最佳選擇。要注意的是同樣的 sink 慣例
// 用在 std::string / std::vector 上時，值參數會多一次移動（呼叫端 →
// 參數 → 成員），若函式只是要存起來，「值參數 + std::move 到成員」
// 仍是公認的最佳實務（比 const& + 複製快，比寫兩個多載簡單）。
//
// (C) 例外安全：所有權在飛行途中失敗會怎樣
//     takeOwnership(std::move(t), mightThrow());
// 引數求值順序在 C++17 前是未指定的。若 std::move(t) 這個引數已經被
// 求值並移動好，而 mightThrow() 接著丟例外，那條執行緒就沒人 join 了。
// C++17 後對函式引數的求值有了「不交錯」的保證（但順序仍未指定），
// 情況改善但沒有完全消失。最保險的做法是**一次只轉移一個所有權**，
// 或用 RAII 包裝（見檔 7）。
//
// (D) 與 std::unique_ptr 的介面設計對照表
//     接管所有權：void sink(std::unique_ptr<T> p)  ←→ void sink(std::thread t)
//     借用不接管：void use(T& r)                   ←→ void use(std::thread& t)
//     觀察狀態  ：void peek(const T& r)            ←→ 幾乎不用（見 (A)）
// 記住這張對照表，thread 的介面設計就不用背了。
//
// 【注意事項 Pay Attention】
// 1. 呼叫端一旦 std::move 出去，**不要再對原物件呼叫 join()/detach()**，
//    會丟 std::system_error。要檢查請先問 joinable()。
// 2. 函式若以值接收 thread 卻**忘了 join 或 detach**，函式結束時參數解構
//    → joinable 為真 → std::terminate()。以值接收就是承諾要處理掉它。
// 3. std::thread&& 參數不會自動轉移所有權；函式內要再 std::move 一次。
//    具名的右值參考本身是 lvalue，這點務必記牢。
// 4. 不要為 sink 函式同時提供 const& 多載 —— thread 不可複製，
//    那個多載寫不出有意義的實作。
// 5. 本檔輸出順序非決定性：子執行緒的 "工作中" 與主執行緒的
//    "取得執行緒所有權" 誰先印出來取決於排程。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】把 thread 傳進函式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void f(std::thread t) 這個介面在表達什麼？呼叫端要怎麼寫？
//     答：這是 sink parameter，表示 f 會**接管**執行緒的所有權，
//         之後由 f 負責 join 或 detach。呼叫端必須寫 f(std::move(t))，
//         直接寫 f(t) 會因為複製建構子被 delete 而編譯失敗。
//     追問：為什麼不用 std::thread&？→ 參考是「借用」語意，所有權仍在
//           呼叫端，責任歸屬會變得不清楚；用值參數讓編譯器強制呼叫端
//           在程式碼上明示所有權交接。
//
// 🔥 Q2. 以值接收 thread 的函式，如果忘了 join 會怎樣？
//     答：函式結束時參數解構，此時 joinable() 為真，
//         std::thread 的解構子依標準呼叫 std::terminate() → 程式 abort。
//         這是標準保證的行為（[thread.thread.destr]），不是 UB。
//     追問：C++20 有更好的做法嗎？→ 有，改用 std::jthread，
//           它的解構子會自動 request_stop() + join()，不會 abort。
//
// ⚠️ 陷阱. void f(std::thread&& t) { /* 沒有再 move */ }
//           呼叫 f(std::move(a)) 之後，a 還 joinable 嗎？
//     答：**還是 joinable**。右值參考只是綁定到 a 的別名，
//         所有權從頭到尾沒有離開 a。函式內若沒有再 std::move(t) 把它移入
//         某個真正的物件，就什麼也沒發生。結果是 a 解構時
//         → std::terminate()。
//     為什麼會錯：把「參數宣告成 &&」誤解為「所有權自動轉移」。
//         真正搬動握把的是**移動建構子/移動賦值**，而 && 參數只是
//         「我願意綁定 rvalue」的宣告。附帶一個更常見的連鎖錯誤：
//         函式內的具名參數 t 是 lvalue，直接 g(t) 會挑到複製版本，
//         必須寫 g(std::move(t))。
//
// ⚠️ 陷阱 2. 「move 進函式之後，原本的 t 就不能再碰了，碰了是 UB」
//     答：不是 UB。t 處於標準明訂的 default constructed state，
//         呼叫 t.joinable()（回傳 false）、印 t.get_id()、
//         甚至重新 t = std::thread(...) 都完全合法。
//         唯一不能做的是 join()/detach()，那會丟 std::system_error。
//     為什麼會錯：把 C++ 的 moved-from 當成 Rust 的 move-out（編譯期就
//         禁止再存取）。C++ 的來源物件仍然活著、仍會被解構。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <utility>
#include <vector>

// 原始示範：sink parameter —— 以值接收，函式負責 join
void takeOwnership(std::thread t) {
    std::cout << "取得執行緒所有權" << std::endl;
    t.join();      // 以值接收就是承諾要處理掉它，否則解構時 terminate
}

// 對照：右值參考參數 —— 看起來像接管，實際上沒有
void looksLikeOwnership(std::thread&& t) {
    // 注意：具名的 t 在這裡是 **lvalue**，要真的接管必須再 std::move(t)
    std::cout << "  函式內 t.joinable() = " << std::boolalpha << t.joinable() << "\n";
    std::thread taken = std::move(t);   // ← 這一行才真的接管
    taken.join();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池：把已建立的 reader 執行緒交給 pool 統一管理
// 情境：網路服務為每個新連線啟動一條 reader 執行緒。建立的地方（accept 迴圈）
//       不適合負責 join —— 它必須立刻回去 accept 下一個連線。所以把執行緒
//       的所有權「交（sink）」給 ConnectionPool，由 pool 在關閉時統一收尾。
// 重點：submit 以值接收，呼叫端被編譯器強制寫 std::move，
//       所有權交接在原始碼上一目了然。
// -----------------------------------------------------------------------------
class ConnectionPool {
    std::vector<std::thread> readers_;
public:
    // sink parameter：接管所有權
    void submit(std::thread reader) {
        readers_.push_back(std::move(reader));   // 再移一次，移進容器
        std::cout << "  [pool] 目前管理 " << readers_.size() << " 條 reader\n";
    }
    // 統一收尾：這是 pool 的責任，不是 accept 迴圈的責任
    void shutdown() {
        for (auto& r : readers_) {
            if (r.joinable()) r.join();
        }
        readers_.clear();
        std::cout << "  [pool] 全部 reader 已 join，連線池關閉\n";
    }
    ~ConnectionPool() { shutdown(); }
};

int main() {
    std::cout << "=== 原始示範：sink parameter ===\n";
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });

    takeOwnership(std::move(t));  // 必須 move，否則編譯錯誤

    // t 現在是空的（標準保證：default constructed state）
    std::cout << "t joinable: " << t.joinable() << std::endl;
    std::cout << "t id == thread::id{} ? " << std::boolalpha
              << (t.get_id() == std::thread::id{}) << std::endl;

    std::cout << "\n=== 對照：右值參考參數不會自動接管 ===\n";
    std::thread r([]() { std::cout << "  rvalue-ref 示範執行緒\n"; });
    looksLikeOwnership(std::move(r));
    std::cout << "呼叫後 r.joinable() = " << r.joinable()
              << "（因為函式內有再 std::move 一次，所以已轉走）\n";

    std::cout << "\n=== moved-from 物件可重新指派 ===\n";
    t = std::thread([]() { std::cout << "  t 被重新指派\n"; });
    std::cout << "重新指派後 t.joinable() = " << t.joinable() << "\n";
    t.join();

    std::cout << "\n=== 實務：連線池接管 reader 執行緒 ===\n";
    {
        ConnectionPool pool;
        for (int fd = 100; fd < 103; ++fd) {
            // accept 迴圈：建立即交出，自己不負責 join
            pool.submit(std::thread([fd]() {
                std::cout << "  [reader] 處理連線 fd=" << fd << "\n";
            }));
        }
        std::cout << "  accept 迴圈結束，繼續服務其他工作\n";
    }   // pool 解構 → shutdown() → 全部 join

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.7：執行緒的移動語意4.cpp" -o move4

// 註：以下為某次實際執行結果。**每次執行都不同** —— 子執行緒的輸出

// === 預期輸出 ===
//     （"工作中"、"[reader] 處理連線 fd=..."）出現的位置與彼此的先後
//     取決於 OS 排程；三條 reader 的 fd 順序也不保證是 100、101、102。
//     只有 join() 之後的段落界線是確定的。
//
// === 原始示範：sink parameter ===
// 取得執行緒所有權
// 工作中
// t joinable: 0
// t id == thread::id{} ? true
//
// === 對照：右值參考參數不會自動接管 ===
//   函式內 t.joinable() = true
//   rvalue-ref 示範執行緒
// 呼叫後 r.joinable() = false（因為函式內有再 std::move 一次，所以已轉走）
//
// === moved-from 物件可重新指派 ===
// 重新指派後 t.joinable() = true
//   t 被重新指派
//
// === 實務：連線池接管 reader 執行緒 ===
//   [pool] 目前管理 1 條 reader
//   [pool] 目前管理 2 條 reader
//   [reader] 處理連線 fd=  [pool] 目前管理 100
// 3 條 reader
//   accept 迴圈結束，繼續服務其他工作
//   [reader] 處理連線 fd=101
//   [reader] 處理連線 fd=102
//   [pool] 全部 reader 已 join，連線池關閉
//
// ↑ 注意倒數第 6～7 行：兩條執行緒的輸出**在同一行中間交錯**了
//   （"fd=" 之後直接接上另一條執行緒的 "[pool] 目前管理"，"100" 被擠到後面）。
//   這是 race condition —— 輸出的**順序**不保證，因為 << 是多次呼叫，
//   兩次呼叫之間可能被其他執行緒插隊。
//   但這**不是 data race**：標準保證對同步過的標準串流並行輸出不產生
//   data race（[iostream.objects]），程式行為仍然有定義、不會是 UB。
//   要避免交錯，必須自己加鎖，或先組好整行字串再一次輸出。
