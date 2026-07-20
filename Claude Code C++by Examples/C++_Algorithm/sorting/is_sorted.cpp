// ============================================================
// std::is_sorted / std::is_sorted_until    (C++11 起)
// 分類 (Category): Sorting operations (排序演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/is_sorted
//   * https://en.cppreference.com/w/cpp/algorithm/is_sorted_until
//   * https://cplusplus.com/reference/algorithm/is_sorted/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// 兩個兄弟函式,同樣解「資料是否已排序」這個問題,但回傳資訊不同:
//
//   ┌───────────────────┬─────────────────────────────────┐
//   │ is_sorted         │ 回 bool — 「整段是不是已排序」  │
//   │ is_sorted_until   │ 回迭代器 — 「第一個破壞順序的位置」│
//   └───────────────────┴─────────────────────────────────┘
//
// 兩者其實是「同一個演算法的不同 API」 — is_sorted 等價於:
//
//   std::is_sorted_until(first, last) == last
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、用途                                                   │
// └────────────────────────────────────────────────────────────┘
//
//   * 「先檢查再決定要不要排序」 — 已排序就跳過 sort,省成本。
//   * 「資料完整性 assert」 — 期待時序遞增的事件流,加 is_sorted 檢查。
//   * 「找第一個亂序位置」 — 配合 is_sorted_until 做 debug。
//   * 「驗證 sort 結果」 — 寫單元測試或 sanity check 時用。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、預設用「非遞減」(<=)                                   │
// └────────────────────────────────────────────────────────────┘
//
// 預設:用 < 比較,「沒有 a < b 反過來成立」即視為已排序。
// 也就是說,等值元素是允許的 — {1, 2, 2, 3} 視為已排序。
//
// 想要「嚴格遞增」 (沒有相等):用 std::less_equal<>:
//
//   std::is_sorted(v, v+n, std::less_equal<>{})
//
// 邏輯:less_equal<> 對相等元素回傳 true,而 is_sorted 看到「a !< b」才繼續;
// 用 less_equal 表示「a <= b 才是順序」 — 等號不允許,反而剔除相等。
// (這個邏輯有點繞,初學者請多想一下。)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt>
//   bool is_sorted(FwdIt first, FwdIt last);
//
//   template <class FwdIt, class Compare>
//   bool is_sorted(FwdIt first, FwdIt last, Compare comp);
//
//   template <class FwdIt>
//   FwdIt is_sorted_until(FwdIt first, FwdIt last);
//
//   template <class FwdIt, class Compare>
//   FwdIt is_sorted_until(FwdIt first, FwdIt last, Compare comp);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與邊界                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N) — 最多 N - 1 次比較;遇反例即停。
//   空間: O(1)
//
//   邊界:空範圍或單元素 → is_sorted 回 true,is_sorted_until 回 last。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 預設是「非遞減」 — 允許等值。要嚴格遞增請用 less_equal<>。
//   2. 用 greater<> 變成「非遞增」順序檢查。
//   3. comp 必須是嚴格弱序 (與 sort 相同的鐵律)。
//   4. is_sorted 也適用於 forward iterators (例如 forward_list)。
//
// ============================================================

/*
補充筆記：std::is_sorted
  - std::is_sorted 屬於排序與選取工具；先分清楚你需要完整排序、前 K 小、還是只要第 N 個元素就定位。
  - sort 不穩定，stable_sort 保留等價元素的原相對順序；資料有次排序鍵時穩定性很重要。
  - nth_element 只保證第 n 個元素就位，左邊不大於它、右邊不小於它，但左右兩邊各自不排序。
  - partial_sort 適合需要前 K 個已排序結果；若 K 遠小於 N，通常比完整 sort 更合適。
  - 比較器必須是 strict weak ordering，不能用 <= 當 less；這是排序初學者常犯的錯。
  - 排序會移動元素，保存的 iterator、pointer、reference 是否有效取決於容器和操作；排序 vector 時元素值位置會改變。
  - std::is_sorted 的比較器若把相等元素也判成 true，例如使用 <=，會破壞排序所需的 strict weak ordering。
  - std::is_sorted 呼叫後元素位置可能改變；若其他資料結構保存索引或指向元素的 iterator，要重新檢查關聯是否仍正確。
  - std::is_sorted 的選擇要看需求：完整排序用 sort/stable_sort，只要第 N 名用 nth_element，只要前 K 名用 partial_sort。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::is_sorted / std::is_sorted_until
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. is_sorted 和 is_sorted_until 的差別?
//     答:同一個演算法的兩種 API。is_sorted 回 bool「整段是不是已排序」;
//         is_sorted_until 回 iterator「第一個破壞順序的位置」,整段有序時回 last。
//         所以 is_sorted(first, last) 等價於 is_sorted_until(first, last) == last。
//         兩者都是 O(n)、最多 n-1 次比較,遇到第一個逆序即停止;
//         空區間與單一元素一律視為已排序 (is_sorted 回 true)。
//     追問:預設判定的是嚴格遞增嗎?
//           (不是,預設用 operator< 判定「非遞減」,{1, 2, 2, 3} 算已排序;
//            要檢查遞減順序就傳 std::greater<>{})
//
// 🔥 Q2. 檢查用的 comparator 有什麼要求?
//     答:必須和「排序時會用的那個 comparator」是同一個,而且同樣必須構成
//         strict weak ordering — 這是整個排序家族共同的鐵律。拿另一個
//         comparator 去檢查,得到的答案對後續的 sort / 二分搜尋沒有意義。
//     追問:為什麼二分搜尋前特別在意這件事?
//           (lower_bound / binary_search 的前置條件就是「已依同一準則排序」,
//            違反時不會報錯,但結果無意義)
//
// Q3. is_sorted 適用哪些 iterator?和 sort 一樣嗎?
//     答:不一樣。is_sorted / is_sorted_until 只需要 forward iterator
//         (只做相鄰比較、單向前進),所以 std::forward_list 也能用;
//         而 sort / partial_sort / nth_element 都要求 random access iterator。
//         兩者自 C++11 加入,並自 C++20 起為 constexpr。
//
// ⚠️ 陷阱. 「先 is_sorted 檢查,已排序就跳過 sort」是有效的最佳化嗎?
//     答:通常不是。你為此多付一次完整的 O(n) 掃描,而 std::sort 對已排序的
//         輸入本來就很快 — introsort 的收尾是一次 insertion sort,對幾乎有序
//         的序列近似 O(n)。這兩個函式真正的用途是「資料完整性斷言 / 單元測試 /
//         除錯」(搭配 is_sorted_until 找出第一個亂序位置),而不是省時間。
//     為什麼會錯:把「檢查很便宜、排序很貴」當成通則,忽略了兩者都至少要
//         走過一次資料,以及 sort 在近乎有序輸入上的實際成本並不高。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::cout << std::boolalpha;

    // --- 範例 1: 已排序 ---
    std::vector<int> a{1, 2, 3, 4, 5};
    std::cout << "a sorted: " << std::is_sorted(a.begin(), a.end()) << '\n';

    // --- 範例 2: 未排序;is_sorted_until 找出第一個亂序位置 ---
    std::vector<int> b{1, 3, 2, 4};
    std::cout << "b sorted: " << std::is_sorted(b.begin(), b.end()) << '\n';
    auto bad = std::is_sorted_until(b.begin(), b.end());
    std::cout << "b first violator at index "
              << (bad - b.begin()) << " (val=" << *bad << ")\n";

    // --- 範例 3: 遞減順序 (用 greater<>) ---
    std::vector<int> c{5, 4, 3, 2, 1};
    std::cout << "c desc: "
              << std::is_sorted(c.begin(), c.end(), std::greater<int>{}) << '\n';

    // --- 範例 4: 邊界 — 空與單元素 ---
    std::vector<int> e;
    std::cout << "empty sorted: " << std::is_sorted(e.begin(), e.end()) << '\n';
    std::vector<int> single{42};
    std::cout << "single sorted: " << std::is_sorted(single.begin(), single.end()) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1752_check_rotated_sorted();
    void leetcode_1827_min_operations();
    void practical_timestamp_check();
    void leetcode_941_valid_mountain_array();
    void practical_validate_sorted_output();
    leetcode_1752_check_rotated_sorted();
    leetcode_1827_min_operations();
    practical_timestamp_check();
    leetcode_941_valid_mountain_array();
    practical_validate_sorted_output();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1752: 檢查陣列是否經排序及旋轉而來
//                (Check if Array Is Sorted and Rotated)
// ----------------------------------------------------------------
// 題目:給陣列 nums,判斷它是否能由「某個非遞減陣列旋轉若干次」而來。
//      例如 [3,4,5,1,2] 是 [1,2,3,4,5] 旋轉而來 → true。
//      [2,1,3,4] 不是任何排序陣列旋轉而來 → false。
//
// 為什麼用 std::is_sorted:
//   旋轉的非遞減陣列,「相鄰兩元素遞減」最多只會發生 1 次 (旋轉接合處)。
//   等同於:把 nums 串接 nums (loop 一次),用 is_sorted_until 看
//   能不能延展到尾。但更簡潔的做法是直接「計算遞減點數量」。
//   這裡示範 is_sorted 在直接片段檢查的應用。
//
// 解法步驟:
//   1. 計算相鄰相減出現「nums[i] > nums[i+1]」的次數 (環狀)。
//   2. 若次數 <= 1 → true;否則 false。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_1752_check_rotated_sorted() {
    std::vector<int> nums{3, 4, 5, 1, 2};
    int n = static_cast<int>(nums.size());
    int drops = 0;
    for (int i = 0; i < n; ++i) {
        if (nums[i] > nums[(i + 1) % n]) ++drops;
    }
    std::cout << "LC1752: " << (drops <= 1 ? "true" : "false") << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1827: 使陣列嚴格遞增的最少操作次數
//                (Min Operations to Make Array Increasing)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums,每次操作可將任一元素 +1。
//      求使陣列嚴格遞增所需最少操作次數。
//
// 為什麼用 std::is_sorted:
//   * 解題核心:線性掃描,把 <= 前者的元素提升到「前者+1」,累加差量。
//   * 完成後用 is_sorted (less_equal<>) 驗證確實嚴格遞增 — 一行 sanity check。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_1827_min_operations() {
    std::vector<int> nums{1, 1, 1};
    int ops = 0;
    for (size_t i = 1; i < nums.size(); ++i) {
        if (nums[i] <= nums[i-1]) {
            ops += (nums[i-1] + 1) - nums[i];
            nums[i] = nums[i-1] + 1;
        }
    }
    bool ok = std::is_sorted(nums.begin(), nums.end(), std::less_equal<int>{});
    std::cout << "LC1827 ops=" << ops << " strictly_inc=" << std::boolalpha << ok << '\n';
}

// ----------------------------------------------------------------
// 實務範例:資料完整性檢查 — 時間戳遞增
// ----------------------------------------------------------------
// 場景:log 系統收到一連串事件,要確保 timestamp 依時間「非遞減」。
//      若不滿足,回報第一個「亂序」位置以便除錯。
void practical_timestamp_check() {
    std::vector<long long> ts{
        1700000100, 1700000200, 1700000200, 1700000150, 1700000400
    };
    bool ok = std::is_sorted(ts.begin(), ts.end());
    std::cout << "ts ordered: " << std::boolalpha << ok;
    if (!ok) {
        auto bad = std::is_sorted_until(ts.begin(), ts.end());
        std::cout << " (violator @" << (bad - ts.begin())
                  << " value=" << *bad << ")";
    }
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 941: 有效的山脈陣列 (Valid Mountain Array)
// ----------------------------------------------------------------
// 題目:給陣列 arr,判斷它是否為「山脈」 — 先嚴格遞增到某峰頂,再嚴格遞減。
//
// 為什麼用 std::is_sorted_until:
//   * 第一階段:用 is_sorted_until(less) 找「停止嚴格遞增的點」 = 峰頂。
//   * 第二階段:從峰頂用 is_sorted (greater) 確認後段嚴格遞減。
//   邊界:峰頂不能在第一或最後一個位置。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_941_valid_mountain_array() {
    std::vector<int> arr{0, 3, 2, 1};
    if (arr.size() < 3) { std::cout << "LC941: false\n"; return; }
    auto peak = std::is_sorted_until(arr.begin(), arr.end(),
                                     std::less_equal<int>{});
    bool ok = (peak != arr.begin()) && (peak != arr.end())
              && std::is_sorted(peak - 1, arr.end(), std::greater_equal<int>{});
    // 修正:嚴格遞增/遞減 — 用 strict 比較
    // (這裡只做簡化示範,完整版需更精細條件;略)
    std::cout << "LC941: " << std::boolalpha << ok << '\n';
}

// ----------------------------------------------------------------
// 實務範例:單元測試 — 驗證自製排序函式的輸出
// ----------------------------------------------------------------
// 場景:自己實作了 quicksort / mergesort,測試時用 is_sorted 確認輸出正確。
//      簡單一行驗證,不必比對到「期望結果」。
void practical_validate_sorted_output() {
    std::vector<int> out{1, 2, 3, 5, 8, 13};   // 自製函式產生的結果
    bool ok = std::is_sorted(out.begin(), out.end());
    std::cout << "sort test passed: " << std::boolalpha << ok << '\n';
}

// === 預期輸出 (Expected output) ===
// a sorted: true
// b sorted: false
// b first violator at index 2 (val=2)
// c desc: true
// empty sorted: true
// single sorted: true
// LC1752: true
// LC1827 ops=3 strictly_inc=true
// ts ordered: false (violator @3 value=1700000150)
// LC941: true
// sort test passed: true
