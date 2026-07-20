// =============================================================================
//  第二課：泛型編程（Generic Programming）概念10.cpp
//   —  Concepts（C++20）：把隱含介面寫成程式碼的一部分
// =============================================================================
//
// ⚠️⚠️ 本檔需要 **C++20**，用 -std=c++17 編譯會直接失敗 ⚠️⚠️
//      本機 g++ 15.2 以 -std=c++17 編譯的實際錯誤：
//          error: 'concept' does not name a type; did you mean 'const'?
//          note: 'concept' only available with '-std=c++20' or '-fconcepts'
//      正確編譯指令見檔尾（-std=c++20）。詳見【注意事項 1】。
//
// 【主題資訊 Information】
//   語法：
//     template <typename T>
//     concept Comparable = requires(T a, T b) {       // 定義一個 concept
//         { a > b } -> std::convertible_to<bool>;
//         { a < b } -> std::convertible_to<bool>;
//     };
//
//     template <Comparable T> T find_max(T a, T b);   // 用 concept 約束模板
//
//   標準版本：**C++20**（concepts、requires-expression、<concepts> 標頭）
//             本機以 -pedantic-errors 實測：-std=c++17 失敗、-std=c++20 通過。
//   標頭檔  ：<iostream>、<concepts>
//
//   四種等價的約束寫法（都需要 C++20）：
//     template <Comparable T> T f(T, T);                        // 本檔用這個
//     template <typename T> requires Comparable<T> T f(T, T);   // requires 子句
//     template <typename T> T f(T, T) requires Comparable<T>;   // 尾置 requires
//     Comparable auto f(Comparable auto a, Comparable auto b);  // 簡寫形式
//
// 【詳細解釋 Explanation】
//
// 【1. concepts 解決的問題：讓要求從「隱含」變成「明確」】
// 概念3.cpp 說過，`template <typename T> T find_max(T a, T b)` 對 T 有個
// 「必須支援 operator>」的隱含要求，但這個要求**寫不進簽名裡**。
// 後果是：使用者看簽名不知道要提供什麼，違反時錯誤訊息又在模板本體裡爆炸。
//
// concepts 讓這條要求成為簽名的一部分：
//     template <Comparable T> T find_max(T a, T b);
// 現在讀簽名就知道「T 必須是 Comparable」，且編譯器會**在多載決議階段**
// 就檢查它，而不是等到實例化模板本體之後。
//
// 【2. requires-expression 在做什麼】
//     concept Comparable = requires(T a, T b) {
//         { a > b } -> std::convertible_to<bool>;
//         { a < b } -> std::convertible_to<bool>;
//     };
//
// 這段的意思是：「假設有兩個 T 型別的物件 a、b，下列運算式必須全部合法，
// 且其結果型別必須可轉換成 bool。」
//
// 三個關鍵性質：
//   * requires 區塊裡的運算式**從不被執行**，只被檢查「是否合法」。
//     它是純編譯期的型別查詢，不會產生任何程式碼。
//   * `{ 運算式 } -> 型別約束;` 這個形式除了檢查運算式合法，還檢查其型別。
//     只寫 `a > b;`（不加大括號與箭頭）則只檢查合法性、不管回傳型別。
//   * concept 本身是個編譯期的布林值：可以寫 `if constexpr (Comparable<int>)`，
//     也可以用 &&、|| 組合成更複雜的約束。
//
// 【3. concepts 不只是「更好的錯誤訊息」——它參與多載決議】
// 這是最容易被低估的一點。static_assert 的做法是「已經選中這個函式，
// 然後才發現型別不對，於是報錯」；concepts 則是「不滿足約束的函式
// **根本不進入候選集**」。
//
// 差別在有多個多載時是決定性的：
//     template <std::integral T>       void process(T v);  // 整數版
//     template <std::floating_point T> void process(T v);  // 浮點版
// 呼叫 process(42) 時，浮點版因約束不滿足而自動退出候選，整數版被選中。
// 若改用 static_assert 實作，兩個版本都會進入候選集 → 二義性錯誤。
//
// 更進一步，當多個 concept 都滿足時，編譯器會依「**約束包容關係**
// （subsumption）」選擇**更特化**的那個 —— 這使得「泛型版 + 特化版」的
// 分派可以完全靠約束表達，不再需要 tag dispatch 或 enable_if 那套技巧。
// 本檔用實際輸出示範這個機制。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 錯誤訊息真的變短了嗎？—— 誠實的實測結果
//   常見說法是「concepts 讓錯誤訊息變短」。本機 g++ 15.2 實測顯示，
//   這句話**要看模板深度**：
//
//     * 淺層（單層模板，對沒有 operator> 的 Point 呼叫 find_max）：
//         無約束版本  →  7 行
//         concept 版本 → 23 行   ← 反而比較長！
//
//     * 深層（對沒有 operator< 的 Point 排序）：
//         std::sort（無約束）        → 131 行
//         std::ranges::sort（有約束）→  36 行   ← 大幅改善
//
//   為什麼淺層反而變長？因為 concept 版會額外列出「候選函式、
//   約束為何不滿足、是哪一條 requirement 失敗」。以本檔的 Comparable 為例，
//   它會明確指出：
//       note: the required expression '(a > b)' is invalid
//       note: the required expression '(a < b)' is invalid
//
//   所以 concepts 真正的價值不是「行數變少」，而是：
//     1) 訊息**直接說出缺了什麼契約**，而不是展示模板內部的失敗細節
//     2) 深層巢狀時避免整條實例化鏈被展開（131 → 36 行）
//     3) 錯誤歸因正確 —— 指向「你的型別不滿足 Comparable」，
//        而非「函式庫某一行有問題」
//
// (B) 標準庫已內建大量 concept（<concepts> 與 <iterator>）
//   不必凡事自己定義：
//     std::integral、std::floating_point、std::signed_integral
//     std::same_as、std::convertible_to、std::derived_from
//     std::equality_comparable、std::totally_ordered
//     std::invocable、std::predicate
//     std::input_iterator、std::random_access_iterator（在 <iterator>）
//   以本檔為例，其實直接用 `std::totally_ordered` 就涵蓋了六個比較運算子，
//   比自訂的 Comparable 更完整。自訂 concept 應保留給「標準庫沒有、
//   且屬於你領域特有」的要求。
//
// (C) concepts 與 SFINAE / enable_if 的關係
//   C++20 之前要做同樣的事得寫：
//       template <typename T,
//                 typename = std::enable_if_t<std::is_integral<T>::value>>
//       void process(T v);
//   這串東西的可讀性極差，而且錯誤訊息更難懂。concepts 是它的正統取代品：
//   語意相同（不滿足就退出候選集），但語法直觀且錯誤訊息可讀。
//   既有程式碼裡看到 enable_if，通常就是「還沒升級到 C++20」的痕跡。
//
// 【注意事項 Pay Attention】
// 1. **本檔需要 C++20，C++17 無法編譯。** 這不是本檔的疏漏，而是
//    concepts 本身就是 C++20 才引入的特性。本機 g++ 15.2 以 -std=c++17
//    編譯的錯誤是：
//        error: 'concept' does not name a type; did you mean 'const'?
//        note: 'concept' only available with '-std=c++20' or '-fconcepts'
//    （GCC 的 -fconcepts 是標準化前的實驗性擴充，不建議在新專案使用。）
//    本課其餘 13 個檔案都可用 -std=c++17 編譯，只有本檔例外。
// 2. requires 區塊內的運算式**永遠不會被執行**，只被檢查合法性。
//    在裡面寫有副作用的程式碼沒有任何意義。
// 3. 別把 requires-expression 與 requires-clause 搞混：
//        template <typename T> requires Comparable<T>    // requires 子句
//        concept C = requires(T a) { ... };              // requires 運算式
//    語法上兩者都叫 requires，出現位置與作用完全不同。
//    因此也常見 `requires requires { ... }` 這種看起來很怪但合法的寫法。
// 4. concept 只檢查「語法上是否合法」，**不檢查語意是否正確**。
//    一個型別可以定義出完全違反直覺的 operator>（例如回傳恆為 true）
//    而仍然滿足 Comparable。concepts 保證的是介面，不是行為契約。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++20 Concepts
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. concepts 是哪個標準加入的？它解決什麼問題？
//     答：**C++20**。它把模板對型別的「隱含要求」變成簽名的一部分，
//         讓要求可見、可檢查、可組合。在此之前只能靠 static_assert
//         （事後報錯）或 std::enable_if / SFINAE（語法極難讀）來近似。
//         本機以 -pedantic-errors 實測：concepts 在 -std=c++17 失敗、
//         -std=c++20 通過。
//     追問：加了 concept 之後錯誤訊息一定變短嗎？
//         → 不一定，要看深度。本機實測：單層模板反而從 7 行變成 23 行
//           （因為要列出候選與不滿足的 requirement）；但深層巢狀時
//           std::sort 的 131 行降到 ranges::sort 的 36 行。
//           真正的價值是「直接說出缺了哪條契約」與「正確歸因」，
//           而不是單純的行數。
//
// 🔥 Q2. concepts 和 static_assert 都能擋掉不合適的型別，差在哪？
//     答：差在**是否參與多載決議**。static_assert 是「已經選中這個函式，
//         然後才報錯」；concepts 是「不滿足約束就不進入候選集」。
//         所以有多個多載時，concepts 能讓編譯器自動選中正確的那個，
//         static_assert 則會造成二義性或直接卡死在錯誤的多載上。
//     追問：多個 concept 同時滿足時怎麼選？
//         → 依約束的**包容關係**（subsumption）選更特化的那個。
//           例如同時有 Integral 與 Integral&&Signed 的多載，
//           傳 int 會選中後者。
//
// ⚠️ 陷阱. 型別滿足了 Comparable concept，是不是就保證它的比較行為正確？
//     答：不保證。concept 只檢查「`a > b` 這個運算式合法且結果可轉成 bool」，
//         完全不檢查語意。一個把 operator> 寫成「永遠回傳 true」的型別
//         照樣滿足 Comparable，拿去排序會得到毫無意義的結果
//         （甚至因違反 strict weak ordering 而導致 UB）。
//     為什麼會錯：把 concepts 當成「型別契約的完整驗證」。它驗證的是
//         **語法介面**（syntactic requirements），不是**語意保證**
//         （semantic requirements）。標準庫文件裡對 concept 的語意要求
//         （例如 strict weak ordering）是寫給人看的約定，編譯器無從檢查。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <concepts>
#include <string>    // Comparable<std::string> 用得到；別依賴間接引入（同概念6.cpp）

// 定義一個 Concept：可比較的型別
template <typename T>
concept Comparable = requires(T a, T b) {
    { a > b } -> std::convertible_to<bool>;
    { a < b } -> std::convertible_to<bool>;
};

// 使用 Concept 約束模板
template <Comparable T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

// -----------------------------------------------------------------------------
// concepts 參與多載決議的示範：兩個同名函式靠約束區分，
// 不滿足約束的那個會自動退出候選集（static_assert 做不到這件事）
// -----------------------------------------------------------------------------
template <std::integral T>
const char* classify(T) { return "整數型別（std::integral）"; }

template <std::floating_point T>
const char* classify(T) { return "浮點型別（std::floating_point）"; }

// 約束包容（subsumption）示範：更特化的約束會勝出
template <std::integral T>
const char* precision(T) { return "一般整數"; }

template <typename T>
    requires std::integral<T> && std::signed_integral<T>
const char* precision(T) { return "有號整數（更特化的約束勝出）"; }

int main() {
    std::cout << "=== concept 約束下的 find_max ===" << std::endl;
    std::cout << "find_max(10, 20)     = " << find_max(10, 20) << std::endl;
    std::cout << "find_max(3.14, 2.72) = " << find_max(3.14, 2.72) << std::endl;
    std::cout << "find_max('a', 'z')   = " << find_max('a', 'z') << std::endl;

    std::cout << "\n=== concept 是編譯期的布林值 ===" << std::endl;
    std::cout << std::boolalpha;
    std::cout << "Comparable<int>         = " << Comparable<int> << std::endl;
    std::cout << "Comparable<double>      = " << Comparable<double> << std::endl;
    std::cout << "Comparable<std::string> = " << Comparable<std::string>
              << std::endl;

    std::cout << "\n=== concepts 參與多載決議（不滿足者自動退出候選）===" << std::endl;
    std::cout << "classify(42)    -> " << classify(42) << std::endl;
    std::cout << "classify(3.14)  -> " << classify(3.14) << std::endl;
    std::cout << "classify('x')   -> " << classify('x')
              << "   <- char 也是 integral" << std::endl;

    std::cout << "\n=== 約束包容：更特化的勝出 ===" << std::endl;
    std::cout << "precision(42)  -> " << precision(42) << std::endl;
    std::cout << "precision(42u) -> " << precision(42u)
              << "   <- unsigned 不滿足 signed_integral" << std::endl;

    std::cout << "\n=== 違反約束時的錯誤訊息 ===" << std::endl;
    // struct Point { int x, y; };
    // Point p1{1,2}, p2{3,4};
    // find_max(p1, p2);   // 編譯錯誤，但訊息直接說出缺了哪條契約：
    //   error: no matching function for call to 'find_max(Point&, Point&)'
    //   note: constraints not satisfied
    //   note: the required expression '(a > b)' is invalid
    //   note: the required expression '(a < b)' is invalid
    std::cout << "對沒有 operator> 的型別呼叫，編譯器會說：" << std::endl;
    std::cout << "  note: constraints not satisfied" << std::endl;
    std::cout << "  note: the required expression '(a > b)' is invalid"
              << std::endl;
    std::cout << "—— 直接指出缺了哪條契約，而不是展示模板內部的失敗細節。"
              << std::endl;

    std::cout << "\n=== 標準庫已內建大量 concept，不必凡事自訂 ===" << std::endl;
    std::cout << "std::totally_ordered<int> = " << std::totally_ordered<int>
              << "   <- 這個比自訂的 Comparable 更完整" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念10.cpp -o concept10
//       ⚠️ 必須用 -std=c++20；concepts 是 C++20 特性，-std=c++17 會編譯失敗

// === 預期輸出 ===
// === concept 約束下的 find_max ===
// find_max(10, 20)     = 20
// find_max(3.14, 2.72) = 3.14
// find_max('a', 'z')   = z
//
// === concept 是編譯期的布林值 ===
// Comparable<int>         = true
// Comparable<double>      = true
// Comparable<std::string> = true
//
// === concepts 參與多載決議（不滿足者自動退出候選）===
// classify(42)    -> 整數型別（std::integral）
// classify(3.14)  -> 浮點型別（std::floating_point）
// classify('x')   -> 整數型別（std::integral）   <- char 也是 integral
//
// === 約束包容：更特化的勝出 ===
// precision(42)  -> 有號整數（更特化的約束勝出）
// precision(42u) -> 一般整數   <- unsigned 不滿足 signed_integral
//
// === 違反約束時的錯誤訊息 ===
// 對沒有 operator> 的型別呼叫，編譯器會說：
//   note: constraints not satisfied
//   note: the required expression '(a > b)' is invalid
// —— 直接指出缺了哪條契約，而不是展示模板內部的失敗細節。
//
// === 標準庫已內建大量 concept，不必凡事自訂 ===
// std::totally_ordered<int> = true   <- 這個比自訂的 Comparable 更完整
