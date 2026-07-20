// =============================================================================
//  第 31 課 -2  —  最重要的一句話：右值引用變數，本身是左值
// =============================================================================
//
// 【主題資訊 Information】
//   核心規則 ： 「有名字的東西一律是左值」，即使它的宣告型別是 T&&
//   標準版本 ： C++11（右值引用）；decltype 亦為 C++11
//   標頭檔   ： <type_traits>（本檔用它把值類別「證明」出來）
//   證明工具 ： decltype(名字)   → 該變數的「宣告型別」
//               decltype((運算式)) → 該運算式的「值類別」
//                 左值 → T&、xvalue → T&&、prvalue → T
//   複雜度   ： 全部發生在編譯期，執行期成本為零
//
// 【詳細解釋 Explanation】
//
// 【1. 兩個層次：型別 vs 值類別】
//   這是本課最容易混淆、也最值得一次弄清楚的地方：
//       void f(int&& val) { ... }
//   * val 的「宣告型別」是 int&&（右值引用）—— 它決定 f 能接受什麼實參。
//   * 但在函式本體內，運算式 val 的「值類別」是左值 —— 因為它有名字。
//   兩者互不衝突，它們回答的是不同的問題：
//       型別   ：這個東西是什麼？
//       值類別 ：這個運算式可不可以被搬走？
//
// 【2. 為什麼標準要這樣規定】
//   因為「有名字」等於「你後面還可能再用到它」。
//       void f(std::string&& s) {
//           g(s);          // 如果這裡 s 是右值，g 就能把它搬空
//           h(s);          // 那這一行拿到的就是空字串 —— 災難
//       }
//   標準採取保守做法：有名字就是左值，誰都不能偷偷搬走它。
//   要放行必須自己寫 std::move(s)，而且照慣例那應該是最後一次使用它。
//
// 【3. 本檔如何「證明」而不是「宣稱」】
//   原始版本用 std::cout << &val 印出位址，論證是「可以取位址 → 是左值」。
//   方向正確，但位址每次執行都不同（ASLR），無法寫成可重現的預期輸出。
//   本檔改用 decltype，讓編譯器直接回答：
//       decltype(val)   → int&&  → std::is_rvalue_reference = true
//       decltype((val)) → int&   → std::is_lvalue_reference = true
//   同一個 val，多加一組括號就從「問宣告型別」變成「問值類別」，
//   兩個答案同時為 true —— 這正是「型別是右值引用、值類別是左值」的鐵證。
//
// 【4. 實際後果：能傳給誰、不能傳給誰】
//   int&& rref = 100;
//       takeLvalueRef(rref);   // ✅ rref 是左值 → 能綁 T&
//       takeConstRef(rref);    // ✅ 左值 → 能綁 const T&
//       takeRvalueRef(rref);   // ❌ 左值 → 綁不了 T&&
//       takeRvalueRef(std::move(rref));  // ✅ 明確轉回右值
//   本檔第 39 行的錯誤示範必須維持註解狀態。
//
// 【概念補充 Concept Deep Dive】
//
// (A) decltype 的兩種模式，為什麼一組括號差這麼多
//   標準規定：decltype(e) 中，若 e 是「未加括號的識別字或成員存取」，
//   得到的是它的宣告型別；其他情況（包含加了括號）則依 e 的值類別給出
//   T&（左值）、T&&（xvalue）或 T（prvalue）。
//   這不是編譯器的怪癖，而是刻意提供兩種查詢方式。
//   順帶一提，這也是 decltype((x)) 這種寫法在
//   「回傳型別要不要帶引用」的場合會被用到的原因。
//
// (B) 這條規則就是 std::forward 存在的理由
//   在樣板中，參數的值類別資訊會在「有了名字」之後遺失。
//       template<class T> void wrapper(T&& arg) { inner(arg); }  // 永遠傳左值！
//   要把呼叫端原本的值類別「原封不動」傳下去，必須用
//       inner(std::forward<T>(arg));
//   forward 的作用是「有條件的 move」：實參本來是右值才轉回右值。
//   完美轉發之所以需要一個專門的函式，根源就在本檔這條規則。
//
// (C) 為什麼「可以取位址」確實蘊含「是左值」
//   原版的論證本身沒有錯：內建的 & 運算子要求運算元是左值，
//   所以能取位址就代表是左值。只是把「位址的值」印出來當證據並不必要
//   —— 真正的證據是「這行程式碼編得過」，而不是它印出什麼數字。
//
// 【注意事項 Pay Attention】
//   1. 函式參數宣告成 T&&，在函式本體內它是左值。要往下傳遞「可搬走」的
//      許可，必須再寫一次 std::move（樣板中則用 std::forward）。
//   2. std::move(x) 之後應該把 x 當成已經交出去了 —— 除了重新賦值或
//      讓它解構，不要再讀它的內容。
//   3. 本檔第 11 行與第 39 行是刻意保留的錯誤示範，取消註解會編譯失敗。
//   4. 不要用列印位址的方式在教材裡示範值類別：位址每次執行都不同。
//      decltype + type traits 才是可重現且精確的做法。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】右值引用變數的值類別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void f(int&& v) 裡面，v 是左值還是右值？
//     答：是左值 —— 因為它有名字。它的「宣告型別」是右值引用，
//         但「作為運算式」它是左值。本機實測：
//         is_rvalue_reference<decltype(v)> = true，
//         is_lvalue_reference<decltype((v))> = true，兩者同時成立。
//         所以 f 內部若要把 v 交給另一個吃 T&& 的函式，必須寫 std::move(v)。
//     追問：這對寫移動建構函數有什麼實際影響？
//         → MyClass(MyClass&& o) : m_member(std::move(o.m_member)) { }
//           少了那個 std::move，o.m_member 是左值，
//           成員會被「複製」而不是移動 —— 移動建構函數變成慢版拷貝，
//           而且不會有任何警告。
//
// 🔥 Q2. decltype(x) 和 decltype((x)) 為什麼可能不同？
//     答：decltype 對「未加括號的識別字」回答宣告型別；對其他運算式
//         則依值類別回答：左值 → T&、xvalue → T&&、prvalue → T。
//         所以對 int&& r 來說，decltype(r) 是 int&&，decltype((r)) 是 int&。
//     追問：什麼時候會踩到這個差別？
//         → decltype(auto) 作為回傳型別時。return x; 回傳 T，
//           return (x); 回傳 T& —— 若 x 是區域變數，後者就是懸空引用。
//
// ⚠️ 陷阱. 「參數型別寫成 T&&，函式內就會自動走移動」——不會。
//     答：函式內 v 是左值，把它傳給任何重載都會選中 const T& 那個版本，
//         也就是「複製」。整條路徑看起來充滿右值引用，實際上一次都沒移動。
//         必須明確寫 std::move(v) 才會走移動。
//     為什麼會錯：以為宣告型別會「傳染」給後續的每一次使用。
//         實際上值類別是「逐個運算式」判定的，
//         而「具名變數」這個運算式永遠是左值。
// ═══════════════════════════════════════════════════════════════════════════

// lesson31_rref_is_lvalue.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson31b lesson31_rref_is_lvalue.cpp

#include <iostream>
#include <type_traits>   // 用 type traits 把「值類別」證明出來，而不是印位址
#include <utility>       // std::move

void takeRvalueRef(int&& val) {
    std::cout << "  takeRvalueRef: val = " << val << "\n";
    // 不印位址（每次執行都不同、無法寫成可重現的預期輸出），
    // 改用 decltype 直接證明「宣告型別是右值引用，但運算式是左值」：
    std::cout << "  decltype(val)   是右值引用? " << std::boolalpha
              << std::is_rvalue_reference<decltype(val)>::value
              << "  ← 宣告型別是 int&&\n";
    std::cout << "  decltype((val)) 是左值引用? "
              << std::is_lvalue_reference<decltype((val))>::value
              << "  ← 但運算式 val 是左值\n";

    // val 是左值，所以不能傳給另一個接受右值引用的函數：
    // takeRvalueRef(val);  // ❌ 錯誤！val 是左值
}

void takeLvalueRef(int& val) {
    std::cout << "  takeLvalueRef: val = " << val << "\n";
}

void takeConstRef(const int& val) {
    std::cout << "  takeConstRef: val = " << val << "\n";
}

int main() {
    int x = 10;

    std::cout << "=== 傳入右值 ===\n";
    takeRvalueRef(42);           // ✅ 42 是右值
    takeRvalueRef(x + 1);       // ✅ x + 1 是右值

    std::cout << "\n=== 右值引用變數是左值 ===\n";
    int&& rref = 100;
    std::cout << "  rref = " << rref << "\n";
    std::cout << "  decltype(rref)   是右值引用? " << std::boolalpha
              << std::is_rvalue_reference<decltype(rref)>::value << "\n";
    std::cout << "  decltype((rref)) 是左值引用? "
              << std::is_lvalue_reference<decltype((rref))>::value
              << "  ← 有名字就是左值\n";

    // rref 是左值，所以可以傳給接受左值引用的函數：
    takeLvalueRef(rref);    // ✅ rref 是左值
    takeConstRef(rref);     // ✅ rref 是左值

    // 但不能再傳給接受右值引用的函數：
    // takeRvalueRef(rref); // ❌ rref 是左值！

    // 要傳給 T&&，必須自己明確地把它轉回右值：
    std::cout << "\n=== 用 std::move 轉回右值 ===\n";
    takeRvalueRef(std::move(rref));   // ✅ std::move 只是 cast，不搬任何東西

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 31 課：右值引用（Rvalue Reference）入門2.cpp" -o lesson31b

// 【本檔對原始版本做的修改】
//   原版用 std::cout << &val 印出位址，藉「可以取址」證明 val 是左值。
//   論證本身正確（內建 & 運算子要求運算元是左值），但位址受 ASLR 影響、
//   每次執行都不同，無法寫成可重現的預期輸出。
//   本檔改用 decltype + type traits 直接讓編譯器回答：
//       decltype(val)   → int&&（is_rvalue_reference = true）
//       decltype((val)) → int& （is_lvalue_reference = true）
//   同一個 val，多一組括號就從「宣告型別」變成「值類別」，
//   兩者同時為 true —— 這比印位址更精確，而且完全可重現。
//   另外補上「用 std::move 轉回右值」的正面示範，讓錯誤示範有對照組。
//
// 【為何本檔沒有 LeetCode 範例】
//   本檔主題是值類別的判定規則，屬於語言核心語意，不是演算法。
//   LeetCode 沒有對應題型；本課用得上移動語意的題目（1656. Design an
//   Ordered Stream）已放在同課的 summary.cpp，此處不重複。

// === 預期輸出 ===
// === 傳入右值 ===
//   takeRvalueRef: val = 42
//   decltype(val)   是右值引用? true  ← 宣告型別是 int&&
//   decltype((val)) 是左值引用? true  ← 但運算式 val 是左值
//   takeRvalueRef: val = 11
//   decltype(val)   是右值引用? true  ← 宣告型別是 int&&
//   decltype((val)) 是左值引用? true  ← 但運算式 val 是左值
//
// === 右值引用變數是左值 ===
//   rref = 100
//   decltype(rref)   是右值引用? true
//   decltype((rref)) 是左值引用? true  ← 有名字就是左值
//   takeLvalueRef: val = 100
//   takeConstRef: val = 100
//
// === 用 std::move 轉回右值 ===
//   takeRvalueRef: val = 100
//   decltype(val)   是右值引用? true  ← 宣告型別是 int&&
//   decltype((val)) 是左值引用? true  ← 但運算式 val 是左值
