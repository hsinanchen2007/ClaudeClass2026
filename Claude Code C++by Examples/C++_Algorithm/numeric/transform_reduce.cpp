// ============================================================
// std::transform_reduce    (C++17 起)
// 分類 (Category): Numeric operations (數值演算法)
// 標頭檔 (Header):  <numeric>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/transform_reduce
//   * https://cplusplus.com/reference/numeric/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::transform_reduce 把「先 transform、再 reduce」融合成一個函式 —
// 一行就完成「映射 + 歸約」的整套資料處理。
//
// 兩種形式:
//
//   一元: result = init + Σ f(a[i])
//   二元: result = init + Σ f(a[i], b[i])
//
// 二元版本與 std::inner_product 等價,但允許平行化 (因為 reduce 性質)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼這個函式存在?                                   │
// └────────────────────────────────────────────────────────────┘
//
// 「映射 + 歸約 (map-reduce)」是非常通用的資料處理模式:
//
//   * 訂單金額 = Σ (數量 × 單價)
//   * 學生加權平均 = Σ (分數 × 學分) / Σ 學分
//   * 字串總長度 = Σ len(s)
//   * 平方和 = Σ x²
//
// 這些都可寫成「對每個元素先做變換,再加總」 — transform_reduce 一行解決。
//
// 想要「函式語言風格」(map-reduce / map-fold) 的 C++ 程式碼,
// transform_reduce 是核心工具之一。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、與相關函式的對照                                       │
// └────────────────────────────────────────────────────────────┘
//
//   ┌──────────────────────────┬───────────────────────────────────┐
//   │ accumulate               │ 順序固定;只「累積」                │
//   │ reduce                   │ 可平行;只「累積」                  │
//   │ inner_product            │ 順序固定;雙序列點積                │
//   │ transform_reduce         │ 可平行;映射 + 累積 (一元/二元)     │
//   │ partial_sum              │ 順序固定;輸出與輸入等長的「前綴和」│
//   │ inclusive_scan           │ 可平行;結果同 partial_sum          │
//   │ exclusive_scan           │ 可平行;前綴和但「排除自己」        │
//   │ transform_inclusive_scan │ 可平行;先映射再做 inclusive_scan    │
//   │ transform_exclusive_scan │ 可平行;先映射再做 exclusive_scan    │
//   └──────────────────────────┴───────────────────────────────────┘
//
// 「資料量大 + 想平行 + 想 map+reduce 一行解決」 → transform_reduce 是首選。
//
// 而 transform_inclusive_scan / transform_exclusive_scan 則是 transform_reduce
// 的「scan 版本」 — 不歸約成單一值,而是輸出「每個位置的累積值」(N 個結果)。
// 它們是 partial_sum 的「平行 + 帶映射」版本,本檔同時示範。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、reduce_op 必須結合律 + 交換律                          │
// └────────────────────────────────────────────────────────────┘
//
// 與 reduce 同樣的限制:reduce_op 必須符合結合律與交換律。
// 字串串接不行 (沒有交換律)、減法不行 (不結合)。
//
// transform_op 倒是沒有限制 — 它只負責對每個元素 (或每對元素) 做映射。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   // 二元版 (類似 inner_product,但可平行)
//   template <class InputIt1, class InputIt2, class T>
//   T transform_reduce(InputIt1 first1, InputIt1 last1,
//                      InputIt2 first2, T init);
//
//   template <class InputIt1, class InputIt2, class T,
//             class BinaryOp1, class BinaryOp2>
//   T transform_reduce(InputIt1 first1, InputIt1 last1,
//                      InputIt2 first2, T init,
//                      BinaryOp1 reduce_op,
//                      BinaryOp2 transform_op);
//
//   // 一元版
//   template <class InputIt, class T,
//             class BinaryOp, class UnaryOp>
//   T transform_reduce(InputIt first, InputIt last, T init,
//                      BinaryOp reduce_op, UnaryOp transform_op);
//
//   // 平行版 (各多載皆有 ExecutionPolicy 多載)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 序列 O(N);平行 O(N/P)
//   空間: O(1)
//
//   1. reduce_op 必須結合律 + 交換律 (字串串接不行)。
//   2. 一元版本可避免建立暫存的 transformed 容器 — 比 transform + reduce 省。
//   3. 二元版本與 inner_product 等價,但可平行。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、scan 變體 — transform_inclusive_scan / transform_exclusive_scan │
// └────────────────────────────────────────────────────────────┘
//
// 這兩個是 C++17 同步引入的「映射 + 前綴和」融合函式 — 與其先 std::transform
// 出一個暫存容器、再 std::inclusive_scan,不如一行解決:
//
//   transform_inclusive_scan(first, last, d_first, BinaryOp, UnaryOp);
//   transform_inclusive_scan(first, last, d_first, BinaryOp, UnaryOp, init);
//   transform_exclusive_scan(first, last, d_first, init, BinaryOp, UnaryOp);
//
// 等同於:
//
//   transform_inclusive_scan = transform → inclusive_scan
//   transform_exclusive_scan = transform → exclusive_scan
//
// 兩者皆可搭配 std::execution::par 平行化。reduce_op 仍須結合律。
//
// 典型應用:
//   * 「平方累積和」 — 每位置都要前 i 個元素的平方累積。
//   * 「絕對值累積和」 — 對 signed 序列做風險度量。
//   * 「字串長度的累積寫入位置」 — 一行算出每筆字串拼接後的起始 offset。
//
// ============================================================

/*
補充筆記：std::transform_reduce
  - transform_reduce 先轉換元素再歸約，能避免建立中間容器。
  - 平行或重排時，reduce 運算必須能安全改變結合順序。
  - 它適合點積、平方和、計算加權分數等模式。
  - std::transform_reduce 屬於數值演算法，常用來把迴圈中的累加、差分、內積或前綴和寫成標準形式。
  - accumulate 的初始值型別會決定累加結果型別；用 0 累加 double 或 long long 時可能不小心截斷或溢位。
  - reduce 可重新排序運算順序以利平行化，因此 binary operation 必須接近結合律且沒有副作用；浮點數結果可能和 accumulate 不完全相同。
  - partial_sum 和 adjacent_difference 互為常見搭配，可用來建立前綴和與差分陣列。
  - gcd/lcm 對整數工作；lcm 可能溢位，即使最後型別是 long long 也要注意輸入大小。
  - 數值演算法最容易出錯的是型別和溢位，不是語法；工作程式應刻意選擇初始值型別。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::transform_reduce (C++17)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. transform_reduce 解決什麼問題?比 transform + reduce 好在哪?
//     答:把「先映射、再歸約」融合成一次走訪 (map-reduce)。好處是不必先 std::transform 出一個
//         中間容器 —— 省下 O(N) 暫存空間與一次額外走訪。C++17 起,宣告在 <numeric>。
//         兩種形式:一元版算 init 加上 Σ f(a[i]);二元版算 init 加上 Σ f(a[i], b[i])。
//     追問:典型場景?(點積、平方和 / L2 norm、加權平均、字串總長度、count_if 的函式式寫法)
//
// 🔥 Q2. 二元版 transform_reduce 和 inner_product 是什麼關係?
//     答:預設行為等價 (都是點積),差別在契約:inner_product 保證順序固定、沒有 policy 多載;
//         transform_reduce 順序未定,但可搭配 execution policy 平行化。也因此
//         transform_reduce 的 reduce_op 必須 associative + commutative,inner_product 則不必。
//
// ⚠️ 陷阱. transform_op 和 reduce_op 誰有結合律的要求?參數順序怎麼記?
//     答:限制只落在 reduce_op (必須結合且可交換);transform_op 只是逐元素映射,沒有結合律
//         要求,但同樣不該有副作用、也不能依賴呼叫次序。一元版的簽章是
//         (first, last, init, reduce_op, unary_op) —— reduce 在前、transform 在後,
//         和函式名稱的字面順序相反,很容易寫反。
//     為什麼會錯:照著名字「transform 再 reduce」去猜參數順序,結果剛好顛倒。
//
// Q3. transform_inclusive_scan 與 transform_exclusive_scan 的 init 位置為什麼不同?
//     答:exclusive 版的 out[0] 就是 init,所以 init 是必填,排在 d_first 之後、BinaryOp 之前;
//         inclusive 版的 init 可省略,若要給則排在最後。這兩支輸出 N 個結果 (scan) 而非單一值,
//         同樣可搭配執行策略。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 一元 transform_reduce — 平方和 Σ x² ---
    std::vector<int> v{1, 2, 3, 4, 5};
    int ssq = std::transform_reduce(v.begin(), v.end(), 0,
                                    std::plus<>{},
                                    [](int x){ return x * x; });
    std::cout << "sum of squares = " << ssq << '\n';

    // --- 範例 2: 二元版 — 點積 (等同 inner_product) ---
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{4, 5, 6};
    int dot = std::transform_reduce(a.begin(), a.end(), b.begin(), 0);
    std::cout << "dot = " << dot << '\n';

    // --- 範例 3: 計算字串總長度 ---
    std::vector<std::string> words{"hello", "world", "C++"};
    std::size_t len = std::transform_reduce(
        words.begin(), words.end(), std::size_t{0},
        std::plus<>{},
        [](const std::string& s){ return s.size(); });
    std::cout << "total length = " << len << '\n';

    // ============================================================
    // === scan 變體: transform_inclusive_scan / transform_exclusive_scan ===
    // ============================================================

    // --- 範例 4: transform_inclusive_scan — 平方的累積和 ---
    //   先把每個元素 x 變成 x²,再做 inclusive_scan。
    //   {1,2,3,4} → 平方 {1,4,9,16} → 累進和 {1, 5, 14, 30}
    std::vector<int> nums{1, 2, 3, 4};
    std::vector<int> sq_scan(nums.size());
    std::transform_inclusive_scan(
        nums.begin(), nums.end(), sq_scan.begin(),
        std::plus<int>{},                    // reduce_op
        [](int x){ return x * x; });         // unary transform
    std::cout << "transform_inclusive_scan (squares cumulative): ";
    for (int x : sq_scan) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 5: transform_exclusive_scan — 絕對值累積 (排除自己) ---
    //   {-3, 5, -2, 4} → |x| {3,5,2,4} → exclusive 累進(init=0) {0, 3, 8, 10}
    std::vector<int> signed_v{-3, 5, -2, 4};
    std::vector<int> abs_es(signed_v.size());
    std::transform_exclusive_scan(
        signed_v.begin(), signed_v.end(), abs_es.begin(),
        /*init=*/0,
        std::plus<int>{},                    // reduce_op
        [](int x){ return x < 0 ? -x : x; }); // unary transform (abs)
    std::cout << "transform_exclusive_scan (abs cumulative): ";
    for (int x : abs_es) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1672_richest_customer_wealth_tr();
    void leetcode_1480_running_sum_total_tr();
    void practical_cart_total();
    void practical_string_concat_offsets();
    void leetcode_2overlap_count_truthy();
    void practical_vector_norm();
    leetcode_1672_richest_customer_wealth_tr();
    leetcode_1480_running_sum_total_tr();
    practical_cart_total();
    practical_string_concat_offsets();
    leetcode_2overlap_count_truthy();
    practical_vector_norm();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1672: 最富有客戶的資產總量 (用 transform_reduce 優雅版)
// ----------------------------------------------------------------
// 題目:給 m × n 的 accounts,求最富有客戶的總資產。
//
// 為什麼用 std::transform_reduce:
//   一行解 — 「對每列 reduce 求和」(transform),「對所有列總和取 max」(reduce_op)。
//   兩步合一,函式語言風格的優雅寫法。
//
// 複雜度:時間 O(m × n);空間 O(1)。
void leetcode_1672_richest_customer_wealth_tr() {
    std::vector<std::vector<int>> accounts{{1,5},{7,3},{3,5}};
    int max_wealth = std::transform_reduce(
        accounts.begin(), accounts.end(), 0,
        [](int a, int b){ return std::max(a, b); },
        [](const std::vector<int>& row){
            return std::accumulate(row.begin(), row.end(), 0);
        });
    std::cout << "LC1672 (TR) = " << max_wealth << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1480 變體:Σ runningSum (Sum of Running Sum)
// ----------------------------------------------------------------
// 題目延伸:給 nums,計算 Σ_i Σ_{j ≤ i} nums[j] = Σ_j nums[j] × (n - j)。
//
// 為什麼用 std::transform_reduce (二元版):
//   等價於對 nums 與 weight = (n, n-1, ..., 1) 做點積 — 二元版一行寫完。
void leetcode_1480_running_sum_total_tr() {
    std::vector<int> nums{1, 2, 3, 4};
    int n = static_cast<int>(nums.size());
    std::vector<int> weight(n);
    for (int i = 0; i < n; ++i) weight[i] = n - i;
    int total = std::transform_reduce(
        nums.begin(), nums.end(), weight.begin(), 0);
    std::cout << "LC1480 (TR) total of runningSum = " << total << '\n';
}

// ----------------------------------------------------------------
// 實務範例:購物車總額 (Cart Total)
// ----------------------------------------------------------------
// 場景:購物車有 N 件商品,quantities[i] × prices[i] 求總金額。
//
// 為什麼用 std::transform_reduce (二元版):
//   就是「點積」 — Σ qty × price — 一行寫完且可平行化。
//   完全符合「加權求和」語意,程式碼可讀性高。
void practical_cart_total() {
    std::vector<int>    quantities{2, 1, 4, 3};
    std::vector<double> prices{ 50.0, 250.0, 30.0, 80.0 };
    double total = std::transform_reduce(
        quantities.begin(), quantities.end(),
        prices.begin(), 0.0);
    std::cout << "cart total = " << total << '\n';
}

// ----------------------------------------------------------------
// 實務範例:字串拼接寫入 offset (Concatenation Offsets)
// ----------------------------------------------------------------
// 場景:多執行緒要把 N 筆字串拼接到同一個大 buffer。為了能讓每個執行緒
// 「平行寫入而不互相覆蓋」,需要先計算出「我這筆字串應該寫在 buffer 的
// 第幾個 byte 開始」。
//
// 為什麼用 std::transform_exclusive_scan:
//   * 「我之前所有字串的長度總和」 = exclusive_scan 語意。
//   * 「先把字串轉成長度」 = transform。
//   * 兩步融合 + 可平行 = transform_exclusive_scan,一行解決。
//
// 複雜度:時間 O(N);空間 O(N) (offsets)。
void practical_string_concat_offsets() {
    std::vector<std::string> parts{"hello", " ", "world", "!", "!!"};
    std::vector<std::size_t> offsets(parts.size());
    std::transform_exclusive_scan(
        parts.begin(), parts.end(), offsets.begin(),
        /*init=*/std::size_t{0},
        std::plus<std::size_t>{},
        [](const std::string& s){ return s.size(); });
    std::cout << "concat offsets: ";
    for (auto x : offsets) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1684 概念:計算「符合條件」的元素個數 (count_if 替代方案)
// ----------------------------------------------------------------
// 題目:給陣列 nums,計算其中「大於閾值 T」的元素數量。
//
// 為什麼用 std::transform_reduce:
//   把 predicate 結果 (0/1) 用 transform,再 reduce 加總 —
//   是 count_if 的「函式式風格」等價;且可平行化。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2overlap_count_truthy() {
    std::vector<int> nums{10, 5, 30, 25, 8, 50};
    int T = 20;
    int cnt = std::transform_reduce(
        nums.begin(), nums.end(), 0,
        std::plus<int>{},
        [T](int x){ return x > T ? 1 : 0; });
    std::cout << "count > " << T << ": " << cnt << '\n';
}

// ----------------------------------------------------------------
// 實務範例:計算向量歐式範數 (L2 norm)
// ----------------------------------------------------------------
// 場景:幾何或機器學習中常要算向量長度 sqrt(Σ x²)。
//      用 transform_reduce 一行算出 Σ x²,再 sqrt。
void practical_vector_norm() {
    std::vector<double> v{3.0, 4.0};   // 3-4-5 直角三角形
    double sq_sum = std::transform_reduce(
        v.begin(), v.end(), 0.0,
        std::plus<double>{},
        [](double x){ return x * x; });
    std::cout << "L2 norm: " << std::sqrt(sq_sum) << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra transform_reduce.cpp -o transform_reduce

// === 預期輸出 ===
// sum of squares = 55
// dot = 32
// total length = 13
// transform_inclusive_scan (squares cumulative): 1 5 14 30
// transform_exclusive_scan (abs cumulative): 0 3 8 10
// LC1672 (TR) = 10
// LC1480 (TR) total of runningSum = 20
// cart total = 710
// concat offsets: 0 5 6 11 12
// count > 20: 3
// L2 norm: 5
