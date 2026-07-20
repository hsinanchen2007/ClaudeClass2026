// =============================================================================
// 主題: 移動建構子什麼時候「不會」被自動生成 —— 以及 trait 為何會騙你
// =============================================================================
//
// 【主題資訊 Information】
//   查詢工具：std::is_move_constructible<T>::value           ← 會騙人
//             std::is_nothrow_move_constructible<T>::value   ← 才看得出真相
//   標準版本：type_traits / 移動建構子 / = default    皆為 C++11
//   標頭檔  ：<type_traits>
//   複雜度  ：純編譯期查詢，執行期零成本
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 自動生成的規則（Rule of Five 的由來）】
//   編譯器只在「你什麼都沒插手」時，才會自動生成移動建構子與移動賦值。
//   只要你自訂了下列任何一項，移動操作就「不會」被自動生成：
//       * 解構子
//       * 複製建構子 / 複製賦值運算子
//       * 另一個移動操作
//   理由是：你會自訂這些，通常代表這個類別在手動管理某種資源；
//   編譯器不敢自作主張猜測「怎麼移動才是對的」，乾脆不生成。
//   本檔的三個 Case 正是這條規則的對照組：
//       AutoMove     什麼都沒寫           → 有移動建構子
//       NoAutoMove   只多寫了解構子        → 移動建構子被抑制
//       ExplicitMove 寫了解構子 + = default → 用 = default 要回來
//
// 【2. is_move_constructible 為什麼會回傳 true（即使沒有移動建構子）】
//   這是本檔最重要的一課。
//   is_move_constructible<T> 問的是「能不能用一個右值來建構 T」，
//   而不是「T 有沒有移動建構子」。
//   由於 const T& 也能綁定右值，只要有複製建構子，答案就是 true ——
//   只是實際上走的是「複製」而非移動。
//   所以 NoAutoMove 明明沒有移動建構子，這個 trait 仍然回報 true。
//
// 【3. 用 is_nothrow_move_constructible 才看得出真相】
//   真正的移動建構子按慣例會標 noexcept；而複製建構子（要配置記憶體）不會。
//   因此「是否 nothrow」就成了一個實用的判別訊號：
//       AutoMove     → true   （編譯器生成的移動建構子推導為 noexcept）
//       NoAutoMove   → false  （其實走的是可能拋例外的複製）
//       ExplicitMove → true   （= default 要回了移動建構子）
//   ⚠️ 嚴格說，這是「間接」判別 ——
//      它真正檢查的是 noexcept，只是實務上與「有沒有真移動」高度相關。
//
// 【4. 這件事的實際代價】
//   若你的類別落入 NoAutoMove 的情況：
//     * std::move(obj) 會安靜地退回複製，程式正確但完全沒有加速
//     * std::vector 擴容時也會用複製
//   而且不會有任何警告 —— 這是效能問題中最難察覺的一類。
//   解法就是 Rule of Five：一旦自訂了其中一個，就把五個都明確處理
//   （自己寫，或明寫 = default / = delete）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 AutoMove 的成員是 std::string 也能自動移動
//     編譯器生成的移動建構子會「逐成員移動」——
//     對每個成員呼叫它自己的移動建構子。
//     std::string 有 noexcept 的移動建構子，int 直接複製，
//     所以整個 AutoMove 的移動建構子被推導為 noexcept。
//
// (B) = default 為什麼能「要回來」被抑制的移動操作
//     = default 是明確告訴編譯器：「請用你原本會生成的那份實作」。
//     它繞過了「自訂解構子 → 抑制移動」這條規則，
//     同時保留了逐成員移動的正確語意。
//     這比自己手寫更安全 —— 手寫容易漏掉成員或忘記歸零。
//
// (C) 現代首選其實是 Rule of Zero
//     本檔三個 Case 都有一個共同前提：類別自己管理資源。
//     若改用 std::string / std::vector / std::unique_ptr 當成員，
//     就完全不需要自訂解構子，五個特殊成員函式一個都不用寫，
//     移動操作也會自動且正確地生成 —— 這就是 Rule of Zero。
//
// 【注意事項 Pay Attention】
//   1. 自訂解構子或任一複製操作，會抑制移動操作的自動生成。
//   2. is_move_constructible 為 true 不代表有真正的移動；
//      它只表示「能用右值建構」，而 const T& 也接得住右值。
//   3. 要判斷有無真移動，改看 is_nothrow_move_constructible（間接但實用）。
//   4. 移動被抑制時，std::move 會安靜退回複製，不會有任何警告。
//   5. 需要移動時請明寫 = default（Rule of Five）；更好的作法是 Rule of Zero。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動操作的自動生成規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 我只是加了一個空的解構子 ~Foo() {}，會有什麼影響？
//     答：移動建構子與移動賦值都不再自動生成。
//         之後 std::move(foo) 會安靜退回複製 —— 程式仍然正確，但完全沒加速，
//         而且 vector 擴容時也會用複製。這個代價完全沒有警告提示。
//     追問：怎麼救？→ 明寫 Foo(Foo&&) = default; 與
//         Foo& operator=(Foo&&) = default;（Rule of Five）。
//         更好的作法是根本不要自訂解構子（Rule of Zero）。
//
// 🔥 Q2. std::is_move_constructible<T>::value 是 true，代表 T 有移動建構子嗎？
//     答：不代表。這個 trait 問的是「能不能用右值建構 T」，
//         而 const T& 也能綁定右值，所以只要有複製建構子它就回 true。
//         要看有沒有真正的移動，應改用 is_nothrow_move_constructible。
//     追問：為什麼 nothrow 版本就能區分？→ 因為真正的移動建構子按慣例標 noexcept
//         （只搬指標不會拋），而複製建構子要配置記憶體、可能拋 bad_alloc。
//         這是間接但實務上很可靠的判別方式。
//
// ⚠️ 陷阱. 這三個類別都能通過 MyClass b = std::move(a); 編譯，是不是都有移動？
//     答：不是。三個都能編譯，但 NoAutoMove 走的是「複製」——
//         因為它的移動建構子被解構子抑制了，重載決議退回 const T&。
//         能編譯不代表發生了移動，兩者是不同的問題。
//     為什麼會錯：把「編譯通過」當成「優化生效」。
//         std::move 從來不保證會發生移動，它只是提出請求；
//         實際走哪條路由重載決議決定，而那取決於類別有沒有移動操作。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>

// Case 1：什麼都沒寫 → 自動生成移動建構子
class AutoMove {
    std::string name_;
    int value_;
};

// Case 2：寫了解構子 → 不會自動生成移動建構子
class NoAutoMove {
    std::string name_;
    int value_;
public:
    ~NoAutoMove() {}  // 有自訂解構子
};

// Case 3：明確要求生成
class ExplicitMove {
    std::string name_;
    int value_;
public:
    ~ExplicitMove() {}
    ExplicitMove(ExplicitMove&&) = default;  // 明確要求生成
    ExplicitMove(const ExplicitMove&) = default;
};

int main() {
    std::cout << std::boolalpha;

    std::cout << "AutoMove 可移動建構？ "
              << std::is_move_constructible<AutoMove>::value << "\n";

    std::cout << "NoAutoMove 可移動建構？ "
              << std::is_move_constructible<NoAutoMove>::value << "\n";
    // ↑ 注意：這仍然是 true！因為 const T& 可以接收右值
    //   但實際上走的是複製，不是真正的移動

    std::cout << "NoAutoMove 有 nothrow 移動建構？ "
              << std::is_nothrow_move_constructible<NoAutoMove>::value << "\n";
    // ↑ 這才能看出真相：沒有真正的移動建構子

    std::cout << "ExplicitMove 有 nothrow 移動建構？ "
              << std::is_nothrow_move_constructible<ExplicitMove>::value << "\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.3 章：移動建構子 (Move Constructor) — 實作與原理4.cpp" -o mc_demo4

// === 預期輸出 ===
// AutoMove 可移動建構？ true
// NoAutoMove 可移動建構？ true
// NoAutoMove 有 nothrow 移動建構？ false
// ExplicitMove 有 nothrow 移動建構？ true
