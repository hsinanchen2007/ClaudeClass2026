// ============================================================
// std::upper_bound
// 分類 (Category): Binary search operations (二分搜尋,需已排序)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/upper_bound
//   * https://cplusplus.com/reference/algorithm/upper_bound/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::upper_bound 解的問題:
//
//   「在已排序的範圍中,找『第一個嚴格大於 value』的位置。」
//
// 即 *it > value 的最小 it。注意是「>」(嚴格大於),不是「>=」 —
// 這跟 lower_bound 的差別只在這一個字。
//
// 範例:在 {1, 3, 3, 5, 7, 9} 中:
//
//   upper_bound(3) → 指向 5    (跳過所有 3,第一個 > 3)
//   upper_bound(4) → 指向 5    (第一個 > 4)
//   upper_bound(0) → 指向 1   (第一個 > 0)
//   upper_bound(99) → end()   (沒有任何 > 99)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、與 lower_bound 配合 — 找「等於 value」的範圍            │
// └────────────────────────────────────────────────────────────┘
//
// upper_bound 通常和 lower_bound 一起出現,組成「等價區段」:
//
//   範圍 [lower_bound(v), upper_bound(v)) = 所有等於 v 的元素
//   個數 = upper_bound(v) - lower_bound(v)
//
// 對「multiset 計數」「直方圖計算」「在已排序時間戳中找區段」這類問題
// 是核心工具。一次性取兩個位置的話,直接用 std::equal_range 更快。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、用 upper_bound 做「找最後一個 == value 的位置」         │
// └────────────────────────────────────────────────────────────┘
//
//   auto hi = std::upper_bound(v.begin(), v.end(), value);
//   auto last = (hi == v.begin()) ? v.end()  // 沒任何 <= value
//                                 : hi - 1;  // 最後一個 == value (若存在)
//
// 但你還需要驗證 *last == value,因為若 value 不存在,last 會指向「最後一個 < value」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class T>
//   FwdIt upper_bound(FwdIt first, FwdIt last, const T& value);
//
//   template <class FwdIt, class T, class Compare>
//   FwdIt upper_bound(FwdIt first, FwdIt last, const T& value, Compare comp);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(log N) 比較
//   空間: O(1)
//   需求: 範圍依 comp 已排序;否則 UB
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「上界」是「第一個 > value」,不是「最後一個 == value」!易混淆。
//   2. 範圍必須已排序。
//   3. comp 必須與排序時相同。
//   4. 一次取 lower + upper 改用 std::equal_range,只 traverse 一次更快。
//
// ============================================================

/*
補充筆記：std::upper_bound
  - upper_bound 回傳第一個「大於 value」的位置；它常和 lower_bound 配合計算重複元素數量。
  - 若 value 出現多次，upper_bound 指向最後一個 value 的下一格，不是任一個 value 的位置。
  - 用 upper_bound 做插入時，新元素會放在既有相等元素之後，可維持相等元素的原先群組順序。
  - std::upper_bound 屬於二分搜尋家族，前提是輸入範圍已依同一個比較規則排序；未排序資料上使用結果沒有意義。
  - 這類函式通常回傳位置或範圍，不一定回傳 bool；拿到 iterator 後要先檢查是否等於 end()。
  - lower_bound 找第一個不小於 value 的位置，upper_bound 找第一個大於 value 的位置，兩者相減可得到重複值數量。
  - binary_search 只回答是否存在；若後續需要插入位置、索引或重複區間，lower_bound/equal_range 更實用。
  - 比較器必須和排序時使用的規則一致；用不同規則搜尋同一批資料，結果會像資料沒排序一樣不可信。
  - 在 vector 上 iterator 相減可得到索引，在 list 上不行；iterator category 會影響你能不能做 O(1) 距離計算。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::upper_bound
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. upper_bound 和 lower_bound 差在哪?兩者怎麼合用?
//     答:lower_bound 回傳第一個 >= value 的位置,upper_bound 回傳第一個
//         > value(嚴格大於)的位置 — 差別只在 value 本身算不算進去。
//         合用時 [lower_bound(v), upper_bound(v)) 就是所有等於 v 的元素,
//         個數 = upper - lower;若兩者相等即代表 v 不存在。
//         若兩個邊界都要,直接用 equal_range 語意更清楚。
//     追問:那「插入」該用哪一個?
//           (答:要插在既有相等元素之前用 lower_bound,插在其後用 upper_bound;
//            後者能維持先來後到的群組順序)
//
// 🔥 Q2. 複雜度?對非 random access iterator 呢?
//     答:比較次數是 O(log N),但總時間只有在 random access iterator 上
//         才是 O(log N)。對 std::list 這類 bidirectional iterator,跳中點得
//         逐步 ++,iterator 前進總步數是 O(N),整體退化為 O(N)。
//         最低需求是 LegacyForwardIterator;範圍必須依同一 comp 已排序,否則 UB。
//     追問:對 std::multiset 呢?
//           (答:改用成員 multiset::upper_bound(),沿樹下降才是真正的 O(log N))
//
// ⚠️ 陷阱. upper_bound(v) 回傳的是「最後一個等於 v 的位置」嗎?
//     答:不是。它回傳的是「第一個大於 v」的位置,也就是最後一個 v 的
//         再下一格 — 是半開區間的右端,可能等於 last。想拿「最後一個等於 v」
//         必須先確認 hi != begin 才能 --hi,而且還要驗證 *(hi-1) == v,
//         因為 v 不存在時 hi-1 指到的是「最後一個小於 v 的元素」。
//     為什麼會錯:「lower/upper」的字面像是「下界/上界」這組對稱詞,直覺會
//         腦補成「第一個 v / 最後一個 v」。實際上兩者回傳的都是「邊界位置」,
//         且都遵守 STL 半開區間 [first, last) 的慣例 — 右端一律是「超過尾端」。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 3, 3, 3, 5, 7, 9};

    // --- 範例 1: upper_bound(3) → 指向 5 (第一個 > 3) ---
    auto it = std::upper_bound(v.begin(), v.end(), 3);
    std::cout << "upper_bound(3) at index " << (it - v.begin())
              << ", val=" << *it << '\n';

    // --- 範例 2: 計算 3 出現的次數 (lower 與 upper 的差) ---
    auto lo = std::lower_bound(v.begin(), v.end(), 3);
    auto hi = std::upper_bound(v.begin(), v.end(), 3);
    std::cout << "count(3) via bounds = " << (hi - lo) << '\n';

    // --- 範例 3: upper_bound(5) → 指向 7 ---
    auto it2 = std::upper_bound(v.begin(), v.end(), 5);
    std::cout << "upper_bound(5) val=" << *it2 << '\n';

    // --- 範例 4: upper_bound(0) → first (因為 first > 0 立刻成立) ---
    auto it3 = std::upper_bound(v.begin(), v.end(), 0);
    std::cout << "upper_bound(0) at index " << (it3 - v.begin()) << '\n';

    // --- 範例 5: upper_bound(99) → last (沒有元素 > 99) ---
    auto it4 = std::upper_bound(v.begin(), v.end(), 99);
    std::cout << "upper_bound(99) is end? "
              << std::boolalpha << (it4 == v.end()) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_34_search_range();
    void leetcode_2300_pairs_of_potions();
    void practical_count_in_range();
    void leetcode_1283_smallest_divisor();
    void practical_histogram_bucketing();
    leetcode_34_search_range();
    leetcode_2300_pairs_of_potions();
    practical_count_in_range();
    leetcode_1283_smallest_divisor();
    practical_histogram_bucketing();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 34 (後半):在排序陣列中查詢元素的最後一個位置
// ----------------------------------------------------------------
// 題目:給已排序陣列 nums 與 target,回傳 [first, last]。不存在 → [-1, -1]。
//
// 為什麼用 std::upper_bound + std::lower_bound:
//   lower_bound 取「第一個 == target」,upper_bound - 1 取「最後一個 == target」。
//   若 lower == upper → 區段為空 → target 不存在。
//
// 複雜度:時間 O(log n);空間 O(1)。
void leetcode_34_search_range() {
    std::vector<int> nums{5, 7, 7, 8, 8, 10};
    int target = 8;
    auto lo = std::lower_bound(nums.begin(), nums.end(), target);
    auto hi = std::upper_bound(nums.begin(), nums.end(), target);
    int first = -1, last = -1;
    if (lo != hi) {
        first = static_cast<int>(lo - nums.begin());
        last  = static_cast<int>(hi - nums.begin()) - 1;
    }
    std::cout << "LC34: [" << first << ',' << last << "]\n";

    // 邊界:不存在
    int t2 = 6;
    auto lo2 = std::lower_bound(nums.begin(), nums.end(), t2);
    auto hi2 = std::upper_bound(nums.begin(), nums.end(), t2);
    int f2 = -1, l2 = -1;
    if (lo2 != hi2) {
        f2 = static_cast<int>(lo2 - nums.begin());
        l2 = static_cast<int>(hi2 - nums.begin()) - 1;
    }
    std::cout << "LC34(miss): [" << f2 << ',' << l2 << "]\n";
}

// ----------------------------------------------------------------
// LeetCode 2300: 咒語和藥水的成功對數
//                (Successful Pairs of Spells and Potions) — 簡化版
// ----------------------------------------------------------------
// 題目簡化:給已排序的藥水力量 potions 與一個 spell 的「最低需求乘積 success」,
//          求有幾個 potion 滿足 spell × potion >= success。
//
// 為什麼用 std::lower_bound (對應「>= 門檻的個數」):
//   * 排序後,只要找「第一個 >= 門檻」的位置 lo;
//   * 從 lo 到 end 的個數就是答案 (= end - lo)。
//
// 複雜度:時間 O(log m) per spell;空間 O(1)。
void leetcode_2300_pairs_of_potions() {
    std::vector<int> potions{1, 2, 3, 4, 5};   // 已排序
    long long success = 7;
    int spell = 3;
    long long need_potion = (success + spell - 1) / spell;   // 向上取整
    auto lo = std::lower_bound(potions.begin(), potions.end(), need_potion);
    int pairs = static_cast<int>(potions.end() - lo);
    std::cout << "LC2300: " << pairs << '\n';
}

// ----------------------------------------------------------------
// 實務範例:統計「區間 [a, b] 內」的元素個數
// ----------------------------------------------------------------
// 場景:已排序的測量值 / 時戳陣列,要查詢某區間內有多少筆資料。
//      count = upper_bound(b) - lower_bound(a)。
void practical_count_in_range() {
    std::vector<int> data{1, 3, 3, 5, 7, 9, 9, 11, 13};
    int a = 3, b = 9;
    auto lo = std::lower_bound(data.begin(), data.end(), a);
    auto hi = std::upper_bound(data.begin(), data.end(), b);
    std::cout << "count in [" << a << ',' << b << "] = "
              << (hi - lo) << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1283: 使結果不超過閾值的最小除數 (Find the Smallest Divisor)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給陣列 nums 與閾值 threshold,找最小的整數 d,使得
//      sum(ceil(nums[i]/d)) <= threshold。
//
// 為什麼用「二分答案」 + upper_bound 思考方式:
//   d 越大 → sum 越小 (單調) → 把答案空間視為已排序;
//   要找的就是「第一個讓 sum(d) <= threshold 成立的 d」。
//   雖然這裡用手動 while 二分而不是 std::upper_bound (因為 predicate 不是
//   值比較),概念完全等同 upper/lower_bound 的「第一個讓條件成立的位置」。
//
// 複雜度:時間 O(n log(max));空間 O(1)。
void leetcode_1283_smallest_divisor() {
    std::vector<int> nums{1, 2, 5, 9};
    int threshold = 6;
    auto sum_div = [&](int d) {
        int s = 0;
        for (int x : nums) s += (x + d - 1) / d;   // ceil
        return s;
    };
    int lo = 1, hi = *std::max_element(nums.begin(), nums.end());
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (sum_div(mid) <= threshold) hi = mid;
        else lo = mid + 1;
    }
    std::cout << "LC1283: " << lo << '\n';
}

// ----------------------------------------------------------------
// 實務範例:直方圖分桶 (Histogram Bucketing)
// ----------------------------------------------------------------
// 場景:量測一批數據 (例如 API 延遲),要分到 bucket 邊界
//      [0, 50, 100, 200, 500, 1000, ∞) ms;對每筆數值用 upper_bound
//      找它「屬於哪個 bucket」(即 < 該邊界的最後一個 bucket)。
//
// 為什麼用 std::upper_bound:
//   upper_bound(x) 回傳「第一個 > x 的位置」,正好對應 x 所屬 bucket 的右邊界,
//   bucket index = 位置 - 1 (避免 0 邊界的特例)。
void practical_histogram_bucketing() {
    std::vector<int> edges{50, 100, 200, 500, 1000};
    std::vector<int> samples{30, 100, 150, 700, 5000};
    for (int x : samples) {
        auto it = std::upper_bound(edges.begin(), edges.end(), x);
        int bucket = static_cast<int>(it - edges.begin());
        std::cout << "x=" << x << " bucket=" << bucket << '\n';
    }
}

// 編譯: g++ -std=c++20 -Wall -Wextra upper_bound.cpp -o upper_bound

// === 預期輸出 ===
// upper_bound(3) at index 4, val=5
// count(3) via bounds = 3
// upper_bound(5) val=7
// upper_bound(0) at index 0
// upper_bound(99) is end? true
// LC34: [3,4]
// LC34(miss): [-1,-1]
// LC2300: 3
// count in [3,9] = 6
// LC1283: 5
// x=30 bucket=0
// x=100 bucket=2
// x=150 bucket=2
// x=700 bucket=4
// x=5000 bucket=5
