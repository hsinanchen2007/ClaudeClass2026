// ============================================================
// std::minmax    (C++11 起)
// 分類 (Category): Minimum/maximum operations (最值)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/minmax
//   * https://cplusplus.com/reference/algorithm/minmax/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::minmax 解的問題:
//
//   「同時拿到 (min, max) 對。」
//
// 兩個版本:
//   * 兩值版: minmax(a, b) → pair<const T&, const T&>
//   * ilist 版: minmax({a, b, c, ...}) → pair<T, T>
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼比分別呼叫 min + max 好?                          │
// └────────────────────────────────────────────────────────────┘
//
//   * 兩值版只需 1 次比較 (vs 兩次 min + max 需 2 次)。
//   * ilist 版約 3N/2 次比較 (vs 兩次掃描需 2N - 2 次)。
//   * 程式語意更清晰 — 一行表達「同時要最小與最大」。
//
// 「3N/2 次比較」是怎麼算的?把元素兩兩配對,每對先互比 1 次得 (s, l),
// 然後 s 和當前 min 比、l 和當前 max 比 (各 1 次),共 3 次比較處理 2 元素。
// 對 N 元素總計 ~3N/2。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、生命期陷阱                                             │
// └────────────────────────────────────────────────────────────┘
//
// 兩值版回傳 `pair<const T&, const T&>` — 與 std::min/max 同樣有 dangling 風險。
//
//   const auto& p = std::minmax(a, b + 1);   // ❌ 危險：b + 1 是暫存值
//   auto       p = std::minmax(a, b + 1);   // ❌ 一樣危險！auto 只是複製那個
//                                           //    pair，pair 裡面【仍然是 reference】
//
// 正確作法（三選一）：
//   int t = b + 1;  auto p = std::minmax(a, t);   // ① 兩邊都先變成 lvalue
//   auto p = std::minmax({a, b + 1});             // ② initializer_list 版回傳 pair<T,T>
//   int lo = std::min(a, b + 1), hi = std::max(a, b + 1);  // ③ 乾脆別建 pair
//
// 本檔下方的範例一律採用上述安全寫法；ASan 的 stack-use-after-scope 就是在抓這個。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、minmax vs minmax_element                                │
// └────────────────────────────────────────────────────────────┘
//
//   * std::minmax(a, b) / std::minmax({...}) → 「值」的 pair
//   * std::minmax_element(first, last)        → 「迭代器」的 pair (給範圍用)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class T>
//   std::pair<const T&, const T&> minmax(const T& a, const T& b);
//
//   template <class T, class Compare>
//   std::pair<const T&, const T&> minmax(const T& a, const T& b, Compare comp);
//
//   template <class T>
//   std::pair<T, T> minmax(std::initializer_list<T> ilist);
//
//   template <class T, class Compare>
//   std::pair<T, T> minmax(std::initializer_list<T> ilist, Compare comp);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 兩值版有生命期陷阱 — 不要 const auto& 接。
//   2. 對「容器」用 std::minmax_element。
//   3. ilist 版若多個最小/最大,兩個位置可能對應原 ilist 中不同元素 (依實作)。
//
// ============================================================

/*
補充筆記：std::minmax
  - std::minmax 一次取得兩個值中的較小與較大，避免重複寫比較邏輯。
  - 回傳 pair，兩個參數版本可能包含 reference；暫時物件生命週期要小心。
  - 處理 initializer_list 時會回傳值，適合小型固定集合。
  - std::minmax 用來取得最小、最大或限制範圍；重點是分清楚回傳的是值、reference 還是 iterator。
  - min/max 處理兩個值，min_element/max_element 處理一段 range；range 版本在空範圍時會回傳 last。
  - clamp(value, lo, hi) 要求 lo 不大於 hi；上下界順序錯誤會違反前置條件。
  - minmax 可避免自己寫兩次比較，minmax_element 則能同時找位置；大量資料時比各跑一次 min_element/max_element 更直接。
  - 比較浮點數時要注意 NaN；一般比較器遇到 NaN 可能讓結果不符合直覺。
  - 若要保留原資料位置，使用 element 版本回傳 iterator；若只要數值，才使用 min/max 的值版本。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::minmax
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::minmax 的兩個版本各回傳什麼？
//     答：兩值版 `minmax(a, b)` 回傳 **`std::pair<const T&, const T&>`**——pair 裡裝的
//         是兩個 **reference**，沒有拷貝。initializer_list 版 `minmax({a, b, c})`
//         回傳 **`std::pair<T, T>`**——裝的是值，安全。
//         比較次數：兩值版 1 次；ilist 版至多 ⌊3N/2⌋ 次（pairwise，比分別跑
//         min + max 的 2(N-1) 次省）。
//     追問：和 minmax_element 差在哪？(minmax 給「值」的 pair，minmax_element
//           給「迭代器」的 pair、用於容器範圍)
//
// ⚠️ 陷阱. `auto p = std::minmax(a, b + 1);` 用 auto 接住就安全了嗎？
//     答：**不安全**。auto 確實把那個 pair 複製了一份，但 **pair 的成員本身就是
//         reference**——複製 pair 只是複製了兩個 reference，它們仍然指向已經銷毀的
//         `b + 1` 暫存物。用結構化繫結 `auto [lo, hi] = std::minmax(a, b+1);` 一樣救不了，
//         繫結的還是那兩個 dangling reference。
//         三種正解（本檔範例採用）：① 先把兩邊都存成具名 lvalue 再比；
//         ② 改用 ilist 版 `std::minmax({a, b + 1})`（回傳 pair<T,T>，是值）；
//         ③ 乾脆分開寫 `int lo = std::min(...), hi = std::max(...);`。
//     為什麼會錯：大家記得的規則是「min/max 只要別用 const auto&、改用 auto 就安全」，
//         那對 `std::min` 成立（auto 會從 const T& 複製出一個 T），但對 minmax 不成立
//         ——被複製的是「裝著 reference 的 pair」，auto 攔不住裡面那層 reference。
//         這也是本檔範例 1 明明只有兩個值，卻刻意寫成 `std::minmax({7, 3})` 的原因。
//
// 🔥 Q3. 什麼工具可以在執行期抓到這種 dangling？
//     答：AddressSanitizer 的 **stack-use-after-scope**（`-fsanitize=address`）；
//         編譯期則有 `-Wdangling-reference`（GCC 13 起）能對部分情形提出警告。
//         但兩者都不保證抓得到所有情況，正解仍是「不要讓 reference 活得比對象久」。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 兩值 ---
    // ⚠️ 這裡【必須】用 initializer_list 版：std::minmax(7, 3) 會回傳
    //    pair<const int&, const int&>，指向兩個在該行結束就消失的暫存值。
    auto p = std::minmax({7, 3});   // initializer_list 版 → pair<int, int>，是值不是參考
    std::cout << "minmax(7,3) = (" << p.first << ", " << p.second << ")\n";

    // --- 範例 2: initializer_list ---
    auto q = std::minmax({4, 1, 7, 0, 3});
    std::cout << "minmax{4,1,7,0,3} = (" << q.first << ", " << q.second << ")\n";

    // --- 範例 3: 自訂 comp ---
    auto r = std::minmax({-5, 3, -7, 2},
                         [](int a, int b){ return std::abs(a) < std::abs(b); });
    std::cout << "minmax by |.|: (" << r.first << ", " << r.second << ")\n";

    // --- 範例 4: 用結構化繫結 (C++17) 拆開 ---
    auto [mn, mx] = std::minmax({10, 20, 5, 30});
    std::cout << "structured: min=" << mn << ", max=" << mx << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_539_min_time_difference_concept();
    void practical_order_range();
    void leetcode_1913_max_product_diff_pair();
    void practical_age_range_summary();
    leetcode_539_min_time_difference_concept();
    practical_order_range();
    leetcode_1913_max_product_diff_pair();
    practical_age_range_summary();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 539 概念:最小時間差 (Minimum Time Difference)
// ----------------------------------------------------------------
// 題目:給時間清單 (HH:MM),求兩兩之間最小的「分鐘差」(考慮環狀)。
//
// 為什麼用 std::minmax:
//   排序後,對相鄰兩個分鐘數用 std::minmax 一次取得 (較小, 較大),
//   差 = 大 - 小。一次得兩個值,程式碼乾淨。
//
// 複雜度:時間 O(n log n) (排序);空間 O(n)。
void leetcode_539_min_time_difference_concept() {
    std::vector<int> mins{23 * 60 + 59, 0, 60};
    std::sort(mins.begin(), mins.end());
    int ans = 24 * 60;
    for (size_t i = 1; i < mins.size(); ++i) {
        auto p = std::minmax(mins[i - 1], mins[i]);
        ans = std::min(ans, p.second - p.first);
    }
    // ⚠️ mins.front() + 24 * 60 是暫存值，不能餵給回傳 reference 的兩參數版；
    //    先存成具名變數（lvalue）再比。
    const int wrapped = mins.front() + 24 * 60;
    const int last    = mins.back();
    auto p2 = std::minmax(wrapped, last);
    ans = std::min(ans, p2.second - p2.first);
    std::cout << "LC539: min time diff = " << ans << " min\n";
}

// ----------------------------------------------------------------
// 實務範例:訂單金額同時取 (min, max) 顯示範圍
// ----------------------------------------------------------------
// 場景:處理含「最低付款」與「最高授權」的訂單,要顯示 "範圍 NT$<min> ~ NT$<max>"。
//
// 為什麼用 std::minmax:
//   一次比較取得 pair<min, max>,效率優於分別呼叫 min/max。
void practical_order_range() {
    int low_payment  = 1500;
    int high_auth    = 800;
    auto [mn, mx] = std::minmax(low_payment, high_auth);
    std::cout << "order range: NT$" << mn << " ~ NT$" << mx << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1913: 兩個數對之間的最大乘積差 (Maximum Product Difference)
// ----------------------------------------------------------------
// 題目:給陣列 nums (len >= 4),找 (a, b, c, d) 四個不同索引使得
//      (nums[a] * nums[b]) - (nums[c] * nums[d]) 最大。
//
// 為什麼用 std::minmax:
//   要最大化差值 = 最大兩數乘積 - 最小兩數乘積。
//   排序或一次掃描即可,但用 minmax 可以很乾淨地一次拿 (mn1, mx1) 起手。
//
// 複雜度:時間 O(n log n) (排序) 或 O(n) (掃描);空間 O(1)。
void leetcode_1913_max_product_diff_pair() {
    std::vector<int> nums{5, 6, 2, 7, 4};
    std::sort(nums.begin(), nums.end());
    int n = nums.size();
    auto [mn1, mn2] = std::minmax(nums[0], nums[1]);   // 兩個最小
    auto [mx2, mx1] = std::minmax(nums[n-2], nums[n-1]); // 兩個最大
    int diff = (mx1 * mx2) - (mn1 * mn2);
    std::cout << "LC1913: " << diff << '\n';
}

// ----------------------------------------------------------------
// 實務範例:統計報表「年齡區間」(用戶資料 min/max age 一次取)
// ----------------------------------------------------------------
// 場景:用戶註冊資料中,要報告「最年輕」與「最年老」用戶年齡 (顯示在儀表板)。
//      minmax 一次取兩值,比分別呼叫 min/max 少一次掃描的比較成本。
void practical_age_range_summary() {
    auto [mn, mx] = std::minmax({28, 19, 45, 32, 60, 22});
    std::cout << "age range: " << mn << " ~ " << mx << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra minmax.cpp -o minmax

// === 預期輸出 ===
// minmax(7,3) = (3, 7)
// minmax{4,1,7,0,3} = (0, 7)
// minmax by |.|: (2, -7)
// structured: min=5, max=30
// LC539: min time diff = 1 min
// order range: NT$800 ~ NT$1500
// LC1913: 34
// age range: 19 ~ 60
