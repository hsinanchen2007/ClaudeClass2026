// ============================================================
// std::accumulate
// 分類 (Category): Numeric operations (數值演算法)
// 標頭檔 (Header):  <numeric>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/accumulate
//   * https://cplusplus.com/reference/numeric/accumulate/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::accumulate 解的問題:
//
//   「對範圍中的元素『累積結合』,得到單一結果。」
//
// 預設用 + 作為結合運算 → 結果是「總和」。
// 也可傳自訂二元運算 (例如 *、min、max、字串串接) 做更廣義的歸約 (reduction)。
//
// 計算方式 (左結合、依序):
//
//   result = init
//   for each x in [first, last):
//       result = result + x         (或 op(result, x))
//   return result
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、accumulate vs reduce (C++17)                            │
// └────────────────────────────────────────────────────────────┘
//
//   * accumulate (C++98) — 順序固定 (sequential),保證左結合
//   * reduce (C++17)     — 不保證順序 → 可平行
//
// 結合律與交換律不同的運算,結果可能不同:
//
//   accumulate:嚴格依序 → 字串串接 OK ("a" + "b" + "c" = "abc")
//   reduce:不保順序 → 字串串接結果未定 → 不要用!
//
// 浮點加法理論上不結合 (因精度),accumulate 與 reduce 可能微小差異。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、init 型別決定結果型別 — 經典陷阱!                      │
// └────────────────────────────────────────────────────────────┘
//
// 這是 accumulate 最常被踩的雷:
//
//   std::vector<double> v{0.5, 1.5};
//   std::accumulate(v.begin(), v.end(), 0)     // ← 0 是 int!
//   // 結果型別 = int → 0.5 與 1.5 在運算中被截成 int → 結果是 1 (錯!)
//
//   std::accumulate(v.begin(), v.end(), 0.0)   // 0.0 是 double → 結果 2.0 (對)
//
// **永遠把 init 寫成「跟元素同型別」**,例如:
//   * 整數加總: init = 0 (或 0LL 配合 long long)
//   * 浮點加總: init = 0.0
//   * 字串串接: init = std::string{}
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、自訂運算的常見用法                                     │
// └────────────────────────────────────────────────────────────┘
//
//   * 求積:    init = 1, op = std::multiplies<>
//   * 求最大:  init = INT_MIN, op = max
//   * 求最小:  init = INT_MAX, op = min
//   * 字串串接: init = std::string{}, op = + (預設)
//   * 平方和:  init = 0, op = [](sum, x){ return sum + x*x; }
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class T>
//   T accumulate(InputIt first, InputIt last, T init);
//
//   template <class InputIt, class T, class BinaryOp>
//   T accumulate(InputIt first, InputIt last, T init, BinaryOp op);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次運算
//   空間: O(1)
//
//   1. init 型別陷阱 — 對齊元素型別。
//   2. 順序固定 — 字串串接 OK,但無法平行化 (用 reduce 才能)。
//   3. 大資料且結合律 OK → 改用 std::reduce 取得平行優勢。
//   4. 「先映射再累積」可一行用 std::transform_reduce (C++17)。
//
// ============================================================

/*
補充筆記：std::accumulate
  - accumulate 的初始值型別決定累加型別；0 代表 int，0LL 才是 long long。
  - 它按照輸入順序累加，適合需要穩定順序或有非交換運算的情境。
  - 若要平行或允許重排運算，再考慮 reduce，但運算必須能安全重排。
  - std::accumulate 屬於數值演算法，常用來把迴圈中的累加、差分、內積或前綴和寫成標準形式。
  - accumulate 的初始值型別會決定累加結果型別；用 0 累加 double 或 long long 時可能不小心截斷或溢位。
  - reduce 可重新排序運算順序以利平行化，因此 binary operation 必須接近結合律且沒有副作用；浮點數結果可能和 accumulate 不完全相同。
  - partial_sum 和 adjacent_difference 互為常見搭配，可用來建立前綴和與差分陣列。
  - gcd/lcm 對整數工作；lcm 可能溢位，即使最後型別是 long long 也要注意輸入大小。
  - 數值演算法最容易出錯的是型別和溢位，不是語法；工作程式應刻意選擇初始值型別。
*/
#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 4, 5};

    // --- 範例 1: 預設求和 ---
    int sum = std::accumulate(v.begin(), v.end(), 0);
    std::cout << "sum = " << sum << '\n';

    // --- 範例 2: 求積 (注意 init = 1) ---
    int prod = std::accumulate(v.begin(), v.end(), 1,
                               std::multiplies<int>{});
    std::cout << "product = " << prod << '\n';

    // --- 範例 3: 字串串接 (init = std::string{}) ---
    std::vector<std::string> ss{"Hello", ", ", "C++", "!"};
    std::string joined = std::accumulate(ss.begin(), ss.end(), std::string{});
    std::cout << "joined: " << joined << '\n';

    // --- 範例 4: init 型別陷阱示範 ---
    std::vector<double> w{0.5, 1.5};
    int    bad  = std::accumulate(w.begin(), w.end(), 0);     // ← int 截斷!
    double good = std::accumulate(w.begin(), w.end(), 0.0);   // ← 正確
    std::cout << "bad (int init) = " << bad
              << ", good (double init) = " << good << '\n';

    // --- 範例 5: 自訂運算 — 平方和 ---
    int ssq = std::accumulate(v.begin(), v.end(), 0,
                              [](int s, int x){ return s + x * x; });
    std::cout << "sum of squares = " << ssq << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1672_richest_customer_wealth();
    void leetcode_1431_kids_with_greatest_number_of_candies();
    void practical_order_total();
    void leetcode_1295_avg_of_array();
    void practical_class_average_score();
    leetcode_1672_richest_customer_wealth();
    leetcode_1431_kids_with_greatest_number_of_candies();
    practical_order_total();
    leetcode_1295_avg_of_array();
    practical_class_average_score();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1672: 最富有客戶的資產總量 (Richest Customer Wealth)
// ----------------------------------------------------------------
// 題目:給 m × n 的 accounts,回傳最富有客戶的總資產 (每列加總後取 max)。
//
// 為什麼用 std::accumulate:
//   對「每一列」做加總正是 accumulate 的典型用法 — 一行解決。
//
// 複雜度:時間 O(m × n);空間 O(1)。
void leetcode_1672_richest_customer_wealth() {
    std::vector<std::vector<int>> accounts{{1,2,3},{3,2,1}};
    int max_wealth = 0;
    for (const auto& row : accounts) {
        int wealth = std::accumulate(row.begin(), row.end(), 0);
        max_wealth = std::max(max_wealth, wealth);
    }
    std::cout << "LC1672: " << max_wealth << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1431: 擁有最多糖果的小孩 (用 accumulate 找最大值的變化版)
// ----------------------------------------------------------------
// 題目:給 candies 與 extraCandies,對每個 i 判斷 candies[i] + extra >= max。
//
// 為什麼用 std::accumulate (找最大值):
//   accumulate 配自訂 op 可以做「找最大值」 — 雖然 max_element 更直接,
//   這裡示範 accumulate 作為「通用歸約」的彈性用法。
void leetcode_1431_kids_with_greatest_number_of_candies() {
    std::vector<int> candies{2, 3, 5, 1, 3};
    int extra = 3;
    int mx = std::accumulate(candies.begin(), candies.end(), 0,
                             [](int a, int b){ return std::max(a, b); });
    std::cout << "LC1431: ";
    for (int c : candies)
        std::cout << ((c + extra >= mx) ? "true " : "false ");
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:訂單總金額 (Order Total)
// ----------------------------------------------------------------
// 場景:購物車有多筆商品,每筆已預先算好「行小計 (line_total)」,求總金額。
//
// 為什麼用 std::accumulate:
//   純粹的加總,語意最直白,程式碼一行。
void practical_order_total() {
    std::vector<double> line_totals{99.5, 250.0, 30.0, 1280.75};
    double total = std::accumulate(line_totals.begin(), line_totals.end(), 0.0);
    std::cout << "Order total = " << total << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1480 概念:陣列平均值 (用 accumulate 算總和再除以 n)
// ----------------------------------------------------------------
// 題目:回傳陣列平均值 (浮點)。
//
// 為什麼用 std::accumulate:
//   平均 = 總和 / 元素個數;總和用 accumulate 一行寫完。
//   注意初值用 0.0 (double) 才不會被截斷。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_1295_avg_of_array() {
    std::vector<int> nums{4, 3, 2, 1, 5};
    double sum = std::accumulate(nums.begin(), nums.end(), 0.0);
    double avg = sum / nums.size();
    std::cout << "LC-avg: " << avg << '\n';
}

// ----------------------------------------------------------------
// 實務範例:班級平均成績計算
// ----------------------------------------------------------------
// 場景:學期末計算全班平均分。accumulate 配 lambda 也能一次過濾零分學生
//      (例如缺考者) 後計算平均。
void practical_class_average_score() {
    std::vector<int> scores{80, 92, 0, 75, 100, 60};   // 0 視為缺考
    int total = std::accumulate(scores.begin(), scores.end(), 0,
                                [](int s, int x){ return s + (x > 0 ? x : 0); });
    int taken = std::accumulate(scores.begin(), scores.end(), 0,
                                [](int s, int x){ return s + (x > 0 ? 1 : 0); });
    std::cout << "class avg (exclude absent): "
              << (taken > 0 ? double(total) / taken : 0.0) << '\n';
}

// === 預期輸出 (Expected output) ===
// sum = 15
// product = 120
// joined: Hello, C++!
// bad (int init) = 1, good (double init) = 2
// sum of squares = 55
// LC1672: 6
// LC1431: true true true false true
// Order total = 1660.25
// LC-avg: 3
// class avg (exclude absent): 81.4
