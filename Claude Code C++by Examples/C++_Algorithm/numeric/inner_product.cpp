// ============================================================
// std::inner_product
// 分類 (Category): Numeric operations (數值演算法)
// 標頭檔 (Header):  <numeric>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/inner_product
//   * https://cplusplus.com/reference/numeric/inner_product/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::inner_product 解的問題:
//
//   「兩個範圍逐位元素配對,先做某運算結合,再累積成單一結果。」
//
// 預設行為就是「向量點積 (dot product)」:
//
//   result = init + a[0]*b[0] + a[1]*b[1] + ... + a[n-1]*b[n-1]
//
// 但 op2 (元素如何結合) 與 op1 (如何累積) 都可以自訂,所以這個函式很彈性 —
// 點積、加權平均、漢明距離、條件計數都能用一行寫完。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼是「兩個運算」?                                 │
// └────────────────────────────────────────────────────────────┘
//
// 一般化的 inner_product 是:
//
//   result = init
//   for i in 0..n-1:
//       result = op1( result, op2(a[i], b[i]) )
//
//   * op2 「結合每對元素」 — 預設是 *
//   * op1 「累積到 result」 — 預設是 +
//
// 例子:
//   * 點積:        op2 = *, op1 = +
//   * 漢明距離:    op2 = (a == b ? 0 : 1), op1 = +
//   * 條件計數:    op2 = pred(a, b) ? 1 : 0, op1 = +
//   * 邏輯 AND:    op2 = ==, op1 = &&
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、init 型別陷阱 (與 accumulate 同)                       │
// └────────────────────────────────────────────────────────────┘
//
// init 的型別決定 result 的型別:
//
//   std::vector<double> a, b;
//   double dot = std::inner_product(a.begin(), a.end(), b.begin(), 0);    // ← int!
//   double dot = std::inner_product(a.begin(), a.end(), b.begin(), 0.0);  // ← 對
//
// 對齊型別,避免精度損失。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、與 transform_reduce (C++17) 的差別                      │
// └────────────────────────────────────────────────────────────┘
//
//   * inner_product (C++98) — 順序固定,不可平行
//   * transform_reduce (C++17) — 與 inner_product 二元版等價,但可平行
//
// 大資料想平行 → 用 transform_reduce。
// 小資料、簡單 — inner_product 仍是最直觀的選擇。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt1, class InputIt2, class T>
//   T inner_product(InputIt1 first1, InputIt1 last1,
//                   InputIt2 first2, T init);
//
//   template <class InputIt1, class InputIt2, class T,
//             class BinaryOp1, class BinaryOp2>
//   T inner_product(InputIt1 first1, InputIt1 last1,
//                   InputIt2 first2, T init,
//                   BinaryOp1 op1,    // 累積 (預設 +)
//                   BinaryOp2 op2);   // 結合 (預設 *)
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次 op2 + N 次 op1
//   空間: O(1)
//
//   1. 第二範圍長度必須 ≥ 第一範圍 (沒有 last2 參數)。
//   2. init 型別決定結果型別 — 對齊元素型別!
//   3. 想平行 → 改用 transform_reduce。
//
// ============================================================

/*
補充筆記：std::inner_product
  - inner_product 同時做 pairwise 相乘與累加，常用於點積。
  - 兩個輸入範圍都必須有足夠長度。
  - 可自訂兩個運算，把它變成更一般的 zip-reduce。
  - std::inner_product 屬於數值演算法，常用來把迴圈中的累加、差分、內積或前綴和寫成標準形式。
  - accumulate 的初始值型別會決定累加結果型別；用 0 累加 double 或 long long 時可能不小心截斷或溢位。
  - reduce 可重新排序運算順序以利平行化，因此 binary operation 必須接近結合律且沒有副作用；浮點數結果可能和 accumulate 不完全相同。
  - partial_sum 和 adjacent_difference 互為常見搭配，可用來建立前綴和與差分陣列。
  - gcd/lcm 對整數工作；lcm 可能溢位，即使最後型別是 long long 也要注意輸入大小。
  - 數值演算法最容易出錯的是型別和溢位，不是語法；工作程式應刻意選擇初始值型別。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::inner_product
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. inner_product 預設做什麼?兩個自訂運算分別扮演什麼角色?
//     答:預設是向量點積 init + Σ a[i]*b[i]。六參數版裡 op1 負責「累積到 result」(預設 +)、
//         op2 負責「把每一對元素結合」(預設 *),迴圈是 result = op1(result, op2(a[i], b[i]))。
//         換掉 op2 就能做漢明距離、逐位相等計數等 zip-reduce。在 <numeric>,C++20 起 constexpr。
//     追問:複雜度?(恰好 N 次 op2 加 N 次 op1,空間 O(1))
//
// ⚠️ 陷阱. inner_product 只吃 first2、沒有 last2 —— 這代表什麼?
//     答:「第二個範圍至少和第一個一樣長」是呼叫端的前置條件,函式不會也無法檢查;
//         第二個範圍較短就是越界讀取 (UB),而且通常不會當場崩潰,只是安靜地讀到垃圾值。
//     為什麼會錯:習慣了 C++14 起 std::equal / std::mismatch 有「雙區間安全版」的人,會以為
//         標準演算法都會幫忙比長度;inner_product 是 C++98 的老介面,沒有這層保護。
//
// 🔥 Q2. init 型別在這裡有什麼影響?
//     答:和 accumulate 完全一樣 —— 結果型別就是 init 的型別 T。對 vector<double> 傳 0 會
//         全程做 int 運算而丟失小數,要寫 0.0;整數點積可能溢位時要寫 0LL。
//
// Q3. 什麼時候該把 inner_product 換成 transform_reduce?
//     答:資料量大且想平行時。inner_product 順序固定、沒有 execution policy 多載;
//         transform_reduce (C++17) 的二元版語意等價但可平行,代價是 reduce_op 必須結合且
//         可交換。小資料、或運算本身不可重排時,inner_product 仍然最直觀。
// ═══════════════════════════════════════════════════════════════════════════

#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 向量點積 ---
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{4, 5, 6};
    int dot = std::inner_product(a.begin(), a.end(), b.begin(), 0);
    std::cout << "dot product = " << dot << '\n';

    // --- 範例 2: 浮點點積 ---
    std::vector<double> x{0.5, 1.5, 2.0};
    std::vector<double> y{2.0, 0.5, 1.0};
    double d = std::inner_product(x.begin(), x.end(), y.begin(), 0.0);
    std::cout << "double dot = " << d << '\n';

    // --- 範例 3: 漢明距離 (兩字串中不同位置的數量) ---
    std::string s1 = "karolin";
    std::string s2 = "kathrin";
    int hamming = std::inner_product(
        s1.begin(), s1.end(), s2.begin(), 0,
        std::plus<>{},                                 // 累積方式: 加
        [](char a, char b){ return a != b ? 1 : 0; }); // 結合方式: 不同則 1
    std::cout << "Hamming distance = " << hamming << '\n';

    // --- 範例 4: 計算「兩向量逐位相等」的數量 ---
    std::vector<int> p{1, 2, 3, 4};
    std::vector<int> q{1, 0, 3, 0};
    int eq = std::inner_product(p.begin(), p.end(), q.begin(), 0,
                                std::plus<>{},
                                [](int a, int b){ return a == b ? 1 : 0; });
    std::cout << "equal positions = " << eq << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1313_decompress_rle_total();
    void leetcode_1773_count_items_matching_a_rule();
    void practical_weighted_average();
    void leetcode_1672_richest_via_inner_product();
    void practical_price_total_quantity();
    leetcode_1313_decompress_rle_total();
    leetcode_1773_count_items_matching_a_rule();
    practical_weighted_average();
    leetcode_1672_richest_via_inner_product();
    practical_price_total_quantity();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1313: 解壓縮編碼陣列 (Decompress Run-Length Encoded List)
// ----------------------------------------------------------------
// 題目:nums = [freq1, val1, freq2, val2, ...],解壓縮後的「總和」可用點積求得:
//      Σ freq[i] × val[i]。
//
// 為什麼用 std::inner_product:
//   分離 freqs 與 vals 後,「總和 = Σ freq × val」就是直接的點積。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_1313_decompress_rle_total() {
    std::vector<int> nums{1, 2, 3, 4};
    std::vector<int> freqs, vals;
    for (std::size_t i = 0; i < nums.size(); i += 2) {
        freqs.push_back(nums[i]);
        vals .push_back(nums[i + 1]);
    }
    int total = std::inner_product(freqs.begin(), freqs.end(), vals.begin(), 0);
    std::cout << "LC1313 total = " << total << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1773: 統計匹配檢索規則的物品數量
//                (Count Items Matching a Rule)
// ----------------------------------------------------------------
// 題目:items 中每個元素是 [type, color, name];給 ruleKey、ruleValue,
//      回傳「對應欄位 == ruleValue」的物品數。
//
// 為什麼用 std::inner_product (條件計數):
//   把 items 對應欄位取出成 col 序列,跟「全是 ruleValue」的常數序列
//   做「相等則 1」的累加 — 這是 inner_product 的條件計數用法。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_1773_count_items_matching_a_rule() {
    std::vector<std::vector<std::string>> items{
        {"phone","blue","pixel"},
        {"computer","silver","lenovo"},
        {"phone","gold","iphone"}
    };
    std::string ruleKey = "color", ruleValue = "silver";
    int field = (ruleKey == "type" ? 0 : (ruleKey == "color" ? 1 : 2));
    std::vector<std::string> col;
    for (auto& it : items) col.push_back(it[field]);
    std::vector<std::string> rhs(col.size(), ruleValue);
    int cnt = std::inner_product(
        col.begin(), col.end(), rhs.begin(), 0,
        std::plus<>{},
        [](const std::string& a, const std::string& b){ return a == b ? 1 : 0; });
    std::cout << "LC1773 count = " << cnt << '\n';
}

// ----------------------------------------------------------------
// 實務範例:加權平均 (Weighted Average)
// ----------------------------------------------------------------
// 場景:學生 N 科成績與對應學分,求加權平均 = Σ(score × credit) / Σ credit。
//
// 為什麼用 std::inner_product:
//   Σ(score × credit) 完全是「兩序列點積」 — 一行解決。
void practical_weighted_average() {
    std::vector<double> scores{ 85.0, 92.0, 70.0, 78.0 };
    std::vector<double> credits{ 3.0,  2.0,  1.0,  4.0 };
    double weighted_sum = std::inner_product(
        scores.begin(), scores.end(), credits.begin(), 0.0);
    double total_credit = std::accumulate(credits.begin(), credits.end(), 0.0);
    double avg = weighted_sum / total_credit;
    std::cout << "weighted average = " << avg << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1672 用 inner_product:對單一客戶資產取總和
// ----------------------------------------------------------------
// 題目:給單個客戶的存款向量,求總資產。inner_product 與「全 1 向量」的
//      點積等價於 accumulate — 用來示範「用點積實作求和」的視角。
//
// 為什麼用 std::inner_product:
//   sum = Σ a[i] × 1。雖然 accumulate 更直接,但這個視角能幫助理解 inner_product
//   的廣泛適用性。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_1672_richest_via_inner_product() {
    std::vector<int> row{3, 2, 5, 7};
    std::vector<int> ones(row.size(), 1);
    int total = std::inner_product(row.begin(), row.end(), ones.begin(), 0);
    std::cout << "LC1672-IP total: " << total << '\n';
}

// ----------------------------------------------------------------
// 實務範例:購物車 — 單價 × 數量 = 應付金額
// ----------------------------------------------------------------
// 場景:每樣商品有單價與數量,總金額 = Σ(price × qty)。
//      經典 inner_product 場景。
void practical_price_total_quantity() {
    std::vector<double> prices{50, 120, 75, 30};
    std::vector<int>    qty   { 2,   1, 3, 5};
    double total = std::inner_product(prices.begin(), prices.end(), qty.begin(), 0.0);
    std::cout << "cart total: " << total << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra inner_product.cpp -o inner_product

// === 預期輸出 ===
// dot product = 32
// double dot = 3.75
// Hamming distance = 3
// equal positions = 2
// LC1313 total = 14
// LC1773 count = 1
// weighted average = 82.1
// LC1672-IP total: 17
// cart total: 595
