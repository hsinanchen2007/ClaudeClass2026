// ============================================================
// std::transform
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/transform
//   * https://cplusplus.com/reference/algorithm/transform/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::transform 解的問題:
//
//   「對範圍中的元素套一個函式,把結果寫到輸出端。」
//
// 這正是 functional programming 中的 `map` 操作 — 把一個 sequence
// 中的每個值,經過函式的「映射」變成另一個 sequence。
//
// transform 有兩種版本:
//
//   ┌──────────┬──────────────────────────────────────────────┐
//   │ 一元版    │ result[i] = op(input[i])                     │
//   │ 二元版    │ result[i] = op(input1[i], input2[i])         │
//   └──────────┴──────────────────────────────────────────────┘
//
// 二元版 (有兩個輸入序列) 對應「對位運算」:兩個 vector 對位相加、
// 對位平均、對位點積...
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、transform vs for_each vs replace 的選擇                │
// └────────────────────────────────────────────────────────────┘
//
//   * transform : 「映射」 — 給每個元素一個函式,寫入新結果
//   * for_each  : 「動作」 — 對每個元素做事,通常不關心回傳值
//   * replace   : 「替換」 — 條件式覆蓋特定值,語意更窄
//
// 三者中,transform 最像「資料管線」 — 輸入一段 → 處理 → 輸出一段。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、可以「原地 (in-place)」變換                            │
// └────────────────────────────────────────────────────────────┘
//
// d_first 可以等於 first1 — 即輸出寫回原本的位置,不需要額外容器。
// 例如把字串就地轉成大寫:
//
//   std::transform(s.begin(), s.end(), s.begin(),
//                  [](unsigned char c){ return std::toupper(c); });
//
// 這是 transform 跟 for_each 都能做的事,但 transform 的語意更
// 接近「我要的就是逐元素映射」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、為什麼沒有 transform_if?                              │
// └────────────────────────────────────────────────────────────┘
//
// 標準函式庫沒有 transform_if (要過濾再轉換)。常見替代方法:
//
//   1. 串接 copy_if + transform
//   2. C++20 ranges:  views::filter | views::transform
//   3. 自己手寫 for 迴圈 (這也是合理選擇)
//
// 「不過度設計」是 STL 設計哲學的一部分。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   // 一元
//   template <class InputIt, class OutputIt, class UnaryOp>
//   OutputIt transform(InputIt first, InputIt last,
//                      OutputIt d_first, UnaryOp op);
//
//   // 二元
//   template <class InputIt1, class InputIt2, class OutputIt, class BinaryOp>
//   OutputIt transform(InputIt1 first1, InputIt1 last1,
//                      InputIt2 first2,
//                      OutputIt d_first, BinaryOp op);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次呼叫 op
//   空間: O(1) (除了輸出空間)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 目的端必須有足夠空間,或用 back_inserter。
//   2. 二元版的第二範圍長度必須 >= 第一範圍。
//   3. op 應為「純函式」(無副作用),平行版尤其要求這點。
//   4. 想要「映射 + 累加」一次完成,改用 std::transform_reduce (C++17)。
//
// ============================================================

/*
補充筆記：std::transform
  - std::transform 是把輸入範圍映射成輸出範圍；它適合表達「每個元素如何轉換」。
  - unary 版本吃一段輸入，binary 版本吃兩段輸入；binary 版本要確保第二段長度足夠。
  - lambda 若捕獲外部狀態，請確認沒有在 transform 過程中造成難以推理的副作用。
  - std::transform 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <cctype>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 4, 5};

    // --- 範例 1: 一元變換 — 平方 ---
    std::vector<int> sq(v.size());
    std::transform(v.begin(), v.end(), sq.begin(),
                   [](int x){ return x * x; });
    std::cout << "squared: ";
    for (int x : sq) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 二元變換 — 兩個 vector 對位相加 ---
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{10, 20, 30};
    std::vector<int> sum(3);
    std::transform(a.begin(), a.end(), b.begin(), sum.begin(),
                   [](int x, int y){ return x + y; });
    std::cout << "sum: ";
    for (int x : sum) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 就地變換 (in-place) — 把字串轉大寫 ---
    std::string s = "Hello, C++!";
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    std::cout << "upper: " << s << '\n';

    // --- 範例 4: 變換時改變型別 (int → double 倒數) ---
    std::vector<double> recip(v.size());
    std::transform(v.begin(), v.end(), recip.begin(),
                   [](int x){ return 1.0 / x; });
    std::cout << "reciprocal: ";
    for (double d : recip) std::cout << d << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1672_richest_customer_wealth();
    void leetcode_1929_concatenate_array();
    void practical_sensor_average();
    void leetcode_709_to_lower_case();
    void practical_currency_convert();
    leetcode_1672_richest_customer_wealth();
    leetcode_1929_concatenate_array();
    practical_sensor_average();
    leetcode_709_to_lower_case();
    practical_currency_convert();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1672: 最富有客戶的資產總量 (Richest Customer Wealth)
// ----------------------------------------------------------------
// 題目:給 m × n 矩陣 accounts,求「每位客戶總資產的最大值」。
//
// 為什麼用 std::transform:
//   「對每一列做加總」 = 對每一列做映射 (row → sum) — 正是 transform。
//   一元 transform 把 vector<vector<int>> 映射為 vector<int>,
//   再對結果取 max_element。
//
// 解法步驟:
//   1. transform(accounts, wealth, [](row){ return accumulate(row); })
//   2. *max_element(wealth) 即為答案。
//
// 複雜度:時間 O(m × n);空間 O(m)。
void leetcode_1672_richest_customer_wealth() {
    std::vector<std::vector<int>> accounts{
        {1,2,3},
        {3,2,1},
        {7,1,2},
    };
    std::vector<int> wealth(accounts.size());
    std::transform(accounts.begin(), accounts.end(), wealth.begin(),
                   [](const std::vector<int>& row){
                       return std::accumulate(row.begin(), row.end(), 0);
                   });
    int rich = *std::max_element(wealth.begin(), wealth.end());
    std::cout << "LC1672: wealth=";
    for (int w : wealth) std::cout << w << ' ';
    std::cout << "max=" << rich << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1929: 陣列串聯 (Concatenation of Array)
// ----------------------------------------------------------------
// 題目:給陣列 nums (長度 n),回傳一個長度 2n 的陣列 ans,
//      其中 ans[i] = nums[i % n] (即把 nums 接在自己後面再回傳)。
//
// 為什麼用 std::transform:
//   雖然單純 copy 也行,這裡示範用 transform 把「位置 i」映射為
//   「nums[i % n]」 — 看清楚 transform 在「索引→值」的映射也很自然。
//
// 解法步驟:
//   1. 建立索引 0..2n-1。
//   2. 用 transform 把每個索引 i 映射為 nums[i % n]。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_1929_concatenate_array() {
    std::vector<int> nums{1, 2, 1};
    int n = static_cast<int>(nums.size());
    std::vector<int> idx(2 * n);
    std::iota(idx.begin(), idx.end(), 0);   // 0, 1, 2, ..., 2n-1
    std::vector<int> ans(2 * n);
    std::transform(idx.begin(), idx.end(), ans.begin(),
                   [&](int i){ return nums[i % n]; });
    std::cout << "LC1929: ";
    for (int x : ans) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:雙感測器讀數對位平均
// ----------------------------------------------------------------
// 場景:兩個感測器在固定取樣點輸出讀數 a[i] 與 b[i],
//      後處理需要兩感測器讀數的「平均」。
//
// 為什麼用 std::transform (二元版):
//   「對位運算」是二元 transform 的天然場景 — 一行寫完。
void practical_sensor_average() {
    std::vector<double> a{1.0, 2.0, 3.0, 4.0};
    std::vector<double> b{1.5, 2.5, 3.5, 4.5};
    std::vector<double> avg(a.size());
    std::transform(a.begin(), a.end(), b.begin(), avg.begin(),
                   [](double x, double y){ return (x + y) / 2.0; });
    std::cout << "Sensor avg: ";
    for (double d : avg) std::cout << d << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 709: 轉換成小寫字母 (To Lower Case)
// ----------------------------------------------------------------
// 題目:給字串 s,把所有大寫字母轉成小寫,回傳結果。
//
// 為什麼用 std::transform:
//   經典「對每個字元做轉換」 — 一行 transform + tolower 解決。
//
// 複雜度:時間 O(n);空間 O(1) (in-place)。
void leetcode_709_to_lower_case() {
    std::string s = "Hello WORLD 123";
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    std::cout << "LC709: " << s << '\n';
}

// ----------------------------------------------------------------
// 實務範例:幣別換算 — 把一批 USD 金額轉成 TWD
// ----------------------------------------------------------------
// 場景:報表中要把 USD 欄位用「即時匯率」換算成 TWD,
//      transform 一次處理整個欄位,語意清晰、效能良好。
void practical_currency_convert() {
    std::vector<double> usd{100.0, 250.5, 79.99, 1234.56};
    std::vector<double> twd(usd.size());
    const double rate = 31.5;
    std::transform(usd.begin(), usd.end(), twd.begin(),
                   [rate](double x){ return x * rate; });
    std::cout << "TWD:";
    for (double t : twd) std::cout << ' ' << t;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// squared: 1 4 9 16 25
// sum: 11 22 33
// upper: HELLO, C++!
// reciprocal: 1 0.5 0.333333 0.25 0.2
// LC1672: wealth=6 6 10 max=10
// LC1929: 1 2 1 1 2 1
// Sensor avg: 1.25 2.25 3.25 4.25
// LC709: hello world 123
// TWD: 3150 7890.75 2519.69 38888.6
