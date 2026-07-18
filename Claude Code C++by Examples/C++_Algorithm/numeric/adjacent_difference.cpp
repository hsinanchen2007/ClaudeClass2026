// ============================================================
// std::adjacent_difference
// 分類 (Category): Numeric operations (數值演算法)
// 標頭檔 (Header):  <numeric>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/adjacent_difference
//   * https://cplusplus.com/reference/numeric/adjacent_difference/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::adjacent_difference 解的問題:
//
//   「對範圍計算『相鄰差』:每個位置 = 該位置 - 前一個位置。」
//
// 公式 (預設用 -):
//
//   out[0] = in[0]                  ★ 第 0 個是「直接拷貝」,不是 0!
//   out[i] = in[i] - in[i-1]        (i >= 1)
//
// 範例:in = {2, 4, 6, 8, 11} → out = {2, 2, 2, 2, 3}
//
// 「相鄰差」是訊號處理、時序分析的基礎工具 — 把「累計值」轉成「增量」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、與 partial_sum 互逆                                     │
// └────────────────────────────────────────────────────────────┘
//
//   adjacent_difference 與 partial_sum 是「離散版的微分與積分」 — 互為逆運算:
//
//   partial_sum(adjacent_difference(v)) == v
//   adjacent_difference(partial_sum(v)) == v
//
// 應用情境:
//   * 「累計確診數」 → 用 adjacent_difference → 「每日新增」
//   * 「每日銷售」  → 用 partial_sum         → 「累計銷售」
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼第 0 個元素是「直接拷貝」而不是 0?                │
// └────────────────────────────────────────────────────────────┘
//
// 為了保證「partial_sum(adjacent_difference(v)) == v」這個互逆性。
// 第 0 個拷貝後,後面 partial_sum 才能完整還原 v。
//
// 這個設計常常嚇到第一次用的人 — 別以為它是 bug,是 by design。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、自訂運算的方向 — 注意 op(右, 左)                        │
// └────────────────────────────────────────────────────────────┘
//
// 自訂 BinaryOp op 時,呼叫方式是「op(右元素, 左元素)」:
//
//   out[i] = op(in[i], in[i-1])
//
// 例如用 std::divides<>{} 得到「相鄰商 (ratio)」:
//   out[i] = in[i] / in[i-1]
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class OutputIt>
//   OutputIt adjacent_difference(InputIt first, InputIt last, OutputIt d_first);
//
//   template <class InputIt, class OutputIt, class BinaryOp>
//   OutputIt adjacent_difference(InputIt first, InputIt last, OutputIt d_first,
//                                BinaryOp op);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N - 1 次運算
//   空間: O(1) (除輸出)
//
//   1. 第 0 個輸出是「拷貝」不是 0!
//   2. 自訂 op 是 op(右, 左) — 順序很重要。
//   3. 與 partial_sum 互逆 (微分/積分)。
//   4. 適合差分編碼、序列變動偵測。
//
// ============================================================

/*
補充筆記：std::adjacent_difference
  - adjacent_difference 輸出第一個元素原值，後面輸出相鄰差。
  - 它和 partial_sum 是互補概念，常用於差分陣列。
  - 自訂二元運算可把「差」換成其他相鄰關係。
  - std::adjacent_difference 屬於數值演算法，常用來把迴圈中的累加、差分、內積或前綴和寫成標準形式。
  - accumulate 的初始值型別會決定累加結果型別；用 0 累加 double 或 long long 時可能不小心截斷或溢位。
  - reduce 可重新排序運算順序以利平行化，因此 binary operation 必須接近結合律且沒有副作用；浮點數結果可能和 accumulate 不完全相同。
  - partial_sum 和 adjacent_difference 互為常見搭配，可用來建立前綴和與差分陣列。
  - gcd/lcm 對整數工作；lcm 可能溢位，即使最後型別是 long long 也要注意輸入大小。
  - 數值演算法最容易出錯的是型別和溢位，不是語法；工作程式應刻意選擇初始值型別。
*/
#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 經典差分 ---
    std::vector<int> v{2, 4, 6, 8, 11};
    std::vector<int> diff(v.size());
    std::adjacent_difference(v.begin(), v.end(), diff.begin());
    std::cout << "adjacent_difference: ";
    for (int x : diff) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 用除法得到「相鄰比值」 ---
    std::vector<double> w{1.0, 2.0, 6.0, 24.0};
    std::vector<double> ratio(w.size());
    std::adjacent_difference(w.begin(), w.end(), ratio.begin(),
                             std::divides<double>{});
    std::cout << "ratios: ";
    for (double x : ratio) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 與 partial_sum 互逆 (round-trip) ---
    std::vector<int> orig{3, 1, 4, 1, 5};
    std::vector<int> diffs(orig.size());
    std::adjacent_difference(orig.begin(), orig.end(), diffs.begin());
    std::vector<int> back(orig.size());
    std::partial_sum(diffs.begin(), diffs.end(), back.begin());
    std::cout << "round-trip: ";
    for (int x : back) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_121_best_time_to_buy_and_sell_stock();
    void leetcode_1991_find_middle_index_diff();
    void practical_daily_new_cases();
    void leetcode_1685_sum_absolute_differences_concept();
    void practical_pace_running();
    leetcode_121_best_time_to_buy_and_sell_stock();
    leetcode_1991_find_middle_index_diff();
    practical_daily_new_cases();
    leetcode_1685_sum_absolute_differences_concept();
    practical_pace_running();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 121: 買賣股票的最佳時機 (Best Time to Buy and Sell Stock)
// ----------------------------------------------------------------
// 題目:給 prices[i] 為第 i 天股價,只能買賣一次,求最大利潤。
//
// 為什麼用 std::adjacent_difference:
//   把「股價」轉成「每日漲跌」 — 等價於「找最大連續子陣列和」(Kadane)。
//   adjacent_difference 一行完成「序列 → 變動」,讓問題形式變得清楚。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_121_best_time_to_buy_and_sell_stock() {
    std::vector<int> prices{7, 1, 5, 3, 6, 4};
    std::vector<int> diff(prices.size());
    std::adjacent_difference(prices.begin(), prices.end(), diff.begin());
    int best = 0, cur = 0;
    for (std::size_t i = 1; i < diff.size(); ++i) {
        cur = std::max(0, cur + diff[i]);
        best = std::max(best, cur);
    }
    std::cout << "LC121 max profit = " << best << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1991: 找出陣列的中間位置 (Find the Middle Index in Array)
// ----------------------------------------------------------------
// 題目:找出 i 使得 sum(0..i-1) == sum(i+1..n-1)。
//
// 為什麼順便示範 std::adjacent_difference:
//   主要解法是 prefix sum,但 adjacent_difference(prefix) == 原 nums 是
//   一個有趣的不變式 — 可作為 sanity check 確保 prefix 計算正確。
void leetcode_1991_find_middle_index_diff() {
    std::vector<int> nums{2, 3, -1, 8, 4};
    std::vector<int> prefix(nums.size() + 1, 0);
    std::partial_sum(nums.begin(), nums.end(), prefix.begin() + 1);
    int total = prefix.back(), idx = -1;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        int left  = prefix[i];
        int right = total - prefix[i + 1];
        if (left == right) { idx = static_cast<int>(i); break; }
    }
    std::vector<int> back(prefix.size());
    std::adjacent_difference(prefix.begin(), prefix.end(), back.begin());
    std::cout << "LC1991 middle index = " << idx
              << " (verified diff[1..]=";
    for (std::size_t i = 1; i < back.size(); ++i)
        std::cout << back[i] << (i + 1 == back.size() ? "" : ",");
    std::cout << ")\n";
}

// ----------------------------------------------------------------
// 實務範例:從累積病例還原為每日新增
// ----------------------------------------------------------------
// 場景:衛生單位給的是「累積確診數 cumulative」,要繪製「每日新增 daily」折線圖。
//
// 為什麼用 std::adjacent_difference:
//   它就是 partial_sum 的逆運算 — 由累積值還原每日增量。
//   第 0 個輸出 = cumulative[0] = 第一天的新增 — 邏輯正好對齊。
void practical_daily_new_cases() {
    std::vector<int> cumulative{5, 12, 18, 25, 40, 60};
    std::vector<int> daily(cumulative.size());
    std::adjacent_difference(cumulative.begin(), cumulative.end(), daily.begin());
    std::cout << "daily new cases: ";
    for (int x : daily) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1685 概念:陣列相鄰差作為基礎統計
// 難度: medium
// ----------------------------------------------------------------
// 題目簡化:給已排序陣列 nums,觀察「相鄰差」總和 — 是個「Range Sum 變體」。
//      (LC 1685 原題是「絕對差總和」,需要 O(n) 前綴和;這裡只示範 adjacent_difference
//       作為前處理,得到 diff 序列。)
//
// 為什麼用 std::adjacent_difference:
//   許多統計題的第一步:「序列 → 差分序列」,觀察波動或變化。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_1685_sum_absolute_differences_concept() {
    std::vector<int> nums{2, 3, 5};
    std::vector<int> diff(nums.size());
    std::adjacent_difference(nums.begin(), nums.end(), diff.begin());
    int sum_diff_nonfirst = 0;
    for (size_t i = 1; i < diff.size(); ++i) sum_diff_nonfirst += diff[i];
    std::cout << "LC1685: sum diffs (excluding first) = " << sum_diff_nonfirst << '\n';
}

// ----------------------------------------------------------------
// 實務範例:跑步軌跡 — 由「累積距離」算出「每公里配速」
// ----------------------------------------------------------------
// 場景:GPS 紀錄每分鐘累積跑距 (公尺),想算「每分鐘新增距離」(配速)。
//      正是 adjacent_difference 的用途。
void practical_pace_running() {
    std::vector<int> cum_meters{200, 380, 555, 720, 900};
    std::vector<int> pace(cum_meters.size());
    std::adjacent_difference(cum_meters.begin(), cum_meters.end(), pace.begin());
    std::cout << "per-minute distance (m):";
    for (int x : pace) std::cout << ' ' << x;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// adjacent_difference: 2 2 2 2 3
// ratios: 1 2 3 4
// round-trip: 3 1 4 1 5
// LC121 max profit = 5
// LC1991 middle index = 3 (verified diff[1..]=2,3,-1,8,4)
// daily new cases: 5 7 6 7 15 20
// LC1685: sum diffs (excluding first) = 3
// per-minute distance (m): 200 180 175 165 180
