// ============================================================
// std::min
// 分類 (Category): Minimum/maximum operations (最值)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/min
//   * https://cplusplus.com/reference/algorithm/min/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::min 解的問題:
//
//   「給我兩個值 (或一個 initializer_list),回傳『較小』的那個。」
//
// 是 STL 中最基礎的工具之一,但有幾個需要小心的細節:
//
//   * 兩值版回傳「const 參考」 — 有「生命期陷阱」(見下)。
//   * ilist 版回傳「值」 (一定是新的 T)。
//   * 兩值相等 → 回傳「a」(第一個參數)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、生命期陷阱 — 千萬要避開!                               │
// └────────────────────────────────────────────────────────────┘
//
// 兩值版 std::min 回傳「對其中一個參數的 const 參考」 — 沒做拷貝。
// 如果參數是「暫存值」,回傳的參考會指向已經被銷毀的記憶體 (dangling)!
//
//   const auto& m = std::min(a, b + 1);   // ← b + 1 是暫存值!
//   // 離開這行的「全表達式」後,b + 1 的暫存物被銷毀 → m 變成 dangling 參考
//
// 安全寫法:
//
//   auto m = std::min(a, b + 1);          // 「by value」一定安全
//   int  m = std::min(a, b + 1);          // 直接寫型別也安全
//
// 這是 std::min/max/clamp 共通的陷阱 — 簡單規則:不要用 `const auto&` 接它們的回傳值。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、std::min vs std::min_element                            │
// └────────────────────────────────────────────────────────────┘
//
//   * std::min(a, b)              → 兩個值的較小者 (常數時間)
//   * std::min({a, b, c, ...})    → ilist 中的最小值 (O(N))
//   * std::min_element(first, last) → 「容器範圍」中的最小元素的「迭代器」
//
// 想知道「值」用 std::min;想知道「位置」(迭代器、索引) 用 std::min_element。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class T>
//   const T& min(const T& a, const T& b);
//
//   template <class T, class Compare>
//   const T& min(const T& a, const T& b, Compare comp);
//
//   template <class T>
//   T min(std::initializer_list<T> ilist);                       // C++11
//
//   template <class T, class Compare>
//   T min(std::initializer_list<T> ilist, Compare comp);
//
//   * C++14 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 兩值 O(1);ilist O(N - 1) 次比較
//   空間: O(1)
//
//   1. 生命期陷阱 — 不要用 const auto& 接。
//   2. 兩值相等回傳 a。
//   3. 容器最小用 min_element;同時要 min/max 用 minmax 或 minmax_element。
//
// ============================================================

/*
補充筆記：std::min
  - std::min 比較兩個值或 initializer_list，回傳較小者。
  - 兩個參數版本可能回傳 reference；不要保存由暫時物件形成的 reference。
  - 自訂比較器應表示嚴格小於關係，不要使用 <=。
  - std::min 用來取得最小、最大或限制範圍；重點是分清楚回傳的是值、reference 還是 iterator。
  - min/max 處理兩個值，min_element/max_element 處理一段 range；range 版本在空範圍時會回傳 last。
  - clamp(value, lo, hi) 要求 lo 不大於 hi；上下界順序錯誤會違反前置條件。
  - minmax 可避免自己寫兩次比較，minmax_element 則能同時找位置；大量資料時比各跑一次 min_element/max_element 更直接。
  - 比較浮點數時要注意 NaN；一般比較器遇到 NaN 可能讓結果不符合直覺。
  - 若要保留原資料位置，使用 element 版本回傳 iterator；若只要數值，才使用 min/max 的值版本。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::min
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::min 的回傳型別是什麼？和 std::min_element 差在哪？
//     答：兩值版 `min(const T& a, const T& b)` 回傳 **const T&**（不做拷貝）；
//         initializer_list 版 `min({a, b, c})` 回傳 **T**（值）。
//         而 `std::min_element(first, last)` 回傳的是**迭代器**。
//         一句話：要「值」用 min，要「位置」用 min_element。
//     追問：為什麼兩值版要回傳 reference？(避免對大型物件做不必要的複製，
//           代價就是下面那個 lifetime 陷阱)
//
// 🔥 Q2. `std::min(a, b)` 在 a 與 b 相等時回傳哪一個？
//     答：回傳 **a**（第一個參數）。因為實作是 `return b < a ? b : a;`——
//         用嚴格 `<`，相等時條件為 false，走到 a 這一支。
//     追問：這有什麼實務意義？(對「相等但可區分」的物件，例如同分但不同人的
//           結構體，決定了你拿到哪一筆；也是自訂 comp 不可寫成 `<=` 的原因之一)
//
// ⚠️ 陷阱. `const auto& m = std::min(a, b + 1);` 有什麼問題？
//     答：`b + 1` 是暫存值，全表達式結束後就被銷毀；min 回傳的是對它的 const
//         reference，於是 m 變成 dangling reference，之後讀 m 是 UB。
//         安全寫法：`auto m = std::min(a, b + 1);`（by value 一定安全）。
//     為什麼會錯：大家都學過「const reference 可以延長暫存物的生命期」，但那條規則
//         只在「直接綁定到暫存物」時成立；這裡綁的是函式回傳的 reference，
//         編譯器無從得知它指向誰，生命期延長不會發生。min/max/clamp 皆同。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 兩值 ---
    std::cout << "min(3, 5) = " << std::min(3, 5) << '\n';

    // --- 範例 2: 字串 (字典順序) ---
    std::cout << "min(\"banana\", \"apple\") = "
              << std::min(std::string("banana"), std::string("apple")) << '\n';

    // --- 範例 3: 自訂 comp — 比絕對值 ---
    std::cout << "min |.|: "
              << std::min(-7, 3, [](int a, int b){ return std::abs(a) < std::abs(b); })
              << '\n';

    // --- 範例 4: initializer_list (一次比多值) ---
    std::cout << "min{4, 1, 7, 0, 3} = "
              << std::min({4, 1, 7, 0, 3}) << '\n';

    // --- 範例 5: 用 by-value 接,避免生命期陷阱 ---
    int a = 5;
    auto m = std::min(a, 10);
    std::cout << "captured by value: " << m << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1295_find_numbers_even_digits();
    void practical_timeout_min();
    void leetcode_746_min_cost_climbing_stairs();
    void practical_remaining_quota();
    leetcode_1295_find_numbers_even_digits();
    practical_timeout_min();
    leetcode_746_min_cost_climbing_stairs();
    practical_remaining_quota();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1295: 統計位數為偶數的數字 (Find Numbers with Even Digits)
// ----------------------------------------------------------------
// 題目:給整數陣列,計算其中「位數為偶數」的元素個數。
//
// 為什麼用 std::min:
//   除了主要計數,順便用 std::min 維護「最小位數」 — 示範
//   std::min 在「滾動更新極值」中的常見用法。
//
// 複雜度:時間 O(n × log10(num));空間 O(1)。
void leetcode_1295_find_numbers_even_digits() {
    std::vector<int> nums{12, 345, 2, 6, 7896};
    int count = 0;
    int min_digits = 1000;
    for (int x : nums) {
        int d = 0, t = x;
        do { ++d; t /= 10; } while (t != 0);
        min_digits = std::min(min_digits, d);
        if (d % 2 == 0) ++count;
    }
    std::cout << "LC1295: even-digit count = " << count
              << ", min digits = " << min_digits << '\n';
}

// ----------------------------------------------------------------
// 實務範例:取兩個 timeout 的較小值
// ----------------------------------------------------------------
// 場景:HTTP 請求同時受「客戶端 timeout」與「伺服器 timeout」限制 —
//      實際生效的是兩者中較小的那一個 (先到的先觸發)。
//
// 為什麼用 std::min:
//   一行表達「取兩個限制中更嚴的」,程式語意一目了然。
void practical_timeout_min() {
    int client_timeout_ms = 5000;
    int server_timeout_ms = 3000;
    int effective = std::min(client_timeout_ms, server_timeout_ms);
    std::cout << "timeout = min(client=" << client_timeout_ms
              << ", server=" << server_timeout_ms << ") = "
              << effective << " ms\n";
}

// ----------------------------------------------------------------
// LeetCode 746: 使用最小花費爬樓梯 (Min Cost Climbing Stairs)
// ----------------------------------------------------------------
// 題目:給陣列 cost,可從 index 0 或 1 開始,每步走 1 或 2 階。
//      抵達樓頂 (i = n) 的最小花費。
//
// 為什麼用 std::min:
//   經典 DP — dp[i] = cost[i] + min(dp[i-1], dp[i-2]);最終答案 = min(dp[n-1], dp[n-2])。
//
// 複雜度:時間 O(n);空間 O(1) (滾動兩個變數)。
void leetcode_746_min_cost_climbing_stairs() {
    std::vector<int> cost{10, 15, 20};
    int prev2 = cost[0], prev1 = cost[1];
    for (size_t i = 2; i < cost.size(); ++i) {
        int cur = cost[i] + std::min(prev1, prev2);
        prev2 = prev1;
        prev1 = cur;
    }
    int ans = std::min(prev1, prev2);
    std::cout << "LC746: " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例:剩餘配額 = min(時間配額, 流量配額)
// ----------------------------------------------------------------
// 場景:雲端服務同時計算 API 呼叫次數與資料傳輸量配額,
//      使用者「真正能用的」是兩者中較小的剩餘。
void practical_remaining_quota() {
    int api_calls_left = 30;
    int bandwidth_left = 18;
    int effective = std::min(api_calls_left, bandwidth_left);
    std::cout << "quota left = " << effective
              << " (api=" << api_calls_left
              << ", bw=" << bandwidth_left << ")\n";
}

// 編譯: g++ -std=c++20 -Wall -Wextra min.cpp -o min

// === 預期輸出 ===
// min(3, 5) = 3
// min("banana", "apple") = apple
// min |.|: 3
// min{4, 1, 7, 0, 3} = 0
// captured by value: 5
// LC1295: even-digit count = 2, min digits = 1
// timeout = min(client=5000, server=3000) = 3000 ms
// LC746: 15
// quota left = 18 (api=30, bw=18)
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
