// ============================================================
// std::partial_sum
// 分類 (Category): Numeric operations (數值演算法)
// 標頭檔 (Header):  <numeric>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/partial_sum
//   * https://cplusplus.com/reference/numeric/partial_sum/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 — 「前綴和 (Prefix Sum)」                      │
// └────────────────────────────────────────────────────────────┘
//
// std::partial_sum 解的問題:
//
//   「對範圍計算『累進和』:每個位置都是『從頭累加到該位置』的總和。」
//
// 公式:
//
//   out[0] = in[0]
//   out[i] = in[0] + in[1] + ... + in[i]   (i >= 1)
//
// 範例:in = {1, 2, 3, 4, 5} → out = {1, 3, 6, 10, 15}
//
// 「前綴和」是演算法面試與實務上極度重要的技巧 — 它把「區間和查詢」從
// O(N) 降到 O(1):sum(i, j) = prefix[j+1] - prefix[i]。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、partial_sum vs adjacent_difference 是「互逆」運算       │
// └────────────────────────────────────────────────────────────┘
//
//   partial_sum:           累積值 → 累進和 (每日銷售 → 累計銷售)
//   adjacent_difference:   累計值 → 增量 (累計確診 → 每日新增)
//
//   adjacent_difference(partial_sum(v)) == v   (反之亦然)
//
// 兩者像「微分」與「積分」的離散版本。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、自訂運算 — 累進積、累進最大                             │
// └────────────────────────────────────────────────────────────┘
//
// 預設用 +。可以傳自訂 op 做其他「累進」:
//
//   * std::multiplies<>{} → 累進積 (1, 2, 6, 24, 120 ...)
//   * 取 max → 「running max」(用於 LC 121 之類的價格序列)
//   * 取 min → 「running min」
//
// 想到「我要邊掃邊維護某個累積狀態」就可以考慮 partial_sum。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、就地計算 (in-place)                                     │
// └────────────────────────────────────────────────────────────┘
//
// d_first 可以等於 first — 直接覆寫原容器:
//
//   std::partial_sum(v.begin(), v.end(), v.begin());
//
// 適合「不需要保留原始值」的場景,省一個容器的記憶體。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、與 inclusive_scan / exclusive_scan (C++17) 的差別       │
// └────────────────────────────────────────────────────────────┘
//
// 「scan 家族」是 C++17 為了「平行運算」而新增的小變體,本檔同時示範它們:
//
//   ┌──────────────────────┬─────────────────────────────────────┐
//   │ partial_sum (C++98)  │ 順序固定 (左到右),不可平行          │
//   │                      │ out[i] = in[0] + ... + in[i]        │
//   ├──────────────────────┼─────────────────────────────────────┤
//   │ inclusive_scan(C++17)│ 結果同 partial_sum,但允許平行重排   │
//   │                      │ out[i] = in[0] + ... + in[i] (+init) │
//   ├──────────────────────┼─────────────────────────────────────┤
//   │ exclusive_scan(C++17)│ 「排除自己」                         │
//   │                      │ out[0] = init                        │
//   │                      │ out[i] = init + in[0] + ... + in[i-1]│
//   └──────────────────────┴─────────────────────────────────────┘
//
// 函式簽章 (C++17,皆於 <numeric>):
//
//   inclusive_scan(first, last, d_first);
//   inclusive_scan(first, last, d_first, BinaryOp);
//   inclusive_scan(first, last, d_first, BinaryOp, init);
//
//   exclusive_scan(first, last, d_first, init);
//   exclusive_scan(first, last, d_first, init, BinaryOp);
//
// 注意:exclusive_scan 必須提供 init,因為 out[0] = init。
// 大資料 + 結合律滿足 (例如加法、min、max) → 用 inclusive_scan 取得平行優勢。
//
// 「為什麼 partial_sum 不能平行?」
//   標準規定 partial_sum 必須以「左到右」嚴格順序求值;若 BinaryOp 不滿足結合律
//   (例如浮點加法雖近似可結合,但精確上不結合),改變順序會改變結果。
//   inclusive_scan / exclusive_scan 則明確「假設」結合律成立,允許實作自由重排,
//   因此可搭配 C++17 平行執行策略 (std::execution::par)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class OutputIt>
//   OutputIt partial_sum(InputIt first, InputIt last, OutputIt d_first);
//
//   template <class InputIt, class OutputIt, class BinaryOp>
//   OutputIt partial_sum(InputIt first, InputIt last, OutputIt d_first,
//                        BinaryOp op);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N - 1 次運算
//   空間: O(1) (除輸出)
//
//   1. d_first 與 first 可重疊 — 就地 OK。
//   2. 大資料想平行 → 用 inclusive_scan。
//   3. 浮點有精度累加問題 — 大數列要小心。
//
// ============================================================

/*
補充筆記：std::partial_sum
  - partial_sum 產生前綴累積序列，常用於 prefix sum。
  - 輸出範圍可以和輸入範圍相同，以原地改成前綴和。
  - 若資料可能溢位，輸出型別與二元運算要事先設計。
  - std::partial_sum 屬於數值演算法，常用來把迴圈中的累加、差分、內積或前綴和寫成標準形式。
  - accumulate 的初始值型別會決定累加結果型別；用 0 累加 double 或 long long 時可能不小心截斷或溢位。
  - reduce 可重新排序運算順序以利平行化，因此 binary operation 必須接近結合律且沒有副作用；浮點數結果可能和 accumulate 不完全相同。
  - partial_sum 和 adjacent_difference 互為常見搭配，可用來建立前綴和與差分陣列。
  - gcd/lcm 對整數工作；lcm 可能溢位，即使最後型別是 long long 也要注意輸入大小。
  - 數值演算法最容易出錯的是型別和溢位，不是語法；工作程式應刻意選擇初始值型別。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::partial_sum
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. partial_sum 做什麼?為什麼前綴和這麼常用?
//     答:輸出與輸入等長的累進和 —— out[0] = in[0]、out[i] = in[0] + ... + in[i]。
//         在 <numeric>,恰好 N-1 次運算,C++20 起 constexpr。價值在於把「區間和查詢」
//         從 O(n) 降到 O(1):sum(i, j) = prefix[j+1] - prefix[i]。
//     追問:可以就地做嗎?(可以,d_first 允許等於 first,直接覆寫原容器)
//
// 🔥 Q2. partial_sum、inclusive_scan、exclusive_scan 三者怎麼選?
//     答:inclusive_scan (C++17) 結果與 partial_sum 相同,但契約上允許重排結合順序,所以
//         只有它有 execution policy 多載可平行;partial_sum 保證嚴格左至右,不可平行。
//         exclusive_scan 則是「排除自己」:out[0] = init、out[i] = init + in[0] + ... + in[i-1],
//         而且 init 是必填參數 —— 因為 out[0] 就是它。
//     追問:為什麼平行框架偏好 exclusive_scan?(算「每個 bucket 的寫入起始 offset」時,
//         需要的正是「我之前的總和、不含我」)
//
// ⚠️ 陷阱. 用 prefix[j+1] - prefix[i] 查區間和時,最常見的錯在哪?
//     答:忘了多留一格。要能表達「空前綴」,prefix 必須是 n+1 長、prefix[0] = 0,並把
//         partial_sum 寫到 prefix.begin() + 1 (本檔 LC303 正是這個寫法)。若直接對 n 長的
//         partial_sum 結果套公式,i = 0 時就會需要 prefix[-1],只能再加分支特判。
//     為什麼會錯:公式背起來了,但沒意識到公式預設的是「長度 n+1、首格為 0」的版面。
//
// Q3. 傳自訂 op 能做出什麼?
//     答:任何「邊掃邊維護累積狀態」的序列:std::multiplies<>{} 得累進積、取 max 得
//         running max (股價題常用)、取 min 得 running min。
// ═══════════════════════════════════════════════════════════════════════════

#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 經典累進和 ---
    std::vector<int> v{1, 2, 3, 4, 5};
    std::vector<int> ps(v.size());
    std::partial_sum(v.begin(), v.end(), ps.begin());
    std::cout << "partial_sum: ";
    for (int x : ps) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 累進積 (用 multiplies) ---
    std::vector<int> w{1, 2, 3, 4, 5};
    std::vector<int> pp(w.size());
    std::partial_sum(w.begin(), w.end(), pp.begin(), std::multiplies<int>{});
    std::cout << "partial_product: ";
    for (int x : pp) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 累進最大 (running max) ---
    std::vector<int> u{3, 1, 4, 1, 5, 9, 2, 6};
    std::vector<int> pm(u.size());
    std::partial_sum(u.begin(), u.end(), pm.begin(),
                     [](int a, int b){ return std::max(a, b); });
    std::cout << "running max: ";
    for (int x : pm) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 4: 就地計算 (in-place) ---
    std::vector<int> z{10, 20, 30};
    std::partial_sum(z.begin(), z.end(), z.begin());
    std::cout << "in-place: ";
    for (int x : z) std::cout << x << ' ';
    std::cout << '\n';

    // ============================================================
    // === scan 家族 (C++17): inclusive_scan / exclusive_scan ===
    // ============================================================

    // --- 範例 5: inclusive_scan (預設加法,結果同 partial_sum) ---
    //   out[i] = in[0] + in[1] + ... + in[i]
    std::vector<int> a{1, 2, 3, 4, 5};
    std::vector<int> is_out(a.size());
    std::inclusive_scan(a.begin(), a.end(), is_out.begin());
    std::cout << "inclusive_scan: ";
    for (int x : is_out) std::cout << x << ' ';
    std::cout << '\n';
    // 與 partial_sum 結果一致,但允許平行重排。

    // --- 範例 6: inclusive_scan + 自訂 init (起始累計值) ---
    //   out[i] = init + in[0] + ... + in[i]
    std::vector<int> is_out2(a.size());
    std::inclusive_scan(a.begin(), a.end(), is_out2.begin(),
                        std::plus<int>{}, /*init=*/100);
    std::cout << "inclusive_scan(init=100): ";
    for (int x : is_out2) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 7: exclusive_scan (排除自己) ---
    //   out[0] = init
    //   out[i] = init + in[0] + ... + in[i-1]
    //
    // 這是「跑步前的距離」概念:第 i 站之前已累積多少,不含第 i 站本身。
    // 也是 GPU/平行框架最常用的「prefix sum」形式。
    std::vector<int> es_out(a.size());
    std::exclusive_scan(a.begin(), a.end(), es_out.begin(), /*init=*/0);
    std::cout << "exclusive_scan(init=0): ";
    for (int x : es_out) std::cout << x << ' ';
    std::cout << '\n';
    // 對 {1,2,3,4,5} 結果為 {0, 1, 3, 6, 10} — 比 partial_sum 整個右移一格。

    // --- 範例 8: exclusive_scan + 自訂 op (累進積) ---
    std::vector<int> b{1, 2, 3, 4, 5};
    std::vector<int> es_prod(b.size());
    std::exclusive_scan(b.begin(), b.end(), es_prod.begin(),
                        /*init=*/1, std::multiplies<int>{});
    std::cout << "exclusive_scan product(init=1): ";
    for (int x : es_prod) std::cout << x << ' ';
    std::cout << '\n';
    // {1, 2, 3, 4, 5} → {1, 1, 2, 6, 24} — 「右側元素的左累積積」常用於組合學。

    // === LeetCode / 實務範例 ===
    void leetcode_1480_running_sum();
    void leetcode_303_range_sum_query();
    void practical_cumulative_revenue();
    void practical_exclusive_scan_offsets();
    void leetcode_724_pivot_index_prefix();
    void practical_cumulative_uptime();
    leetcode_1480_running_sum();
    leetcode_303_range_sum_query();
    practical_cumulative_revenue();
    practical_exclusive_scan_offsets();
    leetcode_724_pivot_index_prefix();
    practical_cumulative_uptime();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1480: 一維陣列的動態和 (Running Sum of 1d Array)
// ----------------------------------------------------------------
// 題目:給陣列 nums,回傳「runningSum[i] = sum(nums[0..i])」。
//
// 為什麼用 std::partial_sum:
//   題意 100% 對應 — partial_sum 就是輸出與輸入等長的「累進和」。
//   可以就地完成,空間 O(1)。
//
// 複雜度:時間 O(n);空間 O(1) (就地)。
void leetcode_1480_running_sum() {
    std::vector<int> nums{1, 2, 3, 4};
    std::partial_sum(nums.begin(), nums.end(), nums.begin());
    std::cout << "LC1480: ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 303: 區域和檢索 (Range Sum Query - Immutable)
// ----------------------------------------------------------------
// 題目:設計類別 NumArray,支援 sumRange(i, j) 回傳 nums[i..j] 之和。
//
// 為什麼用 std::partial_sum:
//   * 預先計算「前綴和」prefix (O(n))。
//   * 之後 sumRange(i, j) = prefix[j+1] - prefix[i],O(1) per query。
//   * 多次查詢的場景下從 O(n) 降到 O(1) — 經典最佳化。
//
// 複雜度:建構 O(n);查詢 O(1);空間 O(n)。
void leetcode_303_range_sum_query() {
    std::vector<int> nums{-2, 0, 3, -5, 2, -1};
    std::vector<int> prefix(nums.size() + 1, 0);
    std::partial_sum(nums.begin(), nums.end(), prefix.begin() + 1);
    auto sumRange = [&](int i, int j){ return prefix[j + 1] - prefix[i]; };
    std::cout << "LC303 sumRange(0,2)=" << sumRange(0, 2)
              << " sumRange(2,5)="     << sumRange(2, 5)
              << " sumRange(0,5)="     << sumRange(0, 5)
              << '\n';
}

// ----------------------------------------------------------------
// 實務範例:累計營收趨勢 (Cumulative Revenue)
// ----------------------------------------------------------------
// 場景:給「每日營收」daily,要繪製「截至當日的累計營收」折線圖。
//
// 為什麼用 std::partial_sum:
//   累計就是 prefix sum,語意自然且程式碼一行搞定。
void practical_cumulative_revenue() {
    std::vector<double> daily{120.0, 80.5, 200.0, 50.0, 300.0};
    std::vector<double> cum(daily.size());
    std::partial_sum(daily.begin(), daily.end(), cum.begin());
    std::cout << "cumulative revenue: ";
    for (double x : cum) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:exclusive_scan 計算「寫入位置 offset」
// ----------------------------------------------------------------
// 場景:有一份「每組要寫入的元素數量」counts,要求出每組「寫入起始位置」。
//   counts  = {3, 1, 0, 4, 2}
//   offsets = {0, 3, 4, 4, 8}     ← 第 i 組寫入起始 = 前 i-1 組總和
//
// 為什麼用 std::exclusive_scan:
//   * 這是 GPU / 平行框架打包資料時最經典的需求 — 「我要把可變長度的
//     buckets 平鋪到一個連續陣列,我得先知道每個 bucket 的起始位置」。
//   * exclusive_scan 一行就解,語意正好對應(「我之前的總和,不含我」)。
//
// 複雜度:時間 O(n);空間 O(n)。
void practical_exclusive_scan_offsets() {
    std::vector<int> counts{3, 1, 0, 4, 2};
    std::vector<int> offsets(counts.size());
    std::exclusive_scan(counts.begin(), counts.end(), offsets.begin(), 0);
    std::cout << "offsets: ";
    for (int x : offsets) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 724: 找到陣列的中心索引 (Find Pivot Index)
// ----------------------------------------------------------------
// 題目:找出 i,使得 sum(nums[0..i-1]) == sum(nums[i+1..n-1])。不存在 → -1。
//
// 為什麼用 std::partial_sum:
//   先算出整個 prefix sum,對每個 i,左和 = prefix[i],右和 = total - prefix[i+1]。
//   平均 O(1) per query。
//
// 複雜度:時間 O(n);空間 O(n)。
void leetcode_724_pivot_index_prefix() {
    std::vector<int> nums{1, 7, 3, 6, 5, 6};
    std::vector<int> prefix(nums.size() + 1, 0);
    std::partial_sum(nums.begin(), nums.end(), prefix.begin() + 1);
    int total = prefix.back(), ans = -1;
    for (size_t i = 0; i < nums.size(); ++i) {
        int left = prefix[i];
        int right = total - prefix[i + 1];
        if (left == right) { ans = i; break; }
    }
    std::cout << "LC724: " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例:伺服器累積運轉時數 (Cumulative Uptime)
// ----------------------------------------------------------------
// 場景:每月維運記錄當月運轉時數,儀表板要顯示「截至本月的累積時數」。
//      partial_sum 直接生成累計序列。
void practical_cumulative_uptime() {
    std::vector<int> monthly{720, 730, 720, 700, 740};   // hours
    std::vector<int> total(monthly.size());
    std::partial_sum(monthly.begin(), monthly.end(), total.begin());
    std::cout << "cumulative uptime:";
    for (int x : total) std::cout << ' ' << x;
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra partial_sum.cpp -o partial_sum

// === 預期輸出 ===
// partial_sum: 1 3 6 10 15
// partial_product: 1 2 6 24 120
// running max: 3 3 4 4 5 9 9 9
// in-place: 10 30 60
// inclusive_scan: 1 3 6 10 15
// inclusive_scan(init=100): 101 103 106 110 115
// exclusive_scan(init=0): 0 1 3 6 10
// exclusive_scan product(init=1): 1 1 2 6 24
// LC1480: 1 3 6 10
// LC303 sumRange(0,2)=1 sumRange(2,5)=-1 sumRange(0,5)=-3
// cumulative revenue: 120 200.5 400.5 450.5 750.5
// offsets: 0 3 4 4 8
// LC724: 3
// cumulative uptime: 720 1450 2170 2870 3610
