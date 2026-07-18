// ============================================================
// std::partial_sort
// 分類 (Category): Sorting operations (排序演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/partial_sort
//   * https://cplusplus.com/reference/algorithm/partial_sort/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::partial_sort 解的問題:
//
//   「我只想要『前 K 名』,而且要『排好』。後面的我不在乎。」
//
// 它對 [first, last) 進行排序,但只保證 [first, middle) 是
// 「前 K 小 (或 K 大,依比較器) 且已排序」;
// [middle, last) 部分順序未指定。
//
// 想像它的內部邏輯:
//   1. 拿 [first, middle) 做一個大小為 K 的 max-heap。
//   2. 對 [middle, last) 每個元素 x:若 x 比 heap 頂小,把 heap 頂換掉。
//   3. 最後對 [first, middle) sort_heap 排序輸出。
//   → 整體 O((N - K) log K) ≈ O(N log K)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼比 sort 快?                                     │
// └────────────────────────────────────────────────────────────┘
//
//   * sort 整段排序:O(N log N)
//   * partial_sort 只排前 K:O(N log K)
//
// 當 K << N (K 遠小於 N) 時,差距很可觀。
// 例如 N = 10^6,K = 10:
//   * sort       ≈ 10^6 × 20 = 2 × 10^7 次操作
//   * partial_sort ≈ 10^6 × log2(10) ≈ 3.3 × 10^6 次操作  (快 6 倍)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、partial_sort vs nth_element                            │
// └────────────────────────────────────────────────────────────┘
//
//   * nth_element  : 只保證「nth 那個位置」對,O(N) 平均。前後段亂。
//   * partial_sort : 前 K 個全排好且正確,後段亂。O(N log K)。
//
// 「我要『第 K 名』」 → nth_element (更快)
// 「我要『前 K 名,且排好顯示』」 → partial_sort
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt>
//   void partial_sort(RandomIt first, RandomIt middle, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   void partial_sort(RandomIt first, RandomIt middle, RandomIt last,
//                     Compare comp);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O((last - first) × log(middle - first)) — 約 O(N log K)
//   空間: O(1)
//   需求: RandomAccessIterator;middle 必須在 [first, last] 內
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. [middle, last) 順序「未指定」 — 別假設它仍保留原順序。
//   2. 不變動原資料、只取前 K → 用 partial_sort_copy。
//   3. 只要找「第 K 個」(不需排序前 K 個) → 用 nth_element 更快。
//   4. middle == last 時,等同於完整 sort (但會比 std::sort 慢)。
//
// ============================================================

/*
補充筆記：std::partial_sort
  - std::partial_sort 屬於排序與選取工具；先分清楚你需要完整排序、前 K 小、還是只要第 N 個元素就定位。
  - sort 不穩定，stable_sort 保留等價元素的原相對順序；資料有次排序鍵時穩定性很重要。
  - nth_element 只保證第 n 個元素就位，左邊不大於它、右邊不小於它，但左右兩邊各自不排序。
  - partial_sort 適合需要前 K 個已排序結果；若 K 遠小於 N，通常比完整 sort 更合適。
  - 比較器必須是 strict weak ordering，不能用 <= 當 less；這是排序初學者常犯的錯。
  - 排序會移動元素，保存的 iterator、pointer、reference 是否有效取決於容器和操作；排序 vector 時元素值位置會改變。
  - std::partial_sort 的比較器若把相等元素也判成 true，例如使用 <=，會破壞排序所需的 strict weak ordering。
  - std::partial_sort 呼叫後元素位置可能改變；若其他資料結構保存索引或指向元素的 iterator，要重新檢查關聯是否仍正確。
  - std::partial_sort 的選擇要看需求：完整排序用 sort/stable_sort，只要第 N 名用 nth_element，只要前 K 名用 partial_sort。
*/
#include <algorithm>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: Top-3 最小 ---
    std::vector<int> v{7, 2, 9, 1, 5, 8, 3, 6, 4};
    std::partial_sort(v.begin(), v.begin() + 3, v.end());
    std::cout << "smallest 3 sorted: " << v[0] << ' ' << v[1] << ' ' << v[2] << '\n';
    std::cout << "rest order is unspecified: ";
    for (size_t i = 3; i < v.size(); ++i) std::cout << v[i] << ' ';
    std::cout << '\n';

    // --- 範例 2: Top-3 最大 (用 greater) ---
    std::vector<int> w{7, 2, 9, 1, 5, 8, 3, 6, 4};
    std::partial_sort(w.begin(), w.begin() + 3, w.end(), std::greater<int>{});
    std::cout << "largest 3: " << w[0] << ' ' << w[1] << ' ' << w[2] << '\n';

    // --- 範例 3: middle == last → 等同於完整排序 ---
    std::vector<int> u{3, 1, 4, 1, 5, 9};
    std::partial_sort(u.begin(), u.end(), u.end());
    std::cout << "full sort: ";
    for (int x : u) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_215_kth_largest_via_partial_sort();
    void leetcode_347_top_k_frequent();
    void practical_leaderboard_top_n();
    void leetcode_1962_remove_stones_top_n_view();
    void practical_top_log_errors();
    leetcode_215_kth_largest_via_partial_sort();
    leetcode_347_top_k_frequent();
    practical_leaderboard_top_n();
    leetcode_1962_remove_stones_top_n_view();
    practical_top_log_errors();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 215: 陣列中的第 K 大元素 (用 partial_sort 的版本)
// ----------------------------------------------------------------
// 題目:給陣列 nums 與 k,回傳第 k 大的元素。
//
// 為什麼用 std::partial_sort:
//   * nth_element 只回傳「第 k 大」一個值。
//   * partial_sort 同時拿到「前 k 大且排序好」 — 對「想看 top-k 排行」更有用。
//   * 用 greater<> 讓「大者排前」,結果 nums[k-1] 就是第 k 大。
//
// 複雜度:時間 O(N log K);空間 O(1)。
void leetcode_215_kth_largest_via_partial_sort() {
    std::vector<int> nums{3, 2, 1, 5, 6, 4};
    int k = 2;
    std::partial_sort(nums.begin(), nums.begin() + k, nums.end(), std::greater<int>{});
    std::cout << "LC215 kth-largest=" << nums[k-1] << " top" << k << ":";
    for (int i = 0; i < k; ++i) std::cout << ' ' << nums[i];
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 347: 前 K 個高頻元素 (Top K Frequent Elements)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums 與 k,回傳出現次數最多的前 k 個元素。
//
// 為什麼用 std::partial_sort:
//   * 先用 hashmap 算頻率,得到 (value, count) 配對。
//   * 用 partial_sort 對配對依「count 降冪」取前 k。
//   * 達到 O(N + M log K),M 為相異元素數。
//
// 複雜度:時間 O(N + M log K);空間 O(M)。
void leetcode_347_top_k_frequent() {
    std::vector<int> nums{1,1,1,2,2,3};
    int k = 2;
    std::unordered_map<int,int> freq;
    for (int n : nums) ++freq[n];
    std::vector<std::pair<int,int>> arr(freq.begin(), freq.end());
    std::partial_sort(arr.begin(), arr.begin() + k, arr.end(),
                      [](const auto& a, const auto& b){ return a.second > b.second; });
    std::cout << "LC347 topK:";
    for (int i = 0; i < k; ++i) std::cout << ' ' << arr[i].first;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:遊戲排行榜前 N 名
// ----------------------------------------------------------------
// 場景:線上遊戲玩家分數陣列,只需顯示前 5 名 (依分數降冪),
//      其餘不關心順序。
//
// 為什麼用 std::partial_sort:
//   * 比 sort 全排序快:O(M log N) vs O(M log M)。
//   * 前 N 名要排好顯示 — 不能用 nth_element (內部不排序)。
void practical_leaderboard_top_n() {
    struct Player { std::string name; int score; };
    std::vector<Player> players{
        {"P1", 1200}, {"P2", 800}, {"P3", 1500}, {"P4", 950},
        {"P5", 1700}, {"P6", 600}, {"P7", 1100}, {"P8", 1300}
    };
    int n = 5;
    std::partial_sort(players.begin(), players.begin() + n, players.end(),
                      [](const Player& a, const Player& b){ return a.score > b.score; });
    std::cout << "leaderboard top" << n << ":";
    for (int i = 0; i < n; ++i)
        std::cout << " " << players[i].name << "(" << players[i].score << ")";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1962 變體:用 partial_sort 預覽「top 3 大石頭」
// ----------------------------------------------------------------
// 題目簡化:給石頭堆陣列,要顯示「最重的 3 堆」並排序顯示。
//
// 為什麼用 std::partial_sort:
//   要看 top 3 (有序),partial_sort 比完整 sort 快得多 (O(N log 3))。
//
// 複雜度:時間 O(N log K);空間 O(1)。
void leetcode_1962_remove_stones_top_n_view() {
    std::vector<int> piles{5, 4, 9, 12, 6, 11};
    int n = 3;
    std::partial_sort(piles.begin(), piles.begin() + n, piles.end(),
                      std::greater<int>{});
    std::cout << "top " << n << " heaps:";
    for (int i = 0; i < n; ++i) std::cout << ' ' << piles[i];
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:Log 系統 — 顯示「Top N 高頻錯誤訊息」
// ----------------------------------------------------------------
// 場景:錯誤訊息每天上百種,只想看「最常出現的前 5 種」(降冪排序)。
//      hashmap 計次 + partial_sort 取前 N。
void practical_top_log_errors() {
    std::vector<std::string> logs{
        "DB_TIMEOUT", "CONN_REFUSED", "DB_TIMEOUT", "AUTH_FAIL",
        "DB_TIMEOUT", "CONN_REFUSED", "AUTH_FAIL", "DB_TIMEOUT"
    };
    std::unordered_map<std::string, int> freq;
    for (auto& s : logs) ++freq[s];
    std::vector<std::pair<std::string, int>> arr(freq.begin(), freq.end());
    int n = std::min<int>(3, arr.size());
    std::partial_sort(arr.begin(), arr.begin() + n, arr.end(),
                      [](const auto& a, const auto& b){ return a.second > b.second; });
    std::cout << "top errors:";
    for (int i = 0; i < n; ++i)
        std::cout << " " << arr[i].first << "(" << arr[i].second << ")";
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// smallest 3 sorted: 1 2 3
// rest order is unspecified: 7 5 8 9 6 4   (順序依實作)
// largest 3: 9 8 7
// full sort: 1 1 3 4 5 9
// LC215 kth-largest=5 top2: 6 5
// LC347 topK: 1 2
// leaderboard top5: P5(1700) P3(1500) P8(1300) P1(1200) P7(1100)
// top 3 heaps: 12 11 9
// top errors: DB_TIMEOUT(4) CONN_REFUSED(2) AUTH_FAIL(2)
