// ============================================================
// std::reduce    (C++17 起)
// 分類 (Category): Numeric operations (數值演算法)
// 標頭檔 (Header):  <numeric>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/reduce
//   * https://cplusplus.com/reference/numeric/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::reduce 是 std::accumulate 的「平行版」。語意上跟 accumulate 一樣,
// 都是「把範圍的元素累積成單一結果」,但有兩個關鍵差別:
//
//   1. 「不保證執行順序」 — 可被切成多塊各自處理後合併,適合平行化。
//   2. 因此「op 必須符合結合律與交換律」 — 否則結果未定義。
//
// 配合 C++17 的執行策略 (execution policy),reduce 可以一行寫出
// 「平行求和」這類典型平行運算:
//
//   std::reduce(std::execution::par, v.begin(), v.end(), 0LL);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、accumulate vs reduce 一覽                               │
// └────────────────────────────────────────────────────────────┘
//
//   ┌──────────────┬─────────────────────────────────────┐
//   │ accumulate   │ 順序固定 (左結合);無法平行          │
//   │ reduce       │ 順序未定;可平行 (需執行策略)         │
//   └──────────────┴─────────────────────────────────────┘
//
// 對符合「結合律 + 交換律」的運算 (+、*、min、max、bit_or、bit_and ...),
// 兩者結果相同。對「字串串接 (+)」這類有順序要求的,只能用 accumulate!
//
// 浮點加法理論上不結合 (因精度損失),accumulate 與 reduce 結果可能微小差異 —
// 對需要 bit-exact 的金融計算要小心。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、init 是「可選」的                                      │
// └────────────────────────────────────────────────────────────┘
//
// std::reduce 有三個多載:
//   * reduce(first, last)              — init = T(),預設 +
//   * reduce(first, last, init)        — 自訂 init,預設 +
//   * reduce(first, last, init, op)    — 自訂 init 與 op
//
// 不指定 init 時,T() 對 int 是 0、對 double 是 0.0。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 大資料總和 (用 par 策略):金融、log 統計、報表
//   * 求最大/最小值的歸約
//   * 任何「結合律 + 交換律」的累積運算
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt>
//   T reduce(InputIt first, InputIt last);
//
//   template <class InputIt, class T>
//   T reduce(InputIt first, InputIt last, T init);
//
//   template <class InputIt, class T, class BinaryOp>
//   T reduce(InputIt first, InputIt last, T init, BinaryOp op);
//
//   // 平行版 (需 <execution>)
//   template <class ExecPolicy, class FwdIt, class T, class BinaryOp>
//   T reduce(ExecPolicy&&, FwdIt first, FwdIt last, T init, BinaryOp op);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 序列 O(N);平行 O(N/P) (P 為核心數)
//   空間: O(1)
//
//   1. op 必須結合律 + 交換律 — 字串串接不行!
//   2. 浮點結果可能與 accumulate 略不同 (精度)。
//   3. init 的型別決定結果型別。
//
// ============================================================

/*
補充筆記：std::reduce
  - reduce 允許標準庫重排運算順序，這是它和 accumulate 的重要差異。
  - 運算若不是結合律安全，例如浮點加法或帶副作用 lambda，結果可能和 accumulate 不同。
  - 沒有明確平行 execution policy 時仍可重排，學習時不要把它當成 accumulate 的同義詞。
  - std::reduce 屬於數值演算法，常用來把迴圈中的累加、差分、內積或前綴和寫成標準形式。
  - accumulate 的初始值型別會決定累加結果型別；用 0 累加 double 或 long long 時可能不小心截斷或溢位。
  - reduce 可重新排序運算順序以利平行化，因此 binary operation 必須接近結合律且沒有副作用；浮點數結果可能和 accumulate 不完全相同。
  - partial_sum 和 adjacent_difference 互為常見搭配，可用來建立前綴和與差分陣列。
  - gcd/lcm 對整數工作；lcm 可能溢位，即使最後型別是 long long 也要注意輸入大小。
  - 數值演算法最容易出錯的是型別和溢位，不是語法；工作程式應刻意選擇初始值型別。
*/
#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <numeric>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 4, 5};

    // --- 範例 1: 預設求和 ---
    std::cout << "sum = " << std::reduce(v.begin(), v.end()) << '\n';

    // --- 範例 2: init 與運算指定 ---
    int prod = std::reduce(v.begin(), v.end(), 1, std::multiplies<int>{});
    std::cout << "product = " << prod << '\n';

    // --- 範例 3: 求 max ---
    int m = std::reduce(v.begin(), v.end(),
                        std::numeric_limits<int>::min(),
                        [](int a, int b){ return std::max(a, b); });
    std::cout << "max = " << m << '\n';

    // --- 範例 4: 平行版本 (註解掉,本地未必有 TBB) ---
    // #include <execution>
    // int s = std::reduce(std::execution::par, v.begin(), v.end(), 0);

    // === LeetCode / 實務範例 ===
    void leetcode_2overall_sum_simple();
    void leetcode_1389_create_target_array_sum();
    void practical_big_data_sum();
    void leetcode_1281_subtract_product_sum();
    void practical_max_via_reduce();
    leetcode_2overall_sum_simple();
    leetcode_1389_create_target_array_sum();
    practical_big_data_sum();
    leetcode_1281_subtract_product_sum();
    practical_max_via_reduce();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 子題:陣列總和
// ----------------------------------------------------------------
// 題目:求 nums 所有元素之和。
//      LeetCode 上很多題第一步都是「先求總和」(LC 1991、LC 724 等)。
//
// 為什麼用 std::reduce:
//   單純加總是最典型的歸約;若資料量大,可平行化。
//
// 複雜度:序列 O(n);平行 O(n/p);空間 O(1)。
void leetcode_2overall_sum_simple() {
    std::vector<int> nums{1, 7, 3, 6, 5, 6};
    int total = std::reduce(nums.begin(), nums.end(), 0);
    std::cout << "LC sum: " << total << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1389: 按既定順序建立目標陣列 (Create Target Array)
// ----------------------------------------------------------------
// 題目:依 index[i] 把 nums[i] 插入 target 對應位置,回傳 target。
//
// 為什麼順便用 std::reduce:
//   驗證「插入後 target 的總和應等於 nums 的總和」(不變量檢查) —
//   這是平行版求和的典型輔助用途。
void leetcode_1389_create_target_array_sum() {
    std::vector<int> nums{0, 1, 2, 3, 4};
    std::vector<int> index{0, 1, 2, 2, 1};
    std::vector<int> target;
    for (std::size_t i = 0; i < nums.size(); ++i)
        target.insert(target.begin() + index[i], nums[i]);
    int s1 = std::reduce(nums.begin(),   nums.end(),   0);
    int s2 = std::reduce(target.begin(), target.end(), 0);
    std::cout << "LC1389 nums_sum=" << s1
              << " target_sum=" << s2
              << (s1 == s2 ? " (consistent)" : " (mismatch)") << '\n';
}

// ----------------------------------------------------------------
// 實務範例:大資料求和 (可平行化)
// ----------------------------------------------------------------
// 場景:對「百萬筆訂單金額」加總,且機器有多核 → 可考慮平行 reduce。
//
// 為什麼用 std::reduce:
//   加法滿足結合律 + 交換律,符合 reduce 要求。
//   實作可切成多塊各自加總後合併,提升大資料的吞吐量。
void practical_big_data_sum() {
    std::vector<long long> sales(1000, 0);
    for (int i = 0; i < 1000; ++i) sales[i] = 100 + i;
    long long total = std::reduce(sales.begin(), sales.end(), 0LL);
    std::cout << "Big-data sum = " << total << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1281: 整數的各位積與和之差
// ----------------------------------------------------------------
// 題目:給整數 n,計算「各位數字之積」減去「各位數字之和」。
//
// 為什麼用 std::reduce:
//   把每位數字蒐集到 vector 後,用 reduce 一次得到「積」和「和」 — 兩次操作。
//
// 複雜度:時間 O(log n);空間 O(log n)。
void leetcode_1281_subtract_product_sum() {
    int n = 234;
    std::vector<int> digits;
    while (n) { digits.push_back(n % 10); n /= 10; }
    int sum = std::reduce(digits.begin(), digits.end(), 0);
    int prod = std::reduce(digits.begin(), digits.end(), 1, std::multiplies<int>{});
    std::cout << "LC1281: " << (prod - sum) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:用 reduce 求最大值 (平行友善版)
// ----------------------------------------------------------------
// 場景:大資料找最大值,reduce 配自訂 binary op (max) 滿足結合律,
//      可平行化 (執行策略 par)。比 std::max_element 在多核情境下更具優勢。
void practical_max_via_reduce() {
    std::vector<int> data{42, 99, 17, 56, 23, 88, 31};
    int mx = std::reduce(data.begin(), data.end(),
                         std::numeric_limits<int>::min(),
                         [](int a, int b){ return std::max(a, b); });
    std::cout << "max via reduce: " << mx << '\n';
}

// === 預期輸出 (Expected output) ===
// sum = 15
// product = 120
// max = 5
// LC sum: 28
// LC1389 nums_sum=10 target_sum=10 (consistent)
// Big-data sum = 599500
// LC1281: 15
// max via reduce: 99
