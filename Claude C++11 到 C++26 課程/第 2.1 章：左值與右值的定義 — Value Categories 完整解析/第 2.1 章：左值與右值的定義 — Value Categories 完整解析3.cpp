// =============================================================================
//  第 2.1 章 -3  —  xvalue（將亡值）：有身份、且可被移動的表達式
// =============================================================================
//
// 【主題資訊 Information】
//   簽名：template<class T> constexpr remove_reference_t<T>&& move(T&& t) noexcept;
//   標準版本：std::move 為 C++11（C++14 起為 constexpr）
//   複雜度：O(1) — 它只是一個 static_cast，執行期什麼事都不做
//   標頭檔：<utility>
//   判定工具：decltype((expr)) 推導為 T&& → 該表達式是 xvalue
//
// 【詳細解釋 Explanation】
//
// 【1. xvalue 是為了填補 C++98 缺的那一格而發明的】
//   C++98 只有 lvalue / rvalue。rvalue 幾乎等於「臨時物件」，於是產生一個
//   死結：一個有名字的物件（lvalue），即使你明知道它馬上就不用了，也沒有
//   任何辦法告訴編譯器「可以偷它的資源」。
//   C++11 的解法是把「有身份」與「可移動」拆成兩個獨立問題，於是多出
//   「有身份 + 可移動」這一格 = xvalue（eXpiring value，將亡值）。
//   std::move 的唯一職責，就是把一個 lvalue 表達式重新標記成 xvalue。
//
// 【2. std::move 沒有移動任何東西 —— 它只是一個 cast】
//   實作大致等同：
//     template<class T>
//     remove_reference_t<T>&& move(T&& t) noexcept {
//         return static_cast<remove_reference_t<T>&&>(t);
//     }
//   它不配置、不複製、不清空、不呼叫任何建構子。真正的搬移發生在「接收端」：
//     std::string s2 = std::move(s);
//     ↑ std::move(s) 產生 xvalue → 於是多載解析選中 string(string&&)
//       → 是那個移動建構子把 s 的緩衝區指標偷走的
//   如果接收端沒有移動建構子（或該型別是 const），多載解析會退回拷貝，
//   std::move 就完全沒有效果 —— 它本來就沒有強制力，只是「表態」。
//   更精確的說法：std::move 是「無條件轉成 xvalue」，命名上其實叫
//   rvalue_cast 會更貼切，這是標準委員會公認的歷史遺憾。
//
// 【概念補充 Concept Deep Dive】
//   ★ 為什麼「具名的右值引用」自己是 lvalue（本章最重要的推論）
//     void f(std::string&& x) {
//         // x 的『型別』是 string&&
//         // 但 x 這個『表達式』是 lvalue —— 它有名字、可以 &x
//         g(x);              // 呼叫 g 的『左值』多載！
//         g(std::move(x));   // 這樣才會呼叫右值多載
//     }
//   設計理由：函式內可能有多行程式碼，x 可能被用很多次。如果 x 一進來就
//   被當成 rvalue，那麼第一次傳給別人時資源就被偷走，後面幾行全部踩空。
//   所以語言規定：只要你能用名字指到它，它就是 lvalue（安全預設），
//   要放棄它必須再寫一次 std::move —— 這是明示的、可稽核的。
//   ★ 一句話總結：型別是 T&& ≠ 值類別是 rvalue。這兩件事正交。
//
//   ★ xvalue 還有哪些來源（不只 std::move）
//       std::move(x)                  最常見
//       static_cast<T&&>(x)           std::move 的底層
//       arr[0]（arr 是 T&& 陣列）、a.m（a 是 xvalue 時的成員存取）
//       回傳型別為 T&& 的函式呼叫，例如 std::move 本身
//
// 【注意事項 Pay Attention】
//   1. ★ 被移動後的物件處於「valid but unspecified state（有效但未指定狀態）」。
//      標準只保證：它仍然是個合法物件，可以安全解構、可以重新賦值、可以呼叫
//      「沒有前置條件」的操作（如 clear()、operator=、empty()）。
//      標準【不保證】它變成空字串，也【不保證】它保留原值。
//      因此：印出被移動後的值不是 UB，但結果由實作決定，不可依賴。
//      本機（libstdc++）實測結果是空字串，但這是實作細節，換平台可能不同，
//      正式程式碼絕不可依賴此行為。
//   2. 不要 std::move 一個 const 物件：const string&& 無法綁到 string&&，
//      會退回 const string&，也就是拷貝。加了 move 卻沒有加速，反而誤導讀者。
//   3. 不要對即將回傳的區域變數寫 return std::move(local);
//      那會把 lvalue 變成 xvalue，反而讓編譯器無法做 NRVO，通常更慢。
//   4. 對基本型別（int、double、指標）std::move 沒有任何效果 —— 沒有資源
//      可偷，移動就是複製，來源的值不變。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】xvalue 與 std::move
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void f(int&& x) { ... } 在 f 內部，x 是 lvalue 還是 rvalue？
//     答：x 是 lvalue。它的「型別」是 int&&（右值引用），但它的「值類別」是
//         lvalue，因為它有名字、可以取 &x。
//         所以要把它繼續當右值傳下去，必須再寫一次 std::move(x)；
//         這正是完美轉發（perfect forwarding）存在的原因。
//     追問：那為什麼語言要這樣設計，不讓 x 直接是 rvalue？
//         → 因為 f 內部可能用到 x 很多次。若 x 天生是 rvalue，第一次傳給
//           別的函式時資源就被偷走，後續使用全部踩空。預設為 lvalue 是
//           安全的，要放棄所有權必須明示。
//
// 🔥 Q2. std::move 到底做了什麼？
//     答：它什麼都沒搬。它是一個編譯期的 static_cast，把表達式的值類別從
//         lvalue 改成 xvalue，執行期不產生任何指令。真正的搬移是接收端的
//         移動建構子／移動賦值運算子做的。
//     追問：如果接收端沒有移動建構子會怎樣？
//         → 多載解析退回拷貝建構子，程式照樣正確編譯與執行，只是沒有變快。
//           std::move 從來不保證會發生移動。
//
// ⚠️ 陷阱. 被 std::move 之後的 std::string，它的值是什麼？
//     答：處於「有效但未指定」狀態。標準只保證它還是合法物件（可解構、
//         可重新賦值），不保證它是空字串。讀取它不是 UB，但結果由實作決定。
//     為什麼會錯：多數人在 libstdc++/libc++ 上看到它變成空字串，就記成
//         「move 後一定變空」。那是實作細節，不是標準保證；對自訂型別更是
//         完全看你的移動建構子怎麼寫。正式程式碼一律「移動後只賦值、不讀值」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>  // std::move

// 用來觀察「值類別」實際選中哪一個多載
void observe(const std::string&) { std::cout << "    → 選中 const T& （左值多載）\n"; }
void observe(std::string&&)      { std::cout << "    → 選中 T&&      （右值多載）\n"; }

// 示範：具名的右值引用參數，在函式內部是 lvalue
void namedRvalueRef(std::string&& x) {
    std::cout << "  參數 x 的型別是 string&&，但 x 有名字：\n";
    std::cout << "  直接傳 x        ："; observe(x);
    std::cout << "  傳 std::move(x) ："; observe(std::move(x));
}

int main() {
    std::string s = "Hello, World!";

    // std::move(s) 是 xvalue
    // 它仍然指向 s 這個物件（有身份），
    // 但告訴編譯器「可以移動」
    std::cout << "=== std::move 產生 xvalue，接收端才真的搬 ===\n";
    std::string s2 = std::move(s);  // s 的資源被移動到 s2

    std::cout << "s2 = \"" << s2 << "\"\n";
    std::cout << "s  = \"" << s  << "\"   ← 有效但未指定（valid but unspecified）\n";
    std::cout << "  標準只保證 s 仍是合法物件，不保證它變成空字串。\n";
    std::cout << "  本機（libstdc++）觀察到的是空字串，但這是實作細節，不可依賴。\n";
    std::cout << "  s.size() = " << s.size() << "（同上，實作細節）\n";

    // 唯一該對「被移動後物件」做的事：重新賦值
    s = "Reassigned";
    std::cout << "  重新賦值後 s = \"" << s << "\"  ← 這是標準保證安全的操作\n";

    // ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== 核心陷阱：具名的右值引用，自己是 lvalue ===\n";
    namedRvalueRef(std::string("Temporary"));
    std::cout << "  結論：型別是 T&& ≠ 值類別是 rvalue，兩者正交\n";

    // ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== std::move 對基本型別沒有效果 ===\n";
    int a = 42;
    int b = std::move(a);
    std::cout << "  int a = 42; int b = std::move(a);\n";
    std::cout << "  a = " << a << "（int 沒有資源可偷，移動等同複製，a 不變）\n";
    std::cout << "  b = " << b << "\n";

    // ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== std::move 一個 const 物件會退回拷貝 ===\n";
    const std::string c = "Immutable";
    std::string d = std::move(c);   // const string&& 綁不到 string&& → 走拷貝
    std::cout << "  const string c = \"Immutable\"; string d = std::move(c);\n";
    std::cout << "  c 仍然是 \"" << c << "\" → 發生的是拷貝，不是移動\n";
    std::cout << "  原因：const string&& 無法綁到 string&&，只能綁 const string&\n";
    std::cout << "  d = \"" << d << "\"\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.1 章：左值與右值的定義 — Value Categories 完整解析3.cpp" -o xvalue_demo

// === 預期輸出 ===
// === std::move 產生 xvalue，接收端才真的搬 ===
// s2 = "Hello, World!"
// s  = ""   ← 有效但未指定（valid but unspecified）
//   標準只保證 s 仍是合法物件，不保證它變成空字串。
//   本機（libstdc++）觀察到的是空字串，但這是實作細節，不可依賴。
//   s.size() = 0（同上，實作細節）
//   重新賦值後 s = "Reassigned"  ← 這是標準保證安全的操作
//
// === 核心陷阱：具名的右值引用，自己是 lvalue ===
//   參數 x 的型別是 string&&，但 x 有名字：
//   直接傳 x        ：    → 選中 const T& （左值多載）
//   傳 std::move(x) ：    → 選中 T&&      （右值多載）
//   結論：型別是 T&& ≠ 值類別是 rvalue，兩者正交
//
// === std::move 對基本型別沒有效果 ===
//   int a = 42; int b = std::move(a);
//   a = 42（int 沒有資源可偷，移動等同複製，a 不變）
//   b = 42
//
// === std::move 一個 const 物件會退回拷貝 ===
//   const string c = "Immutable"; string d = std::move(c);
//   c 仍然是 "Immutable" → 發生的是拷貝，不是移動
//   原因：const string&& 無法綁到 string&&，只能綁 const string&
//   d = "Immutable"
