// =============================================================================
//  05_random_access_iterator.cpp  —  RandomAccessIterator
// =============================================================================
//  特性 (LegacyRandomAccessIterator)：
//   * 在 BidirectionalIterator 之上再加上「O(1) 跳躍」與「比較大小」：
//       it + n, it - n           跳 n 步
//       it += n, it -= n
//       it1 - it2                兩 iterator 距離 (回傳 difference_type)
//       it[n]                    等同 *(it + n)
//       it1 < it2, <=, >, >=     位置比較
//   * 所有操作必須是 O(1)。
//
//  典型容器 / 來源：
//   * std::vector<T>
//   * std::array<T, N>
//   * std::deque<T>           (注意：deque 是 random_access 但「不連續」)
//   * 原生陣列 / 指標 (T*)
//   * std::string             (string::iterator 是 random_access)
//
//  必須 RandomAccess 才能用的演算法：
//   * std::sort, std::stable_sort, std::nth_element, std::partial_sort
//   * std::binary_search, std::lower_bound (前者也接受 ForwardIterator，
//     但只有 RandomAccess 才有 O(log n)；ForwardIterator 上是 O(n) 步數)
//   * std::random_shuffle / std::shuffle
//
//  陷阱：
//   * deque<T>::iterator 雖然是 random_access，但底層不連續，不可當 T* 使用，
//     也不可餵給只接受連續記憶體的 C API。
//   * vector 在 push_back 後，原來的 iterator 可能因 reallocation 而失效。
//
//  參考連結 (cppreference / cplusplus)：
//    https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator  — 規格
//    https://en.cppreference.com/w/cpp/container/vector                — vector
//    https://en.cppreference.com/w/cpp/container/deque                 — deque
//    https://en.cppreference.com/w/cpp/container/array                 — array
//    https://cplusplus.com/reference/iterator/RandomAccessIterator/    — 簡明
// =============================================================================

/*
補充筆記：random_access_iterator
  - random_access_iterator 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - random access iterator 支援 it+n、it-n、it[n] 和 O(1) 距離計算。
  - vector、deque、array 提供 random access iterator；list 不提供，因此 sort algorithm 不能直接用在 list iterator 上。
  - 二分搜尋在 forward iterator 上比較次數仍是 logN，但移動 iterator 可能是 O(N)。
*/
#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <numeric>
#include <string>

int main() {
    std::vector<int> v = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    // -----------------------------------------------------------------------
    // 範例 1：跳躍 + 索引
    // -----------------------------------------------------------------------
    auto it = v.begin();
    std::cout << "*(it + 3)  = " << *(it + 3) << " (期望 1)\n";
    std::cout << "it[5]      = " << it[5]      << " (期望 3)\n";
    std::cout << "v.end()-v.begin() = " << (v.end() - v.begin())
              << " (期望 9，這就是 size)\n";

    // -----------------------------------------------------------------------
    // 範例 2：std::sort — 需要 random_access
    // -----------------------------------------------------------------------
    std::sort(v.begin(), v.end());
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 3：二分搜尋 (lower_bound) — random_access 才能 O(log n)
    // -----------------------------------------------------------------------
    auto pos = std::lower_bound(v.begin(), v.end(), 6);
    std::cout << "lower_bound(6) at index "
              << std::distance(v.begin(), pos)        // 用 distance 取得索引
              << ", *pos = " << *pos << '\n';

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 1：
    //   LC 704. Binary Search
    //   經典二分搜尋 — left/right 是 random_access iterator (此處用索引)
    // -----------------------------------------------------------------------
    auto binary_search_lc = [](const std::vector<int>& nums, int target) -> int {
        int l = 0, r = (int)nums.size() - 1;
        while (l <= r) {
            int m = l + (r - l) / 2;     // 這個 (l + r) / 2 寫法可避免 overflow
            if (nums[m] == target) return m;
            else if (nums[m] < target) l = m + 1;
            else r = m - 1;
        }
        return -1;
    };
    std::vector<int> sorted = {-1, 0, 3, 5, 9, 12};
    std::cout << "binary_search find 9 → idx "
              << binary_search_lc(sorted, 9) << " (期望 4)\n";

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 2：
    //   LC 215. Kth Largest Element (用 nth_element — 需要 random_access)
    // -----------------------------------------------------------------------
    auto find_kth_largest = [](std::vector<int> nums, int k) {
        // 第 k 大 = 排序後 nums[n-k]，用 nth_element 平均 O(n)
        std::nth_element(nums.begin(),
                         nums.begin() + (nums.size() - k),
                         nums.end());
        return nums[nums.size() - k];
    };
    std::vector<int> nums = {3, 2, 1, 5, 6, 4};
    std::cout << "kth largest (k=2) = " << find_kth_largest(nums, 2)
              << " (期望 5)\n";

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 3：
    //   LC 35. Search Insert Position
    //   * 已排序陣列裡找 target，找到就回傳 index；找不到就回傳「該插入的位置」。
    //   * std::lower_bound 正是這個語意：回傳第一個「不小於 target」的位置。
    //     找到就指向它；找不到就指向「該插入處」(可能是 end())。
    //   * 算 index = pos - begin (這就是 RandomAccessIterator 的優勢 — O(1) 算距離)。
    // -----------------------------------------------------------------------
    auto search_insert = [](const std::vector<int>& v, int target) {
        auto p = std::lower_bound(v.begin(), v.end(), target);
        return (int)(p - v.begin());                 // 直接相減 = random_access
    };
    std::vector<int> sv = {1, 3, 5, 6};
    std::cout << "searchInsert(5) = " << search_insert(sv, 5) << " (期望 2)\n";
    std::cout << "searchInsert(2) = " << search_insert(sv, 2) << " (期望 1)\n";
    std::cout << "searchInsert(7) = " << search_insert(sv, 7) << " (期望 4)\n";

    // -----------------------------------------------------------------------
    // 課程知識補充：兩個 RandomAccess 容器的「奇怪特例」
    //
    //   1. std::deque<T>
    //      * iterator 是 random_access (it+n / it[n] / it1-it2 都 O(1)).
    //      * 但底層「不是連續記憶體」— 它是分塊的 (chunked)，所以：
    //          - &deque[0] 不能當成「一塊連續記憶體」傳給 C API.
    //          - std::data(deque) 是非法的 (沒有這個重載).
    //          - 在中間 push_back / push_front 後，「iterator 全部失效」
    //            (跟 vector 中間插入的失效類似)，但「reference 仍有效」.
    //
    //   2. std::vector<bool>
    //      * 標準特化 — 為了省記憶體，每個元素「只佔 1 bit」.
    //      * 因此 *it 「不是」回傳 bool& —— 而是回傳一個「proxy reference」.
    //      * 後果：
    //          - bool& b = v[0];           // ← 編譯錯！沒有 bool&
    //          - 不滿足 ContiguousIterator (即使 iterator 標籤是 random_access)
    //          - 部份泛型程式對 vector<bool> 的行為「跟 vector<其他 T>」不一樣
    //      * 想要 bool 又要連續記憶體請改用 std::vector<char> 或 std::deque<bool>
    //        (其實 std::array<bool, N> / std::bitset<N> 通常更適合)
    // -----------------------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::deque 是 RandomAccess 但內部不連續，怎麼做到 O(1) 跳躍？
    //    A：deque 內部是「chunk 陣列 + 每 chunk 是固定大小的小段連續記憶體」，
    //      iterator 同時保存 (chunk_index, offset_in_chunk)。it+n 只是算術：
    //      新 chunk_index = (offset + n) / chunk_size，新 offset = (offset+n) % chunk_size，
    //      兩次除法/取餘就到位 → O(1)。代價是 it1 - it2 與快取局部性都比 vector 差，
    //      也不能用 std::data(deque) 取連續指標餵 C API。
    //
    //  Q2：vector<bool>::iterator 為什麼是「proxy reference」？會有什麼陷阱？
    //    A：vector<bool> 是標準特化，每元素只佔 1 bit。*it 不能回傳 bool& (因為
    //      不能取一個 bit 的位址)，於是回傳一個 proxy 物件。陷阱：
    //      auto& b = v[0];  // 推導出 proxy&，看似 OK 但實際不是 bool&
    //      bool& b = v[0];  // 編譯錯誤
    //      泛型程式拿 vector<bool> 試常常踩雷，建議改用 std::vector<char> 或
    //      std::bitset<N>。
    //
    //  Q3：raw pointer T* 是什麼等級的 iterator？
    //    A：T* 透過 std::iterator_traits<T*> 的「指標特化版」自動萃取出
    //      iterator_category = random_access_iterator_tag。所以 std::sort、
    //      std::lower_bound 都能直接接受 raw pointer，例如 std::sort(arr, arr+n)。
    //      這就是「指標也是 iterator」這句話的具體實現 — STL 演算法統一靠
    //      iterator_traits 萃取資訊，不在意實際是 class 還是 raw pointer。
    //

    // -----------------------------------------------------------------------
    // LC 範例: LC 33 — Search in Rotated Sorted Array (難度: medium)
    // -----------------------------------------------------------------------
    // 給「旋轉過的已排序陣列」(例 [4,5,6,7,0,1,2]) 找 target,要求 O(log n)。
    // 用變形二分搜尋:每次判斷「左半段是否有序」,再決定 target 落在哪半。
    //
    // 為何對 RandomAccessIterator 完美:演算法核心需要 mid = l + (r-l)/2 與
    // it + n、it - it 等運算 — 全部 O(1),才能撐起 O(log n) 整體複雜度。
    // 換到 Bidirectional iterator (如 list) 算 mid 就退化成 O(n),失去意義。
    {
        std::vector<int> a{4, 5, 6, 7, 0, 1, 2};
        int target = 0;
        int l = 0, r = (int)a.size() - 1, ans = -1;
        while (l <= r) {
            int mid = l + (r - l) / 2;            // ★ RandomAccess 才有的 O(1) 算法
            if (a[mid] == target) { ans = mid; break; }
            if (a[l] <= a[mid]) {                  // 左半段有序
                if (a[l] <= target && target < a[mid]) r = mid - 1;
                else                                   l = mid + 1;
            } else {                                // 右半段有序
                if (a[mid] < target && target <= a[r]) l = mid + 1;
                else                                   r = mid - 1;
            }
        }
        std::cout << "LC33 search(0) = " << ans << " (期望 4)\n";
    }

    // -----------------------------------------------------------------------
    // 實戰範例:分頁瀏覽 (Pagination) — 任意跳頁
    // -----------------------------------------------------------------------
    // 場景:後端 API 把 vector<Article> 切成「每頁 N 筆」,使用者可任意跳到第 K 頁。
    // 跳頁的核心是「從 begin() + (k * N) 開始取 N 筆」 — 需要 it + n 的 O(1) 跳躍,
    // 正是 RandomAccessIterator 的看家本領。若改用 list 則必須 advance N 步,
    // 整體變 O(N*K),分頁的效能優勢全無。
    {
        std::vector<std::string> articles{
            "A1","A2","A3","A4","A5","A6","A7","A8","A9","A10"
        };
        int page_size = 3;
        int page = 2;       // 第 3 頁 (0-based)
        auto first = articles.begin() + page * page_size;
        auto last  = std::min(first + page_size, articles.end());   // 不能超過 end
        std::cout << "第 " << (page+1) << " 頁: [ ";
        for (auto it = first; it != last; ++it) std::cout << *it << ' ';
        std::cout << "]\n";
        // 預期輸出: 第 3 頁: [ A7 A8 A9 ]
    }

    return 0;
}
