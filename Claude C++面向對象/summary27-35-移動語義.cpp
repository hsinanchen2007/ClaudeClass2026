// ============================================================
// 第 31～35 課總結：移動語義（Move Semantics）
// 涵蓋：右值引用、移動建構、移動賦值、五法則、std::move
// 編譯：g++ -std=c++17 -o move_semantics summary27-35-移動語義.cpp
// ============================================================

#include <iostream>
#include <cstring>
#include <algorithm>
#include <utility>     // std::move, std::swap
#include <vector>
#include <string>
#include <type_traits> // is_copy_constructible, etc.

using namespace std;

// =============================================================================
//
// 【主題資訊 Information】
//   範圍：  右值引用（31）→ 移動建構（32）→ 移動賦值（33）
//           → 五法則（34）→ std::move 的正確用法（35）
//   標準：  移動語義是 C++11 引入的。本檔以 C++17 編譯，涉及版本的關鍵點：
//             * 右值引用、std::move、noexcept、= default/= delete —— C++11
//             * "Bob"s 字面量後綴（std::string_literals）—— C++14
//             * 保證的 copy elision（純右值不再需要可存取的移動建構）—— C++17
//   標頭檔：<iostream> <cstring> <algorithm> <utility> <vector> <string> <type_traits>
//   關鍵詞：rvalue reference、value category、move constructor、Rule of Five、
//           noexcept、moved-from state、copy elision / RVO
//
// 【詳細解釋 Explanation】
//
// 【1. 移動語義解決的問題：把「複製」換成「轉移所有權」】
// 在 C++11 之前，把一個持有資源的物件從 A 交給 B，只有一條路：深拷貝。
//     vector<string> makeData();
//     vector<string> v = makeData();   // C++03：可能複製整個 vector
// 但這裡的來源是個「馬上就要消失的臨時物件」——複製完立刻把原件銷毀，
// 等於做了一次完全多餘的深拷貝。
// 移動語義的洞見是：**當來源即將消亡時，直接把它的指標偷過來就好**。
//     移動建構：偷指標（O(1)）+ 把來源設為安全的空狀態
//     拷貝建構：配置新記憶體 + 複製全部內容（O(n)）
// 所以移動不是「更快的拷貝」，而是**語意完全不同的操作**：
// 拷貝後兩個物件都有效且相等；移動後來源進入「有效但未指定」的狀態。
//
// 【2. 值類別（value category）：右值引用變數本身是左值】
// 這是第 31 課最重要、也最反直覺的一點：
//     void f(MoveString&& s) {      // s 的「型別」是右值引用
//         MoveString t = s;         // 但這裡呼叫的是【拷貝】建構！
//         MoveString u = move(s);   // 要移動必須再 move 一次
//     }
// 原因是「型別」與「值類別」是兩件事：
//     * s 的型別是 MoveString&&（右值引用）
//     * 但運算式 `s` 本身有名字、可取位址 → 它是**左值**
// 一句話記法：**有名字的東西都是左值**，即使它的型別是右值引用。
// 這正是完美轉發需要 std::forward 的根本原因。
//
// 【3. 移動建構為什麼「必須」標 noexcept】
// 這是第 32 課看似細節、實則影響效能的關鍵。
// std::vector 擴容時要把舊元素搬到新記憶體，它面臨一個抉擇：
//     * 用移動：快，但萬一搬到一半拋出例外，舊的已被掏空、新的不完整
//       → 無法回復，強例外保證（strong exception guarantee）破功
//     * 用拷貝：慢，但拋出例外時舊資料完好無損 → 可以安全回復
// vector 用 std::move_if_noexcept 做這個判斷：
//     移動建構是 noexcept  → 用移動（快）
//     移動建構不是 noexcept → **退回用拷貝**（慢，但安全）
// 所以「忘了寫 noexcept」的後果不是編譯錯誤，而是**你的移動語義在 vector
// 擴容時被靜默地繞過**，效能回到 C++03。本檔會實測這個差異。
//
// 【4. 五法則（Rule of Five）與零法則（Rule of Zero）】
// 若你自己管理資源，五個特殊成員函式必須一起考慮：
//     ① 解構函式  ② 拷貝建構  ③ 拷貝賦值  ④ 移動建構  ⑤ 移動賦值
// 關鍵的連動規則（很多人不知道，會被靜默影響）：
//     * 只要你**自訂了解構函式、拷貝建構或拷貝賦值任一個**，
//       編譯器就**不再自動產生移動操作** → 所有「移動」會靜默退化成拷貝。
//     * 只要你**自訂了移動建構或移動賦值任一個**，
//       拷貝操作會被**隱式 delete**。
// 這代表「我只加了一個解構函式來印 log」這種無害改動，
// 可能讓整個類別的移動語義消失而毫無警告。
// 現代最佳實踐是**零法則（Rule of Zero）**：
// 讓成員自己是能正確拷貝／移動的型別（std::string、std::vector、
// std::unique_ptr），你一個特殊成員函式都不用寫，五個全部由編譯器正確產生。
//
// 【5. moved-from 狀態：「有效但未指定」的精確含義】
// 標準對被移走的物件的規定是：valid but unspecified state。
//   * 有效（valid）：可以安全地解構、可以安全地賦予新值。
//     這是為什麼移動建構裡必須把 other.m_data 設成 nullptr ——
//     否則來源解構時會 delete 一塊已經交出去的記憶體（double free）。
//   * 未指定（unspecified）：你**不可以假設**它是什麼。
//     標準函式庫的型別多半會讓 moved-from 的 string/vector 變成空的，
//     但那是實作提供的額外保證，不是標準的通用承諾。
//     所以正確的做法是：移動後除非先重新賦值，否則只呼叫
//     「無前置條件」的操作（如 clear()、operator=、解構）。
//   ⚠️ 「移動後一定是空字串」「移動後 size() 一定是 0」都是不該依賴的假設。
//
// 【概念補充 Concept Deep Dive】
// (A) copy elision 讓很多「移動」根本不會發生
//     `MoveString f() { return MoveString("x"); }  auto s = f();`
//     直覺上會有一次移動建構，但 C++17 起這屬於**保證的 copy elision**：
//     那個純右值直接就地建構在 s 的位置，移動建構**一次都不會被呼叫**。
//     所以量測移動語義的效能時，別被 elision 誤導 ——
//     它省下的比移動更多。（GCC/Clang 在 C++17 前就已用 RVO 做同樣的事，
//     但當時是「允許但不保證」，且仍要求移動建構可存取。）
//
// (B) 為什麼 return std::move(local) 是反模式
//     對回傳區域變數，編譯器本來就會嘗試 NRVO（具名回傳值最佳化）直接省掉複製；
//     若不行也會自動當成右值來移動。
//     手動寫 std::move 會把運算式變成右值引用，**反而阻止 NRVO**，
//     結果是「本來零次，現在一次移動」。準則：直接 `return local;`。
//
// (C) std::move 不移動任何東西
//     它只是一個 static_cast<T&&>，是純編譯期的型別轉換，不產生任何執行期程式碼。
//     真正做事的是隨後被選中的移動建構／移動賦值。
//     所以對沒有移動語義的型別（如 const 物件、或只有拷貝的舊類別）
//     用 std::move，會靜默地退回拷貝 —— 不會報錯，只是沒有加速。
//
// (D) 成員初始化順序仍依宣告順序（本檔保留了 -Wreorder 示範）
//     MoveString 的宣告順序是 m_data、m_len，
//     而拷貝建構的初始化列表寫成 m_len(other.m_len), m_data(...)。
//     實際初始化順序是 m_data 先、m_len 後。
//     這裡安全，是因為它用的是 **other.m_len**（來源物件，已完全初始化），
//     而不是自己尚未初始化的 m_len。
//     ⚠️ 若寫成 m_data(new char[m_len + 1]) 就會讀取未初始化的成員 → UB。
//     本檔刻意保留這個 -Wreorder 警告，讓你看到編譯器如何標示這個危險訊號；
//     實務上請讓兩者順序一致以消除警告。
//
// (E) noexcept 與移動賦值的自我賦值檢查
//     移動賦值同樣要處理 `a = std::move(a)` 這種自我移動。
//     多數實作用「先檢查 this != &other」或 copy-and-swap 來保證安全。
//     標準容器對自我移動只保證「有效但未指定」，不保證內容不變。
//
// 【注意事項 Pay Attention】
// 1. 有名字的東西都是左值 —— 型別為 T&& 的參數，用它時仍需 std::move。
// 2. 移動建構／移動賦值請一律標 noexcept，否則 vector 擴容會靜默退回拷貝。
// 3. 自訂解構／拷貝任一個，移動操作就不再自動產生（靜默退化成拷貝）。
// 4. moved-from 是「有效但未指定」；別假設它一定是空的，先賦值再使用。
// 5. std::move 只是 static_cast，本身不移動、不產生執行期程式碼。
// 6. 不要寫 `return std::move(local);` —— 會阻止 NRVO，反而變慢。
// 7. 移動建構中務必把來源的指標設為 nullptr，否則來源解構時會 double free。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動語義、右值引用與五法則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動建構函數為什麼一定要標 noexcept？不標會怎樣？
//     答：std::vector 擴容時用 std::move_if_noexcept 決定要移動還是拷貝舊元素。
//         若移動建構不是 noexcept，vector 為了維持強例外保證會**改用拷貝**
//         —— 你的移動語義被靜默繞過，效能退回 C++03，而且不會有任何警告。
//         這是「不標 noexcept」最實際的代價。
//     追問：為什麼移動一旦拋例外就無法維持強例外保證？
//         → 因為來源已經被掏空了。搬到一半失敗時，舊資料已殘缺、
//           新空間也不完整，無法回復到操作前的狀態。拷貝則不動來源，所以可以回復。
//
// 🔥 Q2. 這段程式碼裡，MoveString t = s; 呼叫的是移動還是拷貝？
//         void f(MoveString&& s) { MoveString t = s; }
//     答：**拷貝**。s 的型別是右值引用，但運算式 s 有名字、可取位址，
//         所以它的值類別是左值 → 多載決議選中拷貝建構。
//         要移動必須寫 MoveString t = std::move(s);
//     追問：那為什麼語言要這樣設計？
//         → 安全。若有名字的右值引用自動當右值用，
//           在函式中多次使用 s 時，第一次就會把它掏空，後續使用全部出錯。
//           要求顯式 std::move，等於強迫你明確標示「我之後不再用它了」。
//
// 🔥 Q3. 什麼是五法則？只自訂解構函式會發生什麼事？
//     答：五法則指解構、拷貝建構、拷貝賦值、移動建構、移動賦值這五個特殊成員函式，
//         自訂其一通常就該考慮全部。
//         只自訂解構函式的後果是：編譯器**不再自動產生移動建構與移動賦值**，
//         於是所有「移動」全部靜默退化成拷貝 —— 沒有編譯錯誤，只有效能損失。
//     追問：那有沒有更好的做法？
//         → 零法則（Rule of Zero）：改用 std::string / std::vector / unique_ptr
//           當成員，五個特殊成員函式一個都不用寫，編譯器會全部正確產生。
//
// ⚠️ 陷阱 1. 為什麼 `return std::move(local);` 反而比 `return local;` 慢？
//     答：直接 return 區域變數時，編譯器會先嘗試 NRVO（具名回傳值最佳化），
//         直接把它建構在呼叫端的回傳位置 —— **零次**建構搬移。
//         寫了 std::move 之後，運算式變成右值引用而非具名物件，NRVO 條件被破壞，
//         編譯器只能退而求其次執行一次移動建構。結果是把 0 次變成 1 次。
//     為什麼會錯：大家記住了「移動比拷貝快」，於是推論「加 move 一定不會更差」。
//         但漏掉的是還有一個比移動更快的選項 —— 完全不搬。
//         正確順序是：elision > 移動 > 拷貝，而手動 move 會把你從第一名踢到第二名。
//
// ⚠️ 陷阱 2. std::move 之後，原本的物件變成什麼？可以繼續用嗎？
//     答：進入「有效但未指定（valid but unspecified）」狀態。
//         可以安全解構、可以重新賦值；但**不可以假設它的內容**。
//         實務上 libstdc++ 的 std::string 被移走後常常是空的，
//         但那是實作細節，標準並未保證 —— 換個實作或換個型別就可能不同。
//     為什麼會錯：多數人把 std::move 理解成「搬走並清空」，
//         於是寫出 `s2 = std::move(s1); assert(s1.empty());` 這種依賴未指定行為的
//         程式碼。實際上 std::move 只是型別轉換，「來源變成什麼」完全取決於
//         被呼叫的那個移動建構／移動賦值怎麼寫的 —— 自訂類別若沒把來源清乾淨，
//         它甚至可能保持原樣。
//
// 【LeetCode 實戰範例】—— 從缺（刻意不加）
//     移動語義是「同一份程式碼怎麼跑得更快」的效能與資源管理主題，
//     LeetCode 的判題只驗證回傳值是否正確，不量測你有沒有省下一次深拷貝，
//     也沒有題目在考右值引用或五法則。
//     （雖然在解題時回傳大型 vector 確實受惠於移動與 RVO，
//       但那是語言自動幫你做的，不構成一道「用到本主題」的題目。）
//     本檔改以「訊息佇列搬移大型 payload」這個真實情境，
//     實測移動與拷貝的次數差異。硬湊一題不相關的比沒有更糟。
// ═══════════════════════════════════════════════════════════════════════════

// ============================================================
// 第 31 課：右值引用（Rvalue Reference）入門
// ============================================================
// 【核心觀念】
// Lvalue（左值）：有名字、可取位址的表達式 → int x = 10; x 是左值
// Rvalue（右值）：臨時的、沒有名字的表達式 → 10, x+y, func() 回傳的臨時物件
//
// 【引用類型與綁定規則】
//   T&        左值引用     → 只能綁定左值
//   const T&  const左值引用 → 可綁定左值和右值（萬能引用讀取）
//   T&&       右值引用     → 只能綁定右值
//
// 【重要陷阱】右值引用變數本身是左值！
//   int&& rref = 42;   // rref 綁定右值 42
//   // 但 rref 本身有名字、可取位址 → rref 是左值！
//   // int&& rref2 = rref;  // ❌ 錯誤！rref 是左值，不能綁定到 T&&
//   // int&& rref2 = std::move(rref);  // ✅ 用 std::move 轉成右值

namespace Lesson31 {

void processLvalue(int& x)        { cout << "  左值引用版本: " << x << endl; }
void processRvalue(int&& x)       { cout << "  右值引用版本: " << x << endl; }

// 函數重載：const T& vs T&& → 編譯器優先匹配 T&&
void overloaded(const string& s)  { cout << "  const T& 版本: " << s << endl; }
void overloaded(string&& s)       { cout << "  T&& 版本: " << s << endl; }

void demo() {
    cout << "=== 第 31 課：右值引用入門 ===" << endl;

    // 綁定規則
    cout << "\n--- 綁定規則 ---" << endl;
    int a = 10;
    int& lref = a;           // 左值引用 → 綁定左值 ✅
    const int& clref = 42;   // const 左值引用 → 綁定右值 ✅（萬能）
    int&& rref = 42;         // 右值引用 → 綁定右值 ✅
    // 實際印出來，順便證明三者都真的綁上了
    cout << "  int&       lref = a;   → " << lref  << "（綁左值）" << endl;
    cout << "  const int& clref = 42; → " << clref << "（const 左值引用可綁右值）" << endl;
    cout << "  int&&      rref = 42;  → " << rref  << "（右值引用綁右值）" << endl;
    // int& bad = 42;        // ❌ 非 const 左值引用不能綁右值

    processLvalue(a);         // 左值 → T&
    processRvalue(100);       // 右值 → T&&
    // processRvalue(a);      // ❌ 左值不能綁到 T&&
    processRvalue(move(a));   // ✅ std::move 把左值轉成右值

    // 重要陷阱：右值引用變數本身是左值
    cout << "\n--- 右值引用變數是左值 ---" << endl;
    int&& r = 42;
    cout << "  r 的值: " << r << endl;
    cout << "  &r = " << &r << " ← 可取位址，所以 r 是左值" << endl;
    // int&& r2 = r;    // ❌ 編譯錯誤！r 是左值
    int&& r2 = move(r); // ✅ 需要 std::move
    cout << "  r2 = " << r2 << endl;

    // 函數重載選擇
    cout << "\n--- 函數重載：const T& vs T&& ---" << endl;
    string name = "Alice";
    overloaded(name);              // 左值 → const T& 版本
    overloaded("Bob"s);            // 右值 → T&& 版本（優先匹配）
    overloaded(move(name));        // move 後 → T&& 版本
}

} // namespace Lesson31

// ============================================================
// 第 32 課：移動建構函數（Move Constructor）
// ============================================================
// 【核心觀念】
// 簽名：ClassName(ClassName&& other) noexcept
//
// 移動三步驟：
//   1. 偷取（steal）：把 other 的指標複製給自己
//   2. 歸零（nullify）：把 other 的指標設為 nullptr
//   3. 複製基本型別：直接複製 int, size_t 等
//
// 移動後的 other 處於「有效但未指定」狀態（valid but unspecified）
// → other 可以安全解構（delete nullptr 是安全的）
//
// 【noexcept 很重要！】
//   vector 擴容時，如果移動建構沒有 noexcept，vector 會用拷貝而非移動
//   → 因為 vector 需要強異常安全保證
//
// 【自動生成規則】
//   如果定義了自訂解構函數、拷貝建構或拷貝賦值 → 編譯器不會自動生成移動建構
//   → 這就是為什麼需要「五法則」

namespace Lesson32 {

class MoveString {
    // ⚠️ 宣告順序就是初始化順序（與初始化列表的書寫順序無關）。
    //    長度必須宣告在指標【之前】，否則像 m_data(new char[m_len + 1]) 這種寫法
    //    會用到還沒初始化的 m_len。本檔目前是安全的（初始化列表讀的是 other.m_len，
    //    不是自己的成員），但把順序擺正可以讓這個陷阱從一開始就不存在。
    size_t m_len;
    char* m_data;
public:
    MoveString(const char* str = "") : m_len(strlen(str)) {
        m_data = new char[m_len + 1];
        strcpy(m_data, str);
        cout << "  [建構] \"" << m_data << "\"" << endl;
    }

    // 拷貝建構（深拷貝）— 昂貴
    MoveString(const MoveString& other)
        : m_len(other.m_len), m_data(new char[other.m_len + 1]) {
        strcpy(m_data, other.m_data);
        cout << "  [拷貝建構💰] \"" << m_data << "\"" << endl;
    }

    // 移動建構 — 便宜！只是偷指標
    MoveString(MoveString&& other) noexcept  // ← noexcept 很重要！
          : m_len(other.m_len),                // 初始化列表順序要與【宣告順序】一致
            m_data(other.m_data)               //   （真正的初始化順序永遠照宣告順序，
                                               //     不照這裡的書寫順序 → 不一致會觸發 -Wreorder）
    {
        other.m_data = nullptr;               // ② 歸零源物件
        other.m_len = 0;
        cout << "  [移動建構⚡] \"" << m_data << "\"" << endl;
    }

    ~MoveString() {
        if (m_data) cout << "  [解構] \"" << m_data << "\"" << endl;
        else        cout << "  [解構] (已被移動，nullptr)" << endl;
        delete[] m_data;  // delete nullptr 是安全的
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

void demo() {
    cout << "\n=== 第 32 課：移動建構函數 ===" << endl;

    // 基本移動
    cout << "\n--- 基本移動 ---" << endl;
    MoveString a("Hello");
    MoveString b(move(a));   // 移動建構：偷取 a 的資源
    cout << "  a: " << a.c_str() << endl;  // (null) — 已被移動
    cout << "  b: " << b.c_str() << endl;  // Hello

    // noexcept 對 vector 的影響
    cout << "\n--- noexcept 對 vector 的影響 ---" << endl;
    cout << "  有 noexcept → vector 擴容時使用移動（快）" << endl;
    cout << "  無 noexcept → vector 擴容時使用拷貝（慢）" << endl;
    vector<MoveString> vec;
    vec.reserve(2);  // 預留空間避免干擾
    vec.push_back(MoveString("Vec1"));  // 移動臨時物件
    vec.push_back(MoveString("Vec2"));  // 移動臨時物件

    // 自動生成規則
    cout << "\n--- 自動生成規則 ---" << endl;
    cout << "  定義了解構函數    → 不自動生成移動建構/賦值" << endl;
    cout << "  定義了拷貝建構    → 不自動生成移動建構/賦值" << endl;
    cout << "  定義了拷貝賦值    → 不自動生成移動建構/賦值" << endl;
    cout << "  → 所以要嘛全不寫（Rule of Zero），要嘛五個都寫（Rule of Five）" << endl;
}

} // namespace Lesson32

// ============================================================
// 第 33 課：移動賦值運算子（Move Assignment Operator）
// ============================================================
// 【核心觀念】
// 簽名：ClassName& operator=(ClassName&& other) noexcept
//
// 移動賦值步驟：
//   1. 自我賦值檢查：if (this == &other) return *this;
//   2. 釋放自己的舊資源：delete[] m_data;
//   3. 偷取 other 的資源：m_data = other.m_data;
//   4. 歸零 other：other.m_data = nullptr;
//
// 【統一賦值運算子（Unified Assignment）】推薦寫法
//   ClassName& operator=(ClassName other) {  // 傳值
//       swap(*this, other);
//       return *this;
//   }
//   → 一個函數同時處理拷貝賦值和移動賦值！
//   → 傳左值時：觸發拷貝建構 → swap
//   → 傳右值時：觸發移動建構 → swap

namespace Lesson33 {

class Buffer {
    int* m_data;
    size_t m_size;
public:
    Buffer(size_t size = 0) : m_data(size ? new int[size]{} : nullptr), m_size(size) {
        cout << "  [Buffer 建構] size=" << m_size << endl;
    }

    // 拷貝建構
    Buffer(const Buffer& other)
        : m_data(other.m_size ? new int[other.m_size] : nullptr),
          m_size(other.m_size) {
        if (m_data) copy(other.m_data, other.m_data + m_size, m_data);
        cout << "  [Buffer 拷貝建構💰]" << endl;
    }

    // 移動建構
    Buffer(Buffer&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size) {
        other.m_data = nullptr;
        other.m_size = 0;
        cout << "  [Buffer 移動建構⚡]" << endl;
    }

    // --- 方法一：分開寫拷貝賦值和移動賦值 ---
    // 拷貝賦值
    // Buffer& operator=(const Buffer& other) { ... }
    // 移動賦值
    // Buffer& operator=(Buffer&& other) noexcept { ... }

    // --- 方法二：統一賦值（推薦）---
    friend void swap(Buffer& a, Buffer& b) noexcept {
        using std::swap;
        swap(a.m_data, b.m_data);
        swap(a.m_size, b.m_size);
    }

    Buffer& operator=(Buffer other) {  // 傳值！
        // 傳左值 → other 由拷貝建構產生
        // 傳右值 → other 由移動建構產生
        cout << "  [Buffer 統一賦值] swap" << endl;
        swap(*this, other);
        return *this;
    } // other 離開作用域，自動釋放舊資源

    ~Buffer() {
        delete[] m_data;
    }

    void fill(int val) { for (size_t i = 0; i < m_size; ++i) m_data[i] = val; }
    int get(size_t i) const { return m_data ? m_data[i] : -1; }
    size_t size() const { return m_size; }
};

void demo() {
    cout << "\n=== 第 33 課：移動賦值運算子 ===" << endl;

    cout << "\n--- 統一賦值同時處理拷貝和移動 ---" << endl;
    Buffer a(3);
    a.fill(10);

    // 拷貝賦值路徑：傳左值 → 拷貝建構 → swap
    cout << "\n  拷貝賦值路徑（左值）：" << endl;
    Buffer b(2);
    b = a;  // a 是左值 → 拷貝建構 other → swap
    cout << "  b[0]=" << b.get(0) << endl;  // 10

    // 移動賦值路徑：傳右值 → 移動建構 → swap
    cout << "\n  移動賦值路徑（右值）：" << endl;
    Buffer c(2);
    c = move(a);  // move(a) 是右值 → 移動建構 other → swap
    cout << "  c[0]=" << c.get(0) << endl;  // 10
    cout << "  a.size()=" << a.size() << " (a 被交換為 c 的舊狀態)" << endl;
}

} // namespace Lesson33

// ============================================================
// 第 34 課：五法則（Rule of Five）
// ============================================================
// 【核心觀念】
// Rule of Five = Rule of Three + 移動建構 + 移動賦值
//   1. 解構函數           ~ClassName()
//   2. 拷貝建構函數       ClassName(const ClassName&)
//   3. 拷貝賦值運算子     ClassName& operator=(const ClassName&)
//   4. 移動建構函數       ClassName(ClassName&&) noexcept
//   5. 移動賦值運算子     ClassName& operator=(ClassName&&) noexcept
//
// 若使用統一賦值（pass-by-value + swap），只需要 4 個：
//   解構 + 拷貝建構 + 移動建構 + 統一賦值
//
// 【Rule of Zero】
//   如果類別只使用 RAII 成員（string, vector, unique_ptr...）
//   → 不需要自訂任何一個，全部用編譯器預設的
//
// 【type_traits 檢查】
//   is_copy_constructible_v<T>   是否可拷貝建構
//   is_move_constructible_v<T>   是否可移動建構
//   is_copy_assignable_v<T>      是否可拷貝賦值
//   is_move_assignable_v<T>      是否可移動賦值
//   is_destructible_v<T>         是否可解構

namespace Lesson34 {

class ManagedArray {
    int* m_data;
    size_t m_size;

public:
    // 建構函數
    explicit ManagedArray(size_t size = 0)
        : m_data(size ? new int[size]{} : nullptr), m_size(size) {}

    // ===== Rule of Five（使用統一賦值，實際只寫 4 個）=====

    // ① 解構函數
    ~ManagedArray() {
        delete[] m_data;
    }

    // ② 拷貝建構函數（深拷貝）
    ManagedArray(const ManagedArray& other)
        : m_data(other.m_size ? new int[other.m_size] : nullptr),
          m_size(other.m_size) {
        if (m_data) copy(other.m_data, other.m_data + m_size, m_data);
    }

    // ③ 移動建構函數（偷資源）
    ManagedArray(ManagedArray&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size) {
        other.m_data = nullptr;
        other.m_size = 0;
    }

    // ④⑤ 統一賦值運算子（同時處理拷貝賦值和移動賦值）
    friend void swap(ManagedArray& a, ManagedArray& b) noexcept {
        using std::swap;
        swap(a.m_data, b.m_data);
        swap(a.m_size, b.m_size);
    }

    ManagedArray& operator=(ManagedArray other) {  // 傳值
        swap(*this, other);
        return *this;
    }

    // ===== Rule of Five 結束 =====

    size_t size() const { return m_size; }
    int& operator[](size_t i) { return m_data[i]; }
    const int& operator[](size_t i) const { return m_data[i]; }
};

void demo() {
    cout << "\n=== 第 34 課：五法則（Rule of Five）===" << endl;

    // 完整示範
    cout << "\n--- 完整 Rule of Five 類別使用 ---" << endl;
    ManagedArray a(3);
    a[0] = 10; a[1] = 20; a[2] = 30;

    ManagedArray b(a);          // 拷貝建構
    ManagedArray c(move(a));    // 移動建構（a 被清空）
    ManagedArray d(5);
    d = b;                      // 統一賦值（拷貝路徑）
    ManagedArray e(5);
    e = move(b);                // 統一賦值（移動路徑）

    cout << "  c[0]=" << c[0] << " c[1]=" << c[1] << " c[2]=" << c[2] << endl;
    cout << "  d[0]=" << d[0] << " d[1]=" << d[1] << " d[2]=" << d[2] << endl;
    cout << "  a.size()=" << a.size() << " (已被移動)" << endl;
    cout << "  b.size()=" << b.size() << " (已被移動)" << endl;

    // type_traits 檢查
    cout << "\n--- type_traits 檢查 ---" << endl;
    cout << boolalpha;
    cout << "  is_copy_constructible: " << is_copy_constructible_v<ManagedArray> << endl;
    cout << "  is_move_constructible: " << is_move_constructible_v<ManagedArray> << endl;
    cout << "  is_copy_assignable:    " << is_copy_assignable_v<ManagedArray> << endl;
    cout << "  is_move_assignable:    " << is_move_assignable_v<ManagedArray> << endl;
    cout << "  is_destructible:       " << is_destructible_v<ManagedArray> << endl;

    // Rule of Zero 說明
    cout << "\n--- Rule of Zero ---" << endl;
    cout << "  如果類別只用 string, vector, unique_ptr 等 RAII 成員" << endl;
    cout << "  → 不需要寫任何一個（解構、拷貝、移動）" << endl;
    cout << "  → 編譯器預設的就是正確的" << endl;

    struct SafeClass {   // Rule of Zero 範例
        string name;     // RAII 成員，自動管理記憶體
        vector<int> data;
        // 不需要寫解構、拷貝建構、拷貝賦值、移動建構、移動賦值！
    };
    SafeClass s1{"Hello", {1,2,3}};
    SafeClass s2 = s1;           // 安全拷貝
    SafeClass s3 = move(s1);     // 安全移動
    cout << "  SafeClass s2.name=" << s2.name << " s3.name=" << s3.name << endl;
}

} // namespace Lesson34

// ============================================================
// 第 35 課：std::move 的使用
// ============================================================
// 【核心觀念】
// std::move(x) 本身不移動任何東西！
// 它只是一個 static_cast<T&&>(x)，把左值轉成右值引用
// 真正的移動發生在接收端的移動建構/移動賦值中
//
// 【五大使用場景】
//  1. 明確不再需要的局部變數 → 傳給函數或容器
//  2. 建構函數中的 pass-by-value + move 慣用法
//  3. push_back(move(obj)) 避免不必要的拷貝
//  4. 從容器中提取元素 → auto item = move(container.back())
//  5. 實現 swap → 透過三次移動交換兩個物件
//
// 【注意事項】
//  - 不要 move const 物件 → const T&& 無法觸發移動，會退化為拷貝
//  - 不要 move 函數回傳的局部變數 → 會阻止 NRVO 優化
//  - move 後的物件處於有效但未指定狀態，不要再使用其值

namespace Lesson35 {

class TrackedString {
    string m_data;
    static int s_copyCount;
    static int s_moveCount;
public:
    TrackedString(const string& s = "") : m_data(s) {}

    TrackedString(const TrackedString& other) : m_data(other.m_data) {
        ++s_copyCount;
        cout << "  [TrackedString 拷貝💰] \"" << m_data << "\"" << endl;
    }

    TrackedString(TrackedString&& other) noexcept : m_data(move(other.m_data)) {
        ++s_moveCount;
        cout << "  [TrackedString 移動⚡] \"" << m_data << "\"" << endl;
    }

    TrackedString& operator=(TrackedString other) {
        swap(m_data, other.m_data);
        return *this;
    }

    const string& str() const { return m_data; }

    static void resetCounters() { s_copyCount = s_moveCount = 0; }
    static void printCounters() {
        cout << "  拷貝次數: " << s_copyCount
             << "  移動次數: " << s_moveCount << endl;
    }
};
int TrackedString::s_copyCount = 0;
int TrackedString::s_moveCount = 0;

// --- 場景 2：建構函數中 pass-by-value + move ---
class Hero {
    TrackedString m_name;
    int m_hp;
public:
    // 傳值 + move 慣用法
    // 傳左值：拷貝到 name → move 到 m_name（1 拷貝 + 1 移動）
    // 傳右值：移動到 name → move 到 m_name（2 移動，零拷貝）
    Hero(TrackedString name, int hp)
        : m_name(move(name)),  // ← move 傳值參數到成員
          m_hp(hp) {}

    void print() const {
        cout << "  Hero: " << m_name.str() << " HP=" << m_hp << endl;
    }
};

void demo() {
    cout << "\n=== 第 35 課：std::move 的使用 ===" << endl;

    // 場景 1：明確不再需要的局部變數
    cout << "\n--- 場景 1：移動局部變數到容器 ---" << endl;
    TrackedString::resetCounters();
    vector<TrackedString> party;
    party.reserve(3);  // 預留空間避免擴容干擾

    TrackedString warrior("Warrior");
    party.push_back(warrior);         // 拷貝（warrior 之後還要用）
    party.push_back(move(warrior));   // 移動（warrior 之後不用了）
    cout << "  warrior 移動後: \"" << warrior.str() << "\" (空字串)" << endl;
    TrackedString::printCounters();

    // 場景 2：pass-by-value + move 建構函數
    cout << "\n--- 場景 2：pass-by-value + move ---" << endl;
    TrackedString::resetCounters();
    cout << "  傳左值：" << endl;
    TrackedString name1("Alice");
    Hero h1(name1, 100);      // 拷貝到參數 → move 到成員
    TrackedString::printCounters();

    TrackedString::resetCounters();
    cout << "  傳右值：" << endl;
    Hero h2(TrackedString("Bob"), 200);  // 移動到參數 → move 到成員
    TrackedString::printCounters();

    // 場景 3：push_back(move(x)) vs emplace_back
    cout << "\n--- 場景 3：push_back vs emplace_back ---" << endl;
    cout << "  push_back(obj)       → 拷貝" << endl;
    cout << "  push_back(move(obj)) → 移動" << endl;
    cout << "  emplace_back(args)   → 原地建構（最佳，零拷貝零移動）" << endl;
    vector<string> names;
    names.reserve(3);
    string s = "Charlie";
    names.push_back(s);           // 拷貝
    names.push_back(move(s));     // 移動
    names.emplace_back("David");  // 原地建構（直接在 vector 記憶體中建構）

    // 場景 4：從容器提取元素
    cout << "\n--- 場景 4：從容器提取元素 ---" << endl;
    vector<string> items = {"Sword", "Shield", "Potion"};
    string extracted = move(items.back());  // 移動而非拷貝
    items.pop_back();
    cout << "  提取: " << extracted << endl;
    cout << "  剩餘 items.size()=" << items.size() << endl;

    // 場景 5：用 move 實現 swap
    cout << "\n--- 場景 5：用 move 實現 swap ---" << endl;
    string x = "AAA", y = "BBB";
    cout << "  交換前: x=" << x << " y=" << y << endl;
    // std::swap 的內部實現大致如下：
    // string temp = move(x);  // ① x → temp
    // x = move(y);            // ② y → x
    // y = move(temp);         // ③ temp → y
    swap(x, y);
    cout << "  交換後: x=" << x << " y=" << y << endl;

    // 注意事項
    cout << "\n--- 注意事項 ---" << endl;
    cout << "  ❌ 不要 move const 物件 → 會退化為拷貝" << endl;
    cout << "     const string cs = \"hi\";" << endl;
    cout << "     string s2 = move(cs);  // 實際上是拷貝！" << endl;
    cout << "  ❌ 不要 move 函數回傳的局部變數 → 會阻止 NRVO" << endl;
    cout << "     return move(localObj);  // 不好！阻止編譯器優化" << endl;
    cout << "     return localObj;        // 好！讓編譯器做 NRVO" << endl;
    cout << "  ❌ move 後不要再使用物件的值（可以重新賦值或解構）" << endl;

    // 驗證 const move 退化為拷貝
    cout << "\n--- 驗證 const move 退化為拷貝 ---" << endl;
    TrackedString::resetCounters();
    const TrackedString constStr("Immovable");
    TrackedString copied = move(constStr);  // const → 無法移動 → 退化為拷貝
    TrackedString::printCounters();  // 拷貝: 1, 移動: 0
}

} // namespace Lesson35

// ============================================================
// main
// ============================================================
// 【日常實務範例】訊息佇列：搬移大型 payload，以及 noexcept 的真實代價
// ============================================================
//   情境：服務內部有一個訊息佇列，每則訊息帶著一份可能很大的 payload
//         （JSON 字串、影像位元組、序列化後的封包）。
//   實務問題：
//     (a) 訊息從產生端交給佇列時，若走拷貝，等於把整份 payload 複製一次
//     (b) 佇列（vector）擴容時，會把既有元素全部搬到新記憶體
//         —— 這時「移動建構有沒有標 noexcept」會直接決定是搬移還是複製
//   本範例用計數器實測兩種情況的拷貝／移動次數，證明前面的論述不是紙上談兵。
//
//   注意：這裡刻意「不」用 reserve()，就是為了觸發擴容並觀察搬移行為。
//         實務上若已知大小，reserve() 才是第一優先的最佳化。
// ============================================================
namespace Practical {

// 計數器：記錄各類操作發生幾次
struct Stats {
    int copyCtor = 0;
    int moveCtor = 0;
    void reset() { copyCtor = moveCtor = 0; }
};

// 版本 A：移動建構標了 noexcept（正確做法）
class MsgNoexcept {
    std::string m_payload;
    static Stats s_stats;
public:
    explicit MsgNoexcept(std::string p) : m_payload(std::move(p)) {}
    MsgNoexcept(const MsgNoexcept& o) : m_payload(o.m_payload) { ++s_stats.copyCtor; }
    MsgNoexcept(MsgNoexcept&& o) noexcept : m_payload(std::move(o.m_payload)) { ++s_stats.moveCtor; }
    MsgNoexcept& operator=(const MsgNoexcept&) = default;
    MsgNoexcept& operator=(MsgNoexcept&&) noexcept = default;
    ~MsgNoexcept() = default;
    static Stats& stats() { return s_stats; }
    size_t size() const { return m_payload.size(); }
};
Stats MsgNoexcept::s_stats;

// 版本 B：移動建構「沒有」標 noexcept（常見疏忽）
class MsgThrowing {
    std::string m_payload;
    static Stats s_stats;
public:
    explicit MsgThrowing(std::string p) : m_payload(std::move(p)) {}
    MsgThrowing(const MsgThrowing& o) : m_payload(o.m_payload) { ++s_stats.copyCtor; }
    MsgThrowing(MsgThrowing&& o) : m_payload(std::move(o.m_payload)) { ++s_stats.moveCtor; }  // 少了 noexcept
    MsgThrowing& operator=(const MsgThrowing&) = default;
    MsgThrowing& operator=(MsgThrowing&&) = default;
    ~MsgThrowing() = default;
    static Stats& stats() { return s_stats; }
    size_t size() const { return m_payload.size(); }
};
Stats MsgThrowing::s_stats;

template <typename Msg>
static void fillQueue(const char* label, int n) {
    Msg::stats().reset();
    std::vector<Msg> queue;                 // 刻意不 reserve，讓它自然擴容
    for (int i = 0; i < n; ++i) {
        queue.push_back(Msg(std::string(1024, 'x')));   // 每則 1KB payload
    }
    cout << "    " << label
         << "  拷貝建構 " << Msg::stats().copyCtor
         << " 次，移動建構 " << Msg::stats().moveCtor << " 次" << endl;
}

void demo() {
    cout << "\n=== 日常實務：訊息佇列與 noexcept 的真實代價 ===" << endl;

    cout << "\n  --- 1) std::move 把 payload 交給訊息，避免深拷貝 ---" << endl;
    std::string payload(2048, 'A');
    cout << "    交出前 payload.size() = " << payload.size() << endl;
    MsgNoexcept m(std::move(payload));      // 所有權轉移，不複製 2KB
    cout << "    訊息內 payload 大小     = " << m.size() << endl;
    cout << "    交出後 payload.size()   = " << payload.size()
         << "  ← libstdc++ 實測為 0，但標準只保證「有效但未指定」，不可依賴" << endl;

    cout << "\n  --- 2) vector 擴容：noexcept 決定搬移或複製 ---" << endl;
    const int N = 8;
    cout << "    連續 push_back " << N << " 則訊息（不 reserve，觀察擴容行為）：" << endl;
    fillQueue<MsgNoexcept>("移動建構有 noexcept：", N);
    fillQueue<MsgThrowing>("移動建構無 noexcept：", N);
    cout << "    → 兩者 push_back 本身都是移動；差別在【擴容時搬移既有元素】。" << endl;
    cout << "      有 noexcept → vector 用移動搬（快）；" << endl;
    cout << "      無 noexcept → vector 為維持強例外保證，改用拷貝（慢）。" << endl;
    cout << "      這個退化是靜默的：不會有編譯錯誤，也不會有警告。" << endl;

    cout << "\n  --- 3) 編譯期驗證：type traits 看得到 noexcept 的差別 ---" << endl;
    cout << boolalpha;
    cout << "    is_nothrow_move_constructible<MsgNoexcept> = "
         << std::is_nothrow_move_constructible<MsgNoexcept>::value << endl;
    cout << "    is_nothrow_move_constructible<MsgThrowing> = "
         << std::is_nothrow_move_constructible<MsgThrowing>::value << endl;
    cout << "    → vector 正是靠這個 trait（透過 std::move_if_noexcept）做決定的。" << endl;
}

} // namespace Practical

// ============================================================
int main() {
    Lesson31::demo();
    Lesson32::demo();
    Lesson33::demo();
    Lesson34::demo();
    Lesson35::demo();
    Practical::demo();

    cout << "\n========================================" << endl;
    cout << "第 31～35 課總結完畢（移動語義）" << endl;
    cout << "========================================" << endl;
    cout << "\n--- 完整知識路線圖 ---" << endl;
    cout << "第 27 課：淺拷貝 vs 深拷貝 → 認識問題" << endl;
    cout << "第 28 課：拷貝建構函數     → 解決「初始化」時的拷貝" << endl;
    cout << "第 29 課：拷貝賦值運算子   → 解決「賦值」時的拷貝" << endl;
    cout << "第 30 課：三法則           → 統整拷貝語義" << endl;
    cout << "第 31 課：右值引用         → 為移動語義鋪路" << endl;
    cout << "第 32 課：移動建構函數     → 偷資源代替深拷貝" << endl;
    cout << "第 33 課：移動賦值運算子   → 賦值時也能偷資源" << endl;
    cout << "第 34 課：五法則           → 統整拷貝+移動語義" << endl;
    cout << "第 35 課：std::move        → 實戰使用技巧" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary27-35-移動語義.cpp -o move_semantics
//   註：本檔保留 MoveString 的 -Wreorder 警告作為教學示範（見概念補充 D）。
//       該警告不影響正確性 —— 拷貝建構讀的是 other.m_len（來源物件，已完全初始化）。

//   ⚠️ 非決定性內容：輸出中所有 @0x... 的記憶體位址（以及 &r 這類位址）
//      **每次執行都不同**（作業系統的 ASLR 會隨機化堆積與堆疊位置）。
//      下方貼的是某一次實跑的結果；請只比對「位址之間的關係」
//      （例如深拷貝的來源與目的位址不同、移動後來源變成 0 或 nullptr），
//      不要比對具體數值。
// === 預期輸出 ===
// === 第 31 課：右值引用入門 ===
//
// --- 綁定規則 ---
//   int&       lref = a;   → 10（綁左值）
//   const int& clref = 42; → 42（const 左值引用可綁右值）
//   int&&      rref = 42;  → 42（右值引用綁右值）
//   左值引用版本: 10
//   右值引用版本: 100
//   右值引用版本: 10
//
// --- 右值引用變數是左值 ---
//   r 的值: 42
//   &r = 0x7ffe5dd239e4 ← 可取位址，所以 r 是左值
//   r2 = 42
//
// --- 函數重載：const T& vs T&& ---
//   const T& 版本: Alice
//   T&& 版本: Bob
//   T&& 版本: Alice
//
// === 第 32 課：移動建構函數 ===
//
// --- 基本移動 ---
//   [建構] "Hello"
//   [移動建構⚡] "Hello"
//   a: (null)
//   b: Hello
//
// --- noexcept 對 vector 的影響 ---
//   有 noexcept → vector 擴容時使用移動（快）
//   無 noexcept → vector 擴容時使用拷貝（慢）
//   [建構] "Vec1"
//   [移動建構⚡] "Vec1"
//   [解構] (已被移動，nullptr)
//   [建構] "Vec2"
//   [移動建構⚡] "Vec2"
//   [解構] (已被移動，nullptr)
//
// --- 自動生成規則 ---
//   定義了解構函數    → 不自動生成移動建構/賦值
//   定義了拷貝建構    → 不自動生成移動建構/賦值
//   定義了拷貝賦值    → 不自動生成移動建構/賦值
//   → 所以要嘛全不寫（Rule of Zero），要嘛五個都寫（Rule of Five）
//   [解構] "Vec1"
//   [解構] "Vec2"
//   [解構] "Hello"
//   [解構] (已被移動，nullptr)
//
// === 第 33 課：移動賦值運算子 ===
//
// --- 統一賦值同時處理拷貝和移動 ---
//   [Buffer 建構] size=3
//
//   拷貝賦值路徑（左值）：
//   [Buffer 建構] size=2
//   [Buffer 拷貝建構💰]
//   [Buffer 統一賦值] swap
//   b[0]=10
//
//   移動賦值路徑（右值）：
//   [Buffer 建構] size=2
//   [Buffer 移動建構⚡]
//   [Buffer 統一賦值] swap
//   c[0]=10
//   a.size()=0 (a 被交換為 c 的舊狀態)
//
// === 第 34 課：五法則（Rule of Five）===
//
// --- 完整 Rule of Five 類別使用 ---
//   c[0]=10 c[1]=20 c[2]=30
//   d[0]=10 d[1]=20 d[2]=30
//   a.size()=0 (已被移動)
//   b.size()=0 (已被移動)
//
// --- type_traits 檢查 ---
//   is_copy_constructible: true
//   is_move_constructible: true
//   is_copy_assignable:    true
//   is_move_assignable:    true
//   is_destructible:       true
//
// --- Rule of Zero ---
//   如果類別只用 string, vector, unique_ptr 等 RAII 成員
//   → 不需要寫任何一個（解構、拷貝、移動）
//   → 編譯器預設的就是正確的
//   SafeClass s2.name=Hello s3.name=Hello
//
// === 第 35 課：std::move 的使用 ===
//
// --- 場景 1：移動局部變數到容器 ---
//   [TrackedString 拷貝💰] "Warrior"
//   [TrackedString 移動⚡] "Warrior"
//   warrior 移動後: "" (空字串)
//   拷貝次數: 1  移動次數: 1
//
// --- 場景 2：pass-by-value + move ---
//   傳左值：
//   [TrackedString 拷貝💰] "Alice"
//   [TrackedString 移動⚡] "Alice"
//   拷貝次數: 1  移動次數: 1
//   傳右值：
//   [TrackedString 移動⚡] "Bob"
//   拷貝次數: 0  移動次數: 1
//
// --- 場景 3：push_back vs emplace_back ---
//   push_back(obj)       → 拷貝
//   push_back(move(obj)) → 移動
//   emplace_back(args)   → 原地建構（最佳，零拷貝零移動）
//
// --- 場景 4：從容器提取元素 ---
//   提取: Potion
//   剩餘 items.size()=2
//
// --- 場景 5：用 move 實現 swap ---
//   交換前: x=AAA y=BBB
//   交換後: x=BBB y=AAA
//
// --- 注意事項 ---
//   ❌ 不要 move const 物件 → 會退化為拷貝
//      const string cs = "hi";
//      string s2 = move(cs);  // 實際上是拷貝！
//   ❌ 不要 move 函數回傳的局部變數 → 會阻止 NRVO
//      return move(localObj);  // 不好！阻止編譯器優化
//      return localObj;        // 好！讓編譯器做 NRVO
//   ❌ move 後不要再使用物件的值（可以重新賦值或解構）
//
// --- 驗證 const move 退化為拷貝 ---
//   [TrackedString 拷貝💰] "Immovable"
//   拷貝次數: 1  移動次數: 0
//
// === 日常實務：訊息佇列與 noexcept 的真實代價 ===
//
//   --- 1) std::move 把 payload 交給訊息，避免深拷貝 ---
//     交出前 payload.size() = 2048
//     訊息內 payload 大小     = 2048
//     交出後 payload.size()   = 0  ← libstdc++ 實測為 0，但標準只保證「有效但未指定」，不可依賴
//
//   --- 2) vector 擴容：noexcept 決定搬移或複製 ---
//     連續 push_back 8 則訊息（不 reserve，觀察擴容行為）：
//     移動建構有 noexcept：  拷貝建構 0 次，移動建構 15 次
//     移動建構無 noexcept：  拷貝建構 7 次，移動建構 8 次
//     → 兩者 push_back 本身都是移動；差別在【擴容時搬移既有元素】。
//       有 noexcept → vector 用移動搬（快）；
//       無 noexcept → vector 為維持強例外保證，改用拷貝（慢）。
//       這個退化是靜默的：不會有編譯錯誤，也不會有警告。
//
//   --- 3) 編譯期驗證：type traits 看得到 noexcept 的差別 ---
//     is_nothrow_move_constructible<MsgNoexcept> = true
//     is_nothrow_move_constructible<MsgThrowing> = false
//     → vector 正是靠這個 trait（透過 std::move_if_noexcept）做決定的。
//
// ========================================
// 第 31～35 課總結完畢（移動語義）
// ========================================
//
// --- 完整知識路線圖 ---
// 第 27 課：淺拷貝 vs 深拷貝 → 認識問題
// 第 28 課：拷貝建構函數     → 解決「初始化」時的拷貝
// 第 29 課：拷貝賦值運算子   → 解決「賦值」時的拷貝
// 第 30 課：三法則           → 統整拷貝語義
// 第 31 課：右值引用         → 為移動語義鋪路
// 第 32 課：移動建構函數     → 偷資源代替深拷貝
// 第 33 課：移動賦值運算子   → 賦值時也能偷資源
// 第 34 課：五法則           → 統整拷貝+移動語義
// 第 35 課：std::move        → 實戰使用技巧
