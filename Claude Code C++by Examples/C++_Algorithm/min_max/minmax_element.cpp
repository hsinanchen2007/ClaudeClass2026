// ============================================================
// std::minmax_element    (C++11 起)
// 分類 (Category): Minimum/maximum operations (最值)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/minmax_element
//   * https://cplusplus.com/reference/algorithm/minmax_element/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::minmax_element 解的問題:
//
//   「在範圍中,同時找出最小元素與最大元素的『迭代器』。」
//
// 一次掃描得到 (min_iter, max_iter) — 比分別呼叫 min_element + max_element 更省。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼比兩次掃描省一半?                                │
// └────────────────────────────────────────────────────────────┘
//
//   * min_element 需 N - 1 次比較
//   * max_element 需 N - 1 次比較
//   * 兩者相加 ≈ 2N 次比較
//
//   * minmax_element 用「pairwise」演算法,約 3(N-1)/2 次比較
//
// 「pairwise」概念:
//   把元素兩兩配對 (a, b),先互比 1 次得 (smaller, larger);
//   smaller 跟當前 min 比、larger 跟當前 max 比 (各 1 次)。
//   每對 2 元素只需 3 次比較,共 ~3N/2。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、與 max_element 的細微差別 — 重複最大值                  │
// └────────────────────────────────────────────────────────────┘
//
// 重要細節!
//
//   * std::max_element     對「多個最大」回傳 **第一個**
//   * std::minmax_element  對「多個最大」回傳 **最後一個**
//
// 為什麼?因為 minmax_element 的 [min_iter, max_iter] 區段
// 通常用來「涵蓋所有重複情況」,把 max 放到最後一個位置可以
// 讓區段最大,涵蓋所有相等元素。
//
// 這個差異很細,被某些題目刁難時要記得:max 想要「第一個」就用 max_element;
// minmax_element 取的是「最後一個」max。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt>
//   std::pair<FwdIt, FwdIt> minmax_element(FwdIt first, FwdIt last);
//
//   template <class FwdIt, class Compare>
//   std::pair<FwdIt, FwdIt> minmax_element(FwdIt first, FwdIt last, Compare comp);
//
//   * C++14 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 約 3(N-1)/2 次比較 — O(N)
//   空間: O(1)
//   邊界:空範圍 → pair{last, last}。
//
//   1. max 取「最後一個」(與 max_element 不同)。
//   2. 比 min_element + max_element 省一半比較。
//   3. 結構化繫結 auto [lo, hi] = ...; 是現代寫法。
//
// ============================================================

/*
補充筆記：std::minmax_element
  - minmax_element 一次掃描同時找最小與最大位置，通常比兩次獨立掃描更合適。
  - 空範圍會回傳 {last,last}，兩個 iterator 都不能解參考。
  - 若比較器只看部分欄位，最小與最大都是以該欄位定義。
  - std::minmax_element 用來取得最小、最大或限制範圍；重點是分清楚回傳的是值、reference 還是 iterator。
  - min/max 處理兩個值，min_element/max_element 處理一段 range；range 版本在空範圍時會回傳 last。
  - clamp(value, lo, hi) 要求 lo 不大於 hi；上下界順序錯誤會違反前置條件。
  - minmax 可避免自己寫兩次比較，minmax_element 則能同時找位置；大量資料時比各跑一次 min_element/max_element 更直接。
  - 比較浮點數時要注意 NaN；一般比較器遇到 NaN 可能讓結果不符合直覺。
  - 若要保留原資料位置，使用 element 版本回傳 iterator；若只要數值，才使用 min/max 的值版本。
*/
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6, 9};

    // --- 範例 1: 一次取得 min 與 max ---
    auto [lo, hi] = std::minmax_element(v.begin(), v.end());
    std::cout << "min=" << *lo << " at " << (lo - v.begin())
              << ", max=" << *hi << " at " << (hi - v.begin()) << '\n';

    // --- 範例 2: 多個 max,minmax_element 取「最後一個」(細節!) ---
    auto me = std::max_element(v.begin(), v.end());
    std::cout << "max_element first 9 at index " << (me - v.begin()) << '\n';
    auto mm = std::minmax_element(v.begin(), v.end());
    std::cout << "minmax_element max at index "
              << (mm.second - v.begin()) << " (last 9)\n";

    // --- 範例 3: 對結構體找成績區間 ---
    struct Score { std::string n; int s; };
    std::vector<Score> ss{{"a",80},{"b",60},{"c",90},{"d",70}};
    auto [low, high] = std::minmax_element(ss.begin(), ss.end(),
        [](const Score& x, const Score& y){ return x.s < y.s; });
    std::cout << "lowest=" << low->n << "(" << low->s << "), "
              << "highest=" << high->n << "(" << high->s << ")\n";

    // === LeetCode / 實務範例 ===
    void leetcode_414_range_via_minmax();
    void leetcode_561_array_partition_concept();
    void practical_score_report();
    void leetcode_1085_sum_digits_in_minimum();
    void practical_chart_axis_auto_range();
    leetcode_414_range_via_minmax();
    leetcode_561_array_partition_concept();
    practical_score_report();
    leetcode_1085_sum_digits_in_minimum();
    practical_chart_axis_auto_range();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 414 概念:用 minmax_element 取陣列範圍 (max - min)
// ----------------------------------------------------------------
// 題目:LC 414 是「找第三大」,但這裡示範前置概念 — 一次取得 (min, max)
//      作為「陣列總範圍」,在某些變化題中可幫忙剪枝。
//
// 為什麼用 std::minmax_element:
//   只需一次掃描就拿到範圍邊界,比兩次 min_element + max_element 省一半。
void leetcode_414_range_via_minmax() {
    std::vector<int> nums{3, 2, 1};
    auto [lo, hi] = std::minmax_element(nums.begin(), nums.end());
    std::cout << "LC414: max=" << *hi << ", min=" << *lo
              << ", range=" << (*hi - *lo) << '\n';
}

// ----------------------------------------------------------------
// LeetCode 561 概念:陣列拆分 I — 觀察範圍變化
// ----------------------------------------------------------------
// 題目:把 2n 個整數兩兩配對 (a, b),最大化 sum(min(a, b))。
//      解法是排序後取偶數位元素加總。
//
// 為什麼順便用 std::minmax_element:
//   觀察「原陣列範圍」(max - min) 是這類問題的常見前置步驟,
//   minmax_element 一行給你兩端。
void leetcode_561_array_partition_concept() {
    std::vector<int> nums{1, 4, 3, 2};
    auto [lo, hi] = std::minmax_element(nums.begin(), nums.end());
    std::cout << "LC561: original range = [" << *lo << ", " << *hi << "]"
              << ", span = " << (*hi - *lo) << '\n';
    std::sort(nums.begin(), nums.end());
    int sum = 0;
    for (size_t i = 0; i < nums.size(); i += 2) sum += nums[i];
    std::cout << "LC561: max sum of min pairs = " << sum << '\n';
}

// ----------------------------------------------------------------
// 實務範例:成績報表一次取得最高與最低
// ----------------------------------------------------------------
// 場景:月度成績報表要同時顯示「最高分」與「最低分」學生 (姓名 + 分數)。
//
// 為什麼用 std::minmax_element:
//   * 一次走訪資料,~3N/2 次比較 (vs 兩次掃描需 2(N-1) 次)。
//   * 結構化繫結讓程式碼短小漂亮。
void practical_score_report() {
    struct Student { std::string name; int score; };
    std::vector<Student> cls{
        {"Alice", 88}, {"Bob", 72}, {"Cathy", 95}, {"David", 55}, {"Eve", 80}
    };
    auto [lo, hi] = std::minmax_element(cls.begin(), cls.end(),
        [](const Student& a, const Student& b){ return a.score < b.score; });
    std::cout << "report: lowest=" << lo->name << "(" << lo->score << "), "
              << "highest=" << hi->name << "(" << hi->score << ")\n";
}

// ----------------------------------------------------------------
// LeetCode 1085: 最小元素的各位數字之和 (Sum of Digits in the Minimum)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums,計算「最小元素」各位數字之和,若是偶數回傳 1,否則 0。
//
// 為什麼用 std::minmax_element:
//   只需要最小元素就好,但這裡示範 minmax_element 一次拿 (min, max),
//   附帶輸出 max 作為 debug。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_1085_sum_digits_in_minimum() {
    std::vector<int> nums{34, 23, 1, 24, 75, 33, 54, 8};
    auto [lo, hi] = std::minmax_element(nums.begin(), nums.end());
    int m = *lo, sum = 0;
    while (m) { sum += m % 10; m /= 10; }
    int ans = (sum % 2 == 0) ? 1 : 0;
    std::cout << "LC1085: min=" << *lo << " (max=" << *hi
              << "), digit_sum=" << sum << " ans=" << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例:圖表 Y 軸自動範圍 (auto-range axis)
// ----------------------------------------------------------------
// 場景:繪製折線圖時,Y 軸範圍要根據資料自動設成 [min - margin, max + margin]。
//      用 minmax_element 一次取兩端,程式碼簡潔。
void practical_chart_axis_auto_range() {
    std::vector<double> series{23.5, 19.2, 28.7, 21.0, 30.5, 17.8};
    auto [lo, hi] = std::minmax_element(series.begin(), series.end());
    double margin = (*hi - *lo) * 0.1;
    std::cout << "Y axis: [" << (*lo - margin) << ", " << (*hi + margin) << "]\n";
}

// === 預期輸出 (Expected output) ===
// min=1 at 1, max=9 at 8
// max_element first 9 at index 5
// minmax_element max at index 8 (last 9)
// lowest=b(60), highest=c(90)
// LC414: max=3, min=1, range=2
// LC561: original range = [1, 4], span = 3
// LC561: max sum of min pairs = 4
// report: lowest=David(55), highest=Cathy(95)
// LC1085: min=1 (max=75), digit_sum=1 ans=0
// Y axis: [16.53, 31.77]
