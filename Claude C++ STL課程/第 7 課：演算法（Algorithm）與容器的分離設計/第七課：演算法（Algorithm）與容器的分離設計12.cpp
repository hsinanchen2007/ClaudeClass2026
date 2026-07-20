// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計12.cpp
//    —  部分排序：partial_sort / nth_element（只做剛好夠用的工作）
// =============================================================================
//
// 【主題資訊 Information】
//   void partial_sort(RandomIt f, RandomIt middle, RandomIt l);              // C++98
//   void partial_sort(RandomIt f, RandomIt middle, RandomIt l, Compare c);   // C++98
//   void nth_element (RandomIt f, RandomIt nth,    RandomIt l);              // C++98
//   void nth_element (RandomIt f, RandomIt nth,    RandomIt l, Compare c);   // C++98
//
//   標準版本：C++98 起（C++17 加執行策略、C++20 加 constexpr）
//   迭代器需求：**Random Access Iterator**（同 sort，list 不能用）
//   複雜度：partial_sort 約 O(N log M)（M = middle - first）；
//           nth_element **平均 O(N) 線性**，最壞情況標準未明定上界
//   標頭檔：<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. 核心觀念：不要做多餘的工作】
// 「從一萬筆資料中找出最小的 10 筆」，多數人的直覺是：
//     std::sort(v.begin(), v.end());        // O(N log N)，排好全部一萬筆
//     取前 10 筆
// 但你只需要 10 筆，卻替另外 9,990 筆排好了序——這些工作全是浪費。
// STL 提供了三個層級的工具，讓你只做剛好夠用的工作：
//     std::sort         全部排好          O(N log N)
//     std::partial_sort 前 M 個排好       O(N log M)   ← M 很小時明顯更快
//     std::nth_element  只保證第 n 個正確  **平均 O(N)** ← 最快
// 選擇準則：
//   * 要前 K 名且**要照順序**      → partial_sort
//   * 只要第 K 名的**值**（如中位數）→ nth_element
//   * 要前 K 名但**不在乎順序**    → nth_element（之後那 K 個就是答案，順序不定）
//
// 【2. nth_element 為什麼能做到平均線性時間】
// 它用的是 quickselect（快速選擇）演算法，本質是「只遞迴一邊的 quicksort」：
//   選一個 pivot 分割 → 看目標位置 n 落在左半還是右半 → **只往那一邊繼續遞迴**
// quicksort 兩邊都要處理，所以是 T(N) = 2T(N/2) + N = O(N log N)；
// quickselect 只處理一邊，T(N) = T(N/2) + N = **O(N)**（等比級數收斂到 2N）。
// 代價是：它只保證第 n 個位置正確，其他位置**只保證分邊、不保證有序**。
//
// 【3. 「未指定順序」的精確意義（本檔輸出必讀）】
// 這兩個演算法都會留下「順序未指定」的區域，標準的保證如下：
//   partial_sort(first, middle, last)：
//     * [first, middle) → **已排序**，且是整個範圍中最小的 M 個
//     * [middle, last)  → 剩下的元素，**順序未指定**
//   nth_element(first, nth, last)：
//     * *nth            → 恰好是「若整個範圍排好序，該位置會出現的那個元素」
//     * [first, nth)    → 全部 **不大於** *nth，但彼此之間**順序未指定**
//     * (nth, last)     → 全部 **不小於** *nth，但彼此之間**順序未指定**
// 本檔輸出中那些「未指定」區域顯示的排列，是 **libstdc++ 15.2 的實測結果，
// 並非標準保證**——換編譯器、換最佳化等級、甚至換輸入長度都可能不同。
// **絕對不可以依賴這些位置的排列。**
//
// 【4. nth_element 找中位數：注意偶數長度的定義】
// 對長度 N 的資料，nth_element(begin, begin + N/2, end) 之後，
// v[N/2] 是「排序後位於索引 N/2 的元素」。
//   * N 為奇數：這就是中位數。
//   * N 為偶數：中位數的數學定義是中間兩個數的平均，
//     此時 v[N/2] 只是「上中位數」，還需要再找 [begin, begin+N/2) 的最大值
//     （那段的最大值就是下中位數，且該段已保證都不大於 v[N/2]，
//      所以用 max_element 掃一次即可，仍是 O(N)）。
// 這是統計程式最常見的邊界錯誤之一。
//
// 【概念補充 Concept Deep Dive】
//
// (A) partial_sort 的實作：用 heap 而不是排序
//   libstdc++ 的做法是：先把前 M 個元素做成 max-heap，
//   然後掃描剩下的 N-M 個元素，只要比 heap 頂端小就替換掉頂端並重新調整。
//   最後對這個 heap 做 heapsort 得到有序結果。
//   總成本約 O(N log M)——每個元素最多一次 O(log M) 的 heap 調整。
//   當 M 遠小於 N 時（例如一萬筆取前 10 名），log M 是常數等級，
//   幾乎等同線性掃描。
//
// (B) nth_element 的最壞情況
//   標準只規定**平均**線性，未明定最壞上界。
//   libstdc++ 的實作在遞迴過深時會退回 heapsort（與 introsort 同樣的自保策略），
//   實務上不會出現 O(N²)——但這是**實作品質保證，不是標準保證**。
//
// (C) 為什麼 top-K 問題不總是用 nth_element
//   nth_element 會**改動原容器**且需要 Random Access Iterator。
//   若資料是串流（無法一次載入）或不允許改動原資料，
//   標準做法是維護一個大小為 K 的 priority_queue（min-heap），
//   成本 O(N log K)、空間 O(K)。本檔的 Top-N 實務範例會示範 nth_element 版本，
//   因為監控資料本來就已經全部在記憶體裡。
//
// 【注意事項 Pay Attention】
// 1. **兩者都需要 Random Access Iterator**；list / forward_list 不能用。
// 2. **[middle, last) 與 nth 兩側的順序是「未指定」**，
//    本檔輸出中的排列是 libstdc++ 實測結果，非標準保證，不可依賴。
// 3. nth_element 的 nth **可以等於 last**（此時什麼都不做），
//    但不可超過 last，否則是未定義行為。
// 4. 只需要**單一**最大／最小值時用 max_element / min_element（O(N)），
//    不必動用 nth_element。
// 5. 偶數長度求中位數時，v[N/2] 只是上中位數，還要另外取下中位數再平均。
// 6. 比較器同樣必須滿足**嚴格弱序**（用 < 不用 <=），理由見第 11 個檔案。
// 7. nth_element 平均 O(N)、最壞情況標準未保證；libstdc++ 會退回 heapsort 自保。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】partial_sort 與 nth_element
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 要從 100 萬筆資料中找出最小的 10 筆，你會怎麼做？為什麼不直接 sort？
//     答：用 std::partial_sort(v.begin(), v.begin() + 10, v.end())。
//         std::sort 是 O(N log N)，會替另外 999,990 筆完全沒用到的資料排序；
//         partial_sort 是 O(N log M)，M = 10，log M 幾乎是常數，
//         成本接近線性掃描。若連「前 10 名的順序」都不需要，
//         用 nth_element 更快（平均 O(N)）。
//     追問：那如果資料是串流、無法全部載入記憶體呢？→ 改用大小為 K 的
//         priority_queue（min-heap），O(N log K) 時間、O(K) 空間，
//         且不需要改動或持有原始資料。
//
// 🔥 Q2. nth_element 為什麼能達到平均 O(N)，比排序的 O(N log N) 還快？
//     答：它用 quickselect——本質是「只遞迴一邊的 quicksort」。
//         選 pivot 分割後，判斷目標位置 n 落在哪一半，**只往那一半繼續**。
//         quicksort 兩邊都要處理是 T(N)=2T(N/2)+N=O(N log N)；
//         quickselect 只處理一邊是 T(N)=T(N/2)+N，等比級數收斂到 O(N)。
//         代價是只保證第 n 個位置正確，其他位置僅保證分邊、不保證有序。
//     追問：最壞情況呢？→ 標準只規定平均線性，未明定最壞上界。
//         libstdc++ 在遞迴過深時會退回 heapsort，但那是實作品質保證，非標準保證。
//
// ⚠️ 陷阱. 執行 std::nth_element(v.begin(), v.begin() + 4, v.end()) 之後，
//        v[0] 到 v[3] 是排序好的嗎？
//     答：**不是**。標準只保證這四個元素都「不大於」v[4]，
//         它們**彼此之間的順序完全未指定**。若你需要前 5 名照順序，
//         要用 partial_sort 而不是 nth_element。
//     為什麼會錯：nth_element 執行後印出來，前面幾個「看起來像排好的」，
//         因為 quickselect 的分割過程碰巧產生了有序的結果，
//         而且小資料時特別容易看起來正常。這是最典型的
//         「測試通過但依賴了未指定行為」——換個編譯器版本、換筆輸入就會壞。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 215. Kth Largest Element in an Array
//   題目：回傳陣列中第 k 大的元素（是排序後的第 k 大，不是第 k 個相異值）。
//   為什麼用到本主題：這題**只要一個值**，不需要整個陣列有序 —— 正是
//         nth_element 的定義。第 k 大等於「排序後索引 n-k」的位置。
//   複雜度：平均 O(N)，優於先 sort 的 O(N log N)。
// -----------------------------------------------------------------------------
int findKthLargest(std::vector<int> nums, int k) {
    auto target = nums.begin() + (static_cast<int>(nums.size()) - k);
    std::nth_element(nums.begin(), target, nums.end());
    return *target;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 347. Top K Frequent Elements
//   題目：回傳出現頻率最高的 k 個元素（答案順序不限）。
//   為什麼用到本主題：先用 map 統計頻率，再從「(值, 次數)」清單中取出
//         頻率最高的 k 個。因為**題目明說順序不限**，用 nth_element
//         剛好夠用（平均 O(M)），不需要 partial_sort 或整體排序。
//   複雜度：統計 O(N log M) + 選取平均 O(M)，M 為相異值個數。
//   註：這裡對取出的 k 個再排序一次，只是為了讓本檔輸出可重現
//       （nth_element 之後那 k 個的順序是未指定的）。
// -----------------------------------------------------------------------------
std::vector<int> topKFrequent(const std::vector<int>& nums, int k) {
    std::map<int, int> freq;
    for (int n : nums) ++freq[n];

    std::vector<std::pair<int, int>> items(freq.begin(), freq.end());  // (值, 次數)

    // 依「次數由大到小」把第 k 個放到正確位置；前 k 個就是答案（順序未指定）
    auto kth = items.begin() + k;
    if (kth != items.end()) {
        std::nth_element(items.begin(), kth, items.end(),
                         [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                             return a.second > b.second;   // 次數大的排前面
                         });
    }

    std::vector<int> result;
    for (int i = 0; i < k && i < static_cast<int>(items.size()); ++i) {
        result.push_back(items[i].first);
    }
    std::sort(result.begin(), result.end());   // 僅為讓輸出可重現
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】監控儀表板：找出最慢的 N 支 API + 計算延遲中位數
//   情境：APM 系統每分鐘收到數千筆 API 延遲資料，儀表板要顯示
//         (a)「最慢的 3 支 API」排行榜——要照順序，用 partial_sort
//         (b)「延遲中位數」——只要一個值，用 nth_element（比排序快）
//   為什麼用到本主題：這正是兩者的分工示範。P50/P95 這類分位數統計
//         在監控系統中極常見，用 nth_element 可以省下大量排序成本。
// -----------------------------------------------------------------------------
struct ApiSample {
    std::string endpoint;
    int latencyMs;
};

void dashboardReport(std::vector<ApiSample> samples, std::size_t topN) {
    if (samples.empty()) return;

    // (a) 最慢的 topN 支：要排行榜順序 → partial_sort
    if (topN > samples.size()) topN = samples.size();
    std::partial_sort(samples.begin(), samples.begin() + topN, samples.end(),
                      [](const ApiSample& a, const ApiSample& b) {
                          return a.latencyMs > b.latencyMs;   // 延遲大的排前面
                      });
    std::cout << "  最慢的 " << topN << " 支 API:" << std::endl;
    for (std::size_t i = 0; i < topN; ++i) {
        std::cout << "    " << (i + 1) << ". " << samples[i].endpoint
                  << "  " << samples[i].latencyMs << "ms" << std::endl;
    }

    // (b) 延遲中位數：只要一個值 → nth_element（平均 O(N)）
    std::vector<int> lat;
    lat.reserve(samples.size());
    for (const auto& s : samples) lat.push_back(s.latencyMs);

    const std::size_t n = lat.size();
    std::nth_element(lat.begin(), lat.begin() + n / 2, lat.end());
    const int upper = lat[n / 2];
    if (n % 2 == 1) {
        std::cout << "  延遲中位數: " << upper << "ms (奇數筆，直接取中間)" << std::endl;
    } else {
        // 偶數筆：還需要下中位數。左半段已保證都不大於 upper，取其最大值即可
        const int lower = *std::max_element(lat.begin(), lat.begin() + n / 2);
        std::cout << "  延遲中位數: " << (lower + upper) / 2.0
                  << "ms (偶數筆，取中間兩筆 " << lower << " 與 " << upper << " 的平均)"
                  << std::endl;
    }
}

int main() {
    // partial_sort：只排序前 n 個, 這裡將 vec 中的元素部分排序，使得前 3 個元素是最小的，且已經排序好
    std::cout << "=== partial_sort ===" << std::endl;
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    // 只需要最小的 3 個，排好放在最前面, 其他元素不保證順序
    std::partial_sort(vec.begin(), vec.begin() + 3, vec.end());
    std::cout << "前 3 個最小（已排序）: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "  ↑ 只有前 3 個 (";
    for (int i = 0; i < 3; ++i) std::cout << vec[i] << (i < 2 ? " " : "");
    std::cout << ") 是標準保證的；" << std::endl;
    std::cout << "    第 4 個之後的排列屬「未指定順序」，此處顯示的是 libstdc++ 實測結果，"
              << "非標準保證" << std::endl;

    // nth_element：把第 n 個元素放到它排序後該在的位置
    std::cout << "\n=== nth_element ===" << std::endl;
    std::vector<int> vec2 = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    // 索引 4（第 5 小）就位；左邊都不大於它、右邊都不小於它，但兩側各自的順序未指定
    std::nth_element(vec2.begin(), vec2.begin() + 4, vec2.end());
    std::cout << "nth_element(4) 後: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "索引 4 的元素（第 5 小）: " << vec2[4]
              << "  ← 這是唯一被標準保證的位置" << std::endl;
    std::cout << "  ↑ 左右兩側只保證「都不大於／都不小於」，各自的排列未指定，"
              << "不可依賴" << std::endl;

    // 驗證標準真正保證的性質（這個檢查在任何實作上都必定成立）
    bool leftOk = std::all_of(vec2.begin(), vec2.begin() + 4,
                              [&](int x) { return x <= vec2[4]; });
    bool rightOk = std::all_of(vec2.begin() + 5, vec2.end(),
                               [&](int x) { return x >= vec2[4]; });
    std::cout << "  標準保證檢查 → 左側都 <= 它: " << (leftOk ? "成立" : "不成立")
              << "，右側都 >= 它: " << (rightOk ? "成立" : "不成立") << std::endl;

    std::cout << "\n=== LeetCode 215. Kth Largest Element in an Array ===" << std::endl;
    std::cout << "[3,2,1,5,6,4], k=2 -> " << findKthLargest({3, 2, 1, 5, 6, 4}, 2) << std::endl;
    std::cout << "[3,2,3,1,2,4,5,5,6], k=4 -> "
              << findKthLargest({3, 2, 3, 1, 2, 4, 5, 5, 6}, 4) << std::endl;

    std::cout << "\n=== LeetCode 347. Top K Frequent Elements ===" << std::endl;
    std::cout << "[1,1,1,2,2,3], k=2 -> ";
    for (int n : topKFrequent({1, 1, 1, 2, 2, 3}, 2)) std::cout << n << " ";
    std::cout << "(題目允許任意順序，此處已排序以便對照)" << std::endl;

    std::cout << "[1], k=1 -> ";
    for (int n : topKFrequent({1}, 1)) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：API 監控儀表板 ===" << std::endl;
    dashboardReport({
        {"/api/orders",   820},
        {"/api/users",     95},
        {"/api/search",  1240},
        {"/api/login",    150},
        {"/api/report",   670},
        {"/api/health",    12}
    }, 3);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計12.cpp -o demo12

//     第 4 個之後的排列屬「未指定順序」，此處顯示的是 libstdc++ 實測結果，非標準保證

// === 預期輸出 ===
// === partial_sort ===
// 前 3 個最小（已排序）: 1 2 3 8 9 5 7 4 6 
//   ↑ 只有前 3 個 (1 2 3) 是標準保證的；
//
// === nth_element ===
// nth_element(4) 後: 2 1 4 3 5 6 7 9 8 
// 索引 4 的元素（第 5 小）: 5  ← 這是唯一被標準保證的位置
//   ↑ 左右兩側只保證「都不大於／都不小於」，各自的排列未指定，不可依賴
//   標準保證檢查 → 左側都 <= 它: 成立，右側都 >= 它: 成立
//
// === LeetCode 215. Kth Largest Element in an Array ===
// [3,2,1,5,6,4], k=2 -> 5
// [3,2,3,1,2,4,5,5,6], k=4 -> 4
//
// === LeetCode 347. Top K Frequent Elements ===
// [1,1,1,2,2,3], k=2 -> 1 2 (題目允許任意順序，此處已排序以便對照)
// [1], k=1 -> 1 
//
// === 日常實務：API 監控儀表板 ===
//   最慢的 3 支 API:
//     1. /api/search  1240ms
//     2. /api/orders  820ms
//     3. /api/report  670ms
//   延遲中位數: 410ms (偶數筆，取中間兩筆 150 與 670 的平均)
