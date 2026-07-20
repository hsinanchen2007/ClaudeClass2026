// ============================================================
// std::nth_element
// 分類 (Category): Sorting operations (排序演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/nth_element
//   * https://cplusplus.com/reference/algorithm/nth_element/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 — 「部分排序」中的最快選擇                     │
// └────────────────────────────────────────────────────────────┘
//
// std::nth_element 解的問題:
//
//   「我只要『第 n 名』那個元素是『正確的』 — 其他位置不用排好。」
//
// 換句話說,呼叫後保證:
//
//   * nums[n] 是「若整段有序時應該出現在 nums[n] 的元素」
//   * [first, n) 全部 <= nums[n]   (前段都比 nums[n] 小或相等)
//   * (n, last) 全部 >= nums[n]    (後段都比 nums[n] 大或相等)
//
// 但!兩段內部的順序「不保證」!不要假設 [first, n) 是排好的。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼這麼快? (introselect)                           │
// └────────────────────────────────────────────────────────────┘
//
// std::nth_element 一般用「introselect」實作 (quickselect 的強化版):
//
//   1. 用 quickselect 平均 O(N) 找出 nth 位置應該的元素。
//   2. 若遞迴深度過深,fallback 到 median-of-medians 保證最壞 O(N)。
//
// 對比 std::sort 的 O(N log N) — nth_element 真的就是 O(N),
// 對「找第 K 大/小、找中位數」這類問題效能差距很大。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、典型應用情境                                           │
// └────────────────────────────────────────────────────────────┘
//
//   * Quickselect:找第 K 大 / 第 K 小元素 (LC 215 等)。
//   * 找中位數:nth = N/2 即可,O(N) 完成。
//   * Top-K (不需要排序內容):前 K 個元素是最小的,但內部順序不重要。
//   * 統計上的 percentile (P50 / P90 / P99) 計算。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、與 partial_sort / sort 的比較                          │
// └────────────────────────────────────────────────────────────┘
//
//   ┌────────────────┬─────────┬───────────────────────────────┐
//   │ 函式           │ 複雜度  │ 結果保證                       │
//   ├────────────────┼─────────┼───────────────────────────────┤
//   │ sort           │ O(N log N) │ 整段排序                    │
//   │ partial_sort   │ O(N log K) │ 前 K 個排序,後段亂          │
//   │ nth_element    │ O(N)    │ 第 K 個位置正確,前後段亂       │
//   └────────────────┴─────────┴───────────────────────────────┘
//
// 「我要前 K 個,且要排序」 → partial_sort
// 「我只要第 K 個 (或 < / >= K 的兩堆)」 → nth_element
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt>
//   void nth_element(RandomIt first, RandomIt nth, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   void nth_element(RandomIt first, RandomIt nth, RandomIt last,
//                    Compare comp);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N) 平均;C++11 後標準提升為最壞 O(N) 也保證
//   空間: O(log N) (遞迴堆疊)
//   需求: RandomAccessIterator;nth ∈ [first, last]
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 兩段內部順序「未指定」!不要當成 partial_sort 用。
//   2. nth == last 表示「不指定任何位置」,什麼都不做。
//   3. 找「第 k 大」: 用 nth_element + greater<>,或用 begin + (n-k) 等價。
//   4. 偶數個元素求中位數需要呼叫兩次 (上中位數 + 下中位數的 max)。
//
// ============================================================

/*
補充筆記：std::nth_element
  - nth_element 只保證第 n 個元素在排序後應在的位置，不保證左右兩側各自有序。
  - 它很適合找 median、top K 門檻或第 K 小值。
  - 如果前 K 個還需要排序，nth_element 後仍要對前 K 段 sort。
  - std::nth_element 屬於排序與選取工具；先分清楚你需要完整排序、前 K 小、還是只要第 N 個元素就定位。
  - sort 不穩定，stable_sort 保留等價元素的原相對順序；資料有次排序鍵時穩定性很重要。
  - nth_element 只保證第 n 個元素就位，左邊不大於它、右邊不小於它，但左右兩邊各自不排序。
  - partial_sort 適合需要前 K 個已排序結果；若 K 遠小於 N，通常比完整 sort 更合適。
  - 比較器必須是 strict weak ordering，不能用 <= 當 less；這是排序初學者常犯的錯。
  - 排序會移動元素，保存的 iterator、pointer、reference 是否有效取決於容器和操作；排序 vector 時元素值位置會改變。
  - std::nth_element 的比較器若把相等元素也判成 true，例如使用 <=，會破壞排序所需的 strict weak ordering。
  - std::nth_element 呼叫後元素位置可能改變；若其他資料結構保存索引或指向元素的 iterator，要重新檢查關聯是否仍正確。
  - std::nth_element 的選擇要看需求：完整排序用 sort/stable_sort，只要第 N 名用 nth_element，只要前 K 名用 partial_sort。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::nth_element
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::nth_element 是做什麼的?複雜度?
//     答:部分排序 — 重排區間,使第 n 個位置放上「若整段完整排序後該位置應有
//         的元素」,且左邊全部 <= 它、右邊全部 >= 它,但左右兩側內部不保證有序。
//         標準規定平均複雜度為 linear O(n)。經典用途:第 k 大 / 第 k 小、
//         中位數、Top-K,比完整排序的 O(n log n) 快。
//         ⚠️ 典型實作是 introselect (quickselect 加上深度保護),但這是
//            implementation-defined;標準只規定「平均線性」,未規定最壞複雜度。
//     追問:nth_element 穩定嗎?(不穩定,和 std::sort 一樣靠 swap 搬移元素)
//
// 🔥 Q2. 求 Top-K 該用 nth_element 還是 partial_sort?
//     答:看「那 K 個之間需不需要有序」。只要知道「是哪 K 個」→ nth_element,
//         平均 O(n);還要求這 K 個彼此也排好 → partial_sort,約 O(n log k)。
//         全部都要有序才用 sort,O(n log n)。求中位數只需 nth_element。
//     追問:k 很接近 n 時呢?
//           (此時部分排序失去優勢,直接 sort 即可)
//
// ⚠️ 陷阱. 呼叫後可以把 [first, nth) 當成「已排好的前 n 名」直接用嗎?
//     答:不行。標準只保證分界性質 (左邊 <= *nth <= 右邊) 以及 *nth 就位,
//         兩側內部的順序是未指定的。需要有序的前 K 名必須改用 partial_sort。
//     為什麼會錯:很多人把 nth_element 想成「排序做到一半」,以為左半部至少
//         大致有序;實際上 quickselect 只做 partition,從未排序左半部。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 找第 4 小 (index 3) ---
    std::vector<int> v{7, 2, 9, 1, 5, 8, 3, 6, 4};
    std::nth_element(v.begin(), v.begin() + 3, v.end());
    std::cout << "4th smallest = " << v[3] << '\n';
    std::cout << "left part (all <= 4th): ";
    for (size_t i = 0; i < 3; ++i) std::cout << v[i] << ' ';
    std::cout << '\n';
    std::cout << "right part (all >= 4th): ";
    for (size_t i = 4; i < v.size(); ++i) std::cout << v[i] << ' ';
    std::cout << '\n';

    // --- 範例 2: 找中位數 (奇數個) ---
    std::vector<int> w{5, 1, 9, 3, 7, 2, 8, 4, 6};
    auto mid = w.begin() + w.size() / 2;
    std::nth_element(w.begin(), mid, w.end());
    std::cout << "median = " << *mid << '\n';

    // --- 範例 3: 第 3 大 (用 greater) ---
    std::vector<int> u{7, 2, 9, 1, 5, 8, 3, 6, 4};
    std::nth_element(u.begin(), u.begin() + 2, u.end(), std::greater<int>{});
    std::cout << "3rd largest = " << u[2] << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_215_kth_largest();
    void leetcode_973_k_closest_points();
    void practical_median();
    void leetcode_1985_kth_largest_in_strings();
    void practical_percentile_calc();
    leetcode_215_kth_largest();
    leetcode_973_k_closest_points();
    practical_median();
    leetcode_1985_kth_largest_in_strings();
    practical_percentile_calc();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 215: 陣列中的第 K 大元素 (Kth Largest Element in an Array)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums 與整數 k,回傳第 k 大的元素 (依排序後計算)。
//
// 為什麼用 std::nth_element:
//   題目「只要第 k 大那一個」 — 不需排序所有元素 — O(N) 平均的
//   nth_element 是最佳工具。
//
// 解法步驟:
//   1. 把 (k-1) (從 0 起算) 位置丟給 nth_element,用 greater<> 讓「大者在前」。
//   2. nums[k-1] 即為答案。
//
// 複雜度:時間 O(N) 平均 / O(N) 最壞 (現代 introselect);空間 O(1)。
void leetcode_215_kth_largest() {
    std::vector<int> nums{3, 2, 1, 5, 6, 4};
    int k = 2;
    std::nth_element(nums.begin(), nums.begin() + (k - 1), nums.end(),
                     std::greater<int>{});
    std::cout << "LC215: " << nums[k - 1] << '\n';
}

// ----------------------------------------------------------------
// LeetCode 973: 最接近原點的 K 個點 (K Closest Points to Origin)
// ----------------------------------------------------------------
// 題目:給一組 (x, y) 點,回傳距離原點最近的 k 個點。
//
// 為什麼用 std::nth_element:
//   題目要「前 k 個」(順序不要求) — quickselect 把它們拉到前 k 個位置即可,
//   O(N) 平均;比 sort + 取前 k (O(N log N)) 快。
//   小技巧:用「平方距離」比較,不必開根號,精度與效率都好。
//
// 複雜度:時間 O(N) 平均;空間 O(1) (就地)。
void leetcode_973_k_closest_points() {
    std::vector<std::pair<int,int>> pts{{1,3},{-2,2},{5,8},{0,1}};
    int k = 2;
    auto dist2 = [](const std::pair<int,int>& p){
        return p.first*p.first + p.second*p.second;
    };
    std::nth_element(pts.begin(), pts.begin() + k, pts.end(),
                     [&](const auto& a, const auto& b){ return dist2(a) < dist2(b); });
    // 為了輸出穩定,將前 k 個依距離排序顯示
    std::sort(pts.begin(), pts.begin() + k,
              [&](const auto& a, const auto& b){ return dist2(a) < dist2(b); });
    std::cout << "LC973:";
    for (int i = 0; i < k; ++i)
        std::cout << " (" << pts[i].first << "," << pts[i].second << ")";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:中位數計算
// ----------------------------------------------------------------
// 場景:統計分析常需快速求中位數。對 N 個樣本,nth_element 比完整排序快。
//
// 為什麼用 std::nth_element:
//   只要 nth = N/2 那個位置「對」即可;前後段隨便。O(N) 完成。
void practical_median() {
    std::vector<int> data{8, 3, 6, 1, 4, 9, 2, 7, 5};
    auto mid = data.begin() + data.size() / 2;
    std::nth_element(data.begin(), mid, data.end());
    std::cout << "median = " << *mid << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1985: 找出陣列中第 K 大的整數 (字串版,nth_element 解法)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給字串陣列 nums (每個字串表示一個非負整數),回傳第 k 大字串 (依數值)。
//      簡化:當所有值能放進 long long 時,直接用 nth_element。
//
// 為什麼用 std::nth_element:
//   只要「第 k 大那一個」即可,O(N) 平均比完整 sort 快。
//   注意用 greater<> 讓「大者」在前,nth 位置 (k-1) 即答案。
//
// 複雜度:時間 O(N) 平均;空間 O(N) (字串轉 long long)。
void leetcode_1985_kth_largest_in_strings() {
    std::vector<std::string> nums{"3", "6", "7", "10"};
    int k = 4;
    std::vector<long long> v;
    for (auto& s : nums) v.push_back(std::stoll(s));
    std::nth_element(v.begin(), v.begin() + (k - 1), v.end(),
                     std::greater<long long>{});
    std::cout << "LC1985: " << v[k - 1] << '\n';
}

// ----------------------------------------------------------------
// 實務範例:百分位數計算 (例如 P95 延遲)
// ----------------------------------------------------------------
// 場景:監控系統要算 API 延遲的 P95;
//      nth_element 把第 95% 位置的元素「定位」,前面都 <= 它。O(N) 平均。
void practical_percentile_calc() {
    std::vector<int> latency{20, 30, 25, 18, 100, 40, 35, 22, 28, 33};
    int n = latency.size();
    int p95 = (int)(n * 0.95);   // 第 95% 位置
    std::nth_element(latency.begin(), latency.begin() + p95, latency.end());
    std::cout << "P95 latency: " << latency[p95] << " ms\n";
}

// === 預期輸出 (Expected output) ===
// 4th smallest = 4
// left part (all <= 4th): ?? ?? ??     (內容皆 <= 4,順序依實作)
// right part (all >= 4th): ?? ?? ?? ?? ??
// median = 5
// 3rd largest = 7
// LC215: 5
// LC973: (0,1) (-2,2)
// median = 5
// LC1985: 3
// P95 latency: 100 ms
