// =============================================================================
//  第 2.2 章 之 3  —  用 const T& / T&& 兩個多載實作「複製 vs 移動」
// =============================================================================
//
// 【主題資訊 Information】
//   語法：   void f(const T& x);   // 接 lvalue（也接得到 rvalue，但唯讀）
//            void f(T&& x);        // 只接 rvalue，可以掏空它
//   標準：   T&& 多載為 C++11
//   標頭檔： std::move 需要 <utility>
//   成本：   const T& 版本通常要深拷貝 O(n)；T&& 版本只搬指標 O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. 這一組多載就是整個移動語意的「入口」】
//   C++11 之前，一個吃 std::string 的函式只能寫成 `f(const std::string&)`，
//   無論呼叫端傳來的是一個要長期存活的變數，還是一個下一行就會蒸發的臨時物件，
//   函式內部都只能做同一件事：**深拷貝**。因為它拿到的是唯讀參考，
//   不敢也不能對來源動手。
//
//   加上 `f(std::string&&)` 之後，呼叫端的「意圖」第一次進入了型別系統：
//       f(name);              → 選中 const T&：「我還要用 name，請你自己複製一份」
//       f(std::move(name));   → 選中 T&&     ：「name 我不要了，你儘管掏空它」
//   兩條路徑在編譯期就分岔，執行期零額外判斷、零虛擬呼叫。
//   標準庫的 push_back / insert / emplace 全都是這個模式。
//
// 【2. 進了 T&& 多載之後，為什麼還要再寫一次 std::move？】
//   看本檔的右值版本：
//       void process(std::string&& s) {
//           std::string local = std::move(s);   // ← 這個 move 不能省
//       }
//   s 的**型別**是 std::string&&，但 s 這個**運算式**是 lvalue（它有名字）。
//   如果寫成 `std::string local = s;`，重載決議會選中**複製建構子**，
//   整個「用 T&& 換效能」的努力就白費了 —— 而且編譯器不會給任何警告，
//   因為那段程式碼完全合法，只是慢。
//
//   這是初學者最常犯、也最難自己發現的效能 bug：
//   **T&& 只負責把 rvalue「接進來」，要把它繼續當 rvalue「傳下去」，
//   必須再 std::move 一次。** 詳見本章第 6 個範例檔。
//
// 【概念補充 Concept Deep Dive】
//
// (A) const T& 與 T&& 之間的重載決議（overload resolution）是怎麼判的？
//   兩個多載都能接受右值時，編譯器要在兩條 reference binding 之間排名。
//   規則（[over.ics.rank]）是：**若兩個都可行，綁定到 rvalue reference 的
//   那個，在實參是 rvalue 時勝出**。直覺理解是「更精確、更少妥協」：
//       * 綁到 T&& 不需要加上 const。
//       * 綁到 const T& 需要多做一次 qualification conversion（加 const）。
//   所以：
//       實參是 lvalue  → T&& 根本不可行 → 只剩 const T& → 選 const T&
//       實參是 rvalue  → 兩者皆可行     → T&& 勝出       → 選 T&&
//   這個排名是**確定的、不會 ambiguous**，不需要 SFINAE 或 tag dispatch 幫忙。
//
//   但要小心：如果你同時提供 `f(T&)`、`f(const T&)`、`f(T&&)` 三個，
//   傳 non-const lvalue 時 `f(T&)` 會勝過 `f(const T&)`（同理，少一次加 const）。
//   多載一多就容易失控，這也是後來大家偏好「單一 by-value sink 參數」的原因之一
//   —— 見下方 (C)。
//
// (B) 被移動後的物件處於什麼狀態？
//   標準對標準庫型別的規定是：**valid but unspecified state（有效但未指定的狀態）**。
//   「有效」＝ 你仍然可以安全地解構它、可以指派新值給它、可以呼叫任何
//   「沒有前置條件」的成員函式（如 size()、empty()、clear()）。
//   「未指定」＝ 它裡面**具體是什麼值，標準不保證**。
//
//   本機（libstdc++）實測，被移動後的 std::string 會顯示為空字串，
//   但這是**實作行為，不是標準保證**。其他實作（或短字串走 SSO 的情況）
//   完全可能留下原本的內容。所以：
//       ✔ 可以寫： name.clear(); name = "新值"; if (name.empty()) ...
//       ✘ 不可以假設： name 一定是 ""
//
// (C) 兩個多載 vs 單一 by-value「sink 參數」
//   除了本檔的雙多載寫法，還有一種常見做法：
//       void process(std::string s) { store_ = std::move(s); }   // by value + move
//   * 傳 lvalue → 複製一次進 s，再移動一次進 store_（1 copy + 1 move）
//   * 傳 rvalue → 移動一次進 s，再移動一次進 store_（2 moves）
//   雙多載版本各自只需 1 次操作，理論上更快；但要寫兩份、且 N 個參數會爆炸成
//   2^N 個多載。實務取捨：
//       * 移動很便宜（string / vector）→ by-value sink，程式碼少一半。
//       * 移動也很貴，或在最熱的路徑上   → 寫雙多載。
//   標準庫（如 vector::push_back）選擇雙多載，因為它是所有人的熱路徑。
//
// 【注意事項 Pay Attention】
//   1. 進到 T&& 多載後，往下傳一定要再 std::move，否則會靜默退化成複製。
//   2. 對「已被移動」的物件，只能做無前置條件的操作；不要假設它的內容。
//   3. `process("Charlie")` 走的是 T&& 版本：const char* 先隱式轉型
//      產生一個 std::string 臨時物件（prvalue），臨時物件是右值。
//      這代表這行其實有一次建構成本，不是零成本。
//   4. 只有持有外部資源（heap 指標、file handle、socket）的型別，
//      移動才比複製快。對 int、double 這種型別兩者完全相同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const T& / T&& 多載
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 同時有 `f(const T&)` 和 `f(T&&)`，傳一個右值進去會選哪個？為什麼不會 ambiguous？
//     答：選 T&& 版本。兩者對右值都可行，但標準規定「綁定到 rvalue reference」
//         在實參是 rvalue 時排名較高（綁 const T& 要多一次加 const 的轉換）。
//         這個排名是確定的，所以不會 ambiguous。
//     追問：傳 lvalue 呢？
//         → T&& 對 lvalue 根本不可行（連候選都進不去），只剩 const T&。
//
// ⚠️ 陷阱. 在 `void process(std::string&& s)` 內寫 `std::string local = s;`，
//          會發生移動還是複製？
//     答：**複製**。s 的型別是 std::string&&，但 s 這個運算式有名字，
//         所以是 lvalue，重載決議會選複製建構子。
//         要真正移動必須寫 `std::string local = std::move(s);`。
//     為什麼會錯：多數人把「參數宣告成 T&&」誤解為「這個參數從此永遠是右值」。
//         型別（type）和值類別（value category）是兩個獨立的概念。
//         最可怕的是這個 bug **不會有任何編譯錯誤或警告**，
//         只會讓效能默默退回 C++98 水準。
//
// ⚠️ 陷阱. `process(std::move(name));` 之後，name 一定是空字串嗎？
//     答：不一定。標準只保證它處於 **valid but unspecified state**。
//         本機 libstdc++ 實測顯示為空字串，但那是實作細節、不是標準保證。
//         你可以安全地對它重新賦值或呼叫 clear()，但不可以假設它的內容。
//     為什麼會錯：大家在自己機器上跑出「空字串」，就把它當成語言規範背下來，
//         換一個標準庫實作或換一個字串長度（SSO）就可能不同。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <vector>

// 重載 1：接收左值
void process(const std::string& s) {
    std::cout << "[左值版本] 收到: " << s << "\n";
    // 這裡只能讀取 s，如果需要保存，必須複製
}

// 重載 2：接收右值
void process(std::string&& s) {
    std::cout << "[右值版本] 收到: " << s << "\n";
    // 這裡可以安全地「偷走」s 的資源
    std::string local = std::move(s);  // 移動，而非複製（這個 move 不能省！）
    std::cout << "  移動到 local: " << local << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】訊息佇列的 push —— 典型的 sink 參數雙多載
//   情境：一個背景 worker 佇列，上游可能傳「還要繼續用的訊息」（必須複製），
//         也可能傳「組完就丟的一次性訊息」（可以直接搬進來）。
//   這正是 std::vector::push_back 提供兩個多載的理由，我們在這裡手寫一次。
// -----------------------------------------------------------------------------
class MessageQueue {
public:
    // lvalue 版本：呼叫端還要用，我們只能自己複製一份
    void push(const std::string& msg) {
        std::cout << "    push(const&) 複製一份: \"" << msg << "\"\n";
        items_.push_back(msg);            // 深拷貝
    }

    // rvalue 版本：呼叫端已放棄所有權，直接把緩衝區接收過來
    void push(std::string&& msg) {
        std::cout << "    push(&&)     直接接手: \"" << msg << "\"\n";
        items_.push_back(std::move(msg)); // 不能省 move，否則退化成複製
    }

    std::size_t size() const { return items_.size(); }

private:
    std::vector<std::string> items_;
};

int main() {
    std::cout << "=== 四種呼叫方式各自選中哪個多載 ===\n";
    std::string name = "Alice";

    process(name);                        // 呼叫左值版本
    process(std::string("Bob"));          // 呼叫右值版本
    process("Charlie");                   // 呼叫右值版本（隱含轉型產生臨時物件）
    process(std::move(name));             // 呼叫右值版本（明確轉為右值）

    std::cout << "\nname after move: \"" << name << "\"\n";
    std::cout << "  ↑ 本機 libstdc++ 實測為空字串；標準只保證 "
                 "valid but unspecified，不可假設內容\n";
    std::cout << "  對它重新賦值永遠安全：";
    name = "Alice-2";
    std::cout << "name = \"" << name << "\"\n";

    std::cout << "\n=== 日常實務：訊息佇列 push ===\n";
    MessageQueue q;

    std::string keep_using = "heartbeat";      // 之後還要用 → 走複製
    q.push(keep_using);
    std::cout << "    keep_using 仍可用: \"" << keep_using << "\"\n";

    q.push("GET /index.html HTTP/1.1");        // 字面值 → 產生臨時物件 → 走移動

    std::string one_shot = "job-42 finished";  // 用完即丟 → 明確 move
    q.push(std::move(one_shot));

    std::cout << "    佇列長度: " << q.size() << "\n";

    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra "第 2.2 章：右值參考 && — 基本語法與用途3.cpp" -o rv3

// === 預期輸出 ===
// === 四種呼叫方式各自選中哪個多載 ===
// [左值版本] 收到: Alice
// [右值版本] 收到: Bob
//   移動到 local: Bob
// [右值版本] 收到: Charlie
//   移動到 local: Charlie
// [右值版本] 收到: Alice
//   移動到 local: Alice
//
// name after move: ""
//   ↑ 本機 libstdc++ 實測為空字串；標準只保證 valid but unspecified，不可假設內容
//   對它重新賦值永遠安全：name = "Alice-2"
//
// === 日常實務：訊息佇列 push ===
//     push(const&) 複製一份: "heartbeat"
//     keep_using 仍可用: "heartbeat"
//     push(&&)     直接接手: "GET /index.html HTTP/1.1"
//     push(&&)     直接接手: "job-42 finished"
//     佇列長度: 3
