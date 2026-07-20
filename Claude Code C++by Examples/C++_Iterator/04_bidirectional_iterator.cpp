// =============================================================================
//  04_bidirectional_iterator.cpp  —  BidirectionalIterator
// =============================================================================
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼這個 iterator 存在?                              │
//  └────────────────────────────────────────────────────────────┘
//
//  Forward 解決了「多次走訪」、但只能「往前」。可現實有很多場景需要「往回」:
//
//    * 雙端逼近 (two-pointer from both ends):回文判斷、雙端排序
//    * 從容器尾端開始往前找第一個符合條件 (例如 std::find_if 走 rbegin/rend)
//    * 有序 set/map 裡「最大元素」 → 走 --end()
//    * std::reverse 演算法本身就需要 -- 才能交換對稱位置
//
//  STL 抽象出一層「Bidirectional」:
//    Forward 的所有能力 (含 multi-pass)
//      + 「-- (回退)」
//
//  資料結構面:雙向 linked list (std::list)、平衡 BST (std::set / std::map)
//             這些都能往前也能往後走 — 但「跳第 N 個」要 O(n),不是 RandomAccess。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、特性 (LegacyBidirectionalIterator)                     │
//  └────────────────────────────────────────────────────────────┘
//
//   * 在 ForwardIterator 之上新增「往回走」:
//       --it / it--      回退一步 (符合可遞迴的對稱性 — ++it 走完,--it 應能還原)
//   * 仍然不支援 it + n、it1 - it2、it[n] (那是 RandomAccess)。
//   * 仍然支援 multi-pass:可以拷副本後從同位置走第二次。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、底層機制 — 為什麼 std::list 是 Bidirectional?          │
//  └────────────────────────────────────────────────────────────┘
//
//  std::list 是「雙向 linked list」,每個節點配置在 heap 上、各持有兩個指標:
//
//      ┌──────┐        ┌──────┐        ┌──────┐
//      │ prev │←───────│ prev │←───────│ prev │
//      │ data │        │ data │        │ data │
//      │ next │───────→│ next │───────→│ next │
//      └──────┘        └──────┘        └──────┘
//
//  iterator 內部就是「指向某個節點的指標」:
//
//      class list_iterator {
//          Node* node_;
//          T& operator*()  const { return node_->data; }
//          list_iterator& operator++() { node_ = node_->next; return *this; }
//          list_iterator& operator--() { node_ = node_->prev; return *this; }
//      };
//
//  ++ 跟 -- 都是 O(1) 跳指標,但 +n 要走 n 步 → 不是 RandomAccess。
//  這也說明為什麼「list 的記憶體不連續」 — 每個節點是獨立 heap 物件,
//  cache locality 比 vector 差,但 insert / erase 任意位置是 O(1)。
//
//  std::set / std::map 的 iterator 走的是 BST (通常 RB-tree) 的中序前驅 / 後繼,
//  ++ 與 -- 平均都是攤銷 O(1) (最壞 O(log n))。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、典型容器                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * std::list<T>            (雙向 linked list)
//   * std::set / multiset
//   * std::map / multimap
//   * std::reverse_iterator   (adaptor,等級沿用底層,所以 list 的 reverse_it
//                              也是 bidirectional)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、能做但 ForwardIterator 做不到的事                      │
//  └────────────────────────────────────────────────────────────┘
//
//   * std::reverse、std::reverse_iterator(it)
//   * std::prev(it) — 退一步 (和 std::next 對稱)
//   * 從尾巴往前掃 (例如 it = c.end(); --it;)
//   * std::stable_partition
//   * std::inplace_merge
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 六、Pitfalls (陷阱)                                        │
//  └────────────────────────────────────────────────────────────┘
//
//   1. c.end() 是「過尾」位置,不能解參考;但可以 --c.end() 取最後一個元素
//      (前提是容器非空,否則 --begin() 是 UB)。
//   2. std::sort 需要 RandomAccess,所以不能直接 std::sort(list.begin(), list.end())!
//      std::list 自己有成員 list.sort()(內部用 mergesort)。
//   3. 從 begin() 再 -- 是 UB — 沒有「begin 之前」這個位置。
//   4. 對 std::set / std::map 修改元素值 (透過非 const iterator) 也是 UB —
//      因為會破壞紅黑樹的順序;set::iterator 規範上是 const,但 multiset 偶爾被誤用。
//   5. 不要用 it < it2 比較 (那是 RandomAccess);bidirectional 只有 ==/!=。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 七、參考連結 (References)                                  │
//  └────────────────────────────────────────────────────────────┘
//
//    https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator — 規格
//    https://en.cppreference.com/w/cpp/container/list                   — list
//    https://en.cppreference.com/w/cpp/container/set                    — set
//    https://en.cppreference.com/w/cpp/iterator/prev                    — std::prev
//    https://cplusplus.com/reference/iterator/BidirectionalIterator/    — 簡明
// =============================================================================

/*
補充筆記：bidirectional_iterator
  - bidirectional_iterator 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - bidirectional iterator 在 forward 能力上增加 --，可向前也可向後走。
  - list、set、map 的 iterator 通常可雙向移動，但不能用 it + n。
  - reverse_iterator 依賴可向後走的能力，因此至少需要 bidirectional iterator。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】BidirectionalIterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. BidirectionalIterator 比 Forward 多了什麼？哪些容器提供？
//     答：多了往回走的 --it / it--，其餘（含 multi-pass）與 forward 相同；仍然沒有
//         it + n、it1 - it2、it[n]，那些是 random access 才有。標準容器中 list、set、map、
//         multiset、multimap 的 iterator 屬於這一級。
//     追問：為什麼 list 的 ++ / -- 是 O(1) 卻不是 random access？（節點只能一步一步跳
//         指標，走第 n 個要 O(n)）／reverse_iterator 最低需要哪一級？（bidirectional）
//
// 🔥 Q2. 為什麼 std::sort(l.begin(), l.end()) 對 std::list 編譯不過？
//     答：std::sort 需要 random access iterator（要做區間分割與跳躍），list 只提供
//         bidirectional。list 自己有成員函式 l.sort()，內部用 merge sort，透過重接節點
//         指標完成排序，不搬動元素。
//     追問：那 std::reverse 對 list 可以嗎？（可以，它只需要 bidirectional）
//
// ⚠️ 陷阱. 用 --c.end() 取最後一個元素，什麼時候會出事？
//     答：容器為空時是 UB——沒有「begin 之前」這個位置，對 begin() 再 -- 同樣是 UB。
//         取最後一個元素前必須先確認 !c.empty()。
//     為什麼會錯：大家記得「*end() 不能解參考」，卻忘了 -- 本身在邊界上就已經越界；
//         空容器時 end() == begin()，退一步就掉出合法範圍了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <set>
#include <vector>
#include <iterator>
#include <algorithm>
#include <string>

int main() {
    // -----------------------------------------------------------------------
    // 範例 1:std::list 雙向走訪
    //   正向用 begin/end + ++,反向必須先 --end() 再讀 — 因為 end 是過尾。
    // -----------------------------------------------------------------------
    std::list<int> l = {10, 20, 30, 40, 50};

    std::cout << "正向: ";
    for (auto it = l.begin(); it != l.end(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    std::cout << "反向: ";
    auto it = l.end();
    while (it != l.begin()) { --it; std::cout << *it << ' '; }
    std::cout << '\n';

    // 也可以直接用 reverse_iterator (見 06_reverse_iterator.cpp):
    std::cout << "rbegin/rend: ";
    for (auto rit = l.rbegin(); rit != l.rend(); ++rit) std::cout << *rit << ' ';
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 2:std::reverse — 演算法需求 = BidirectionalIterator
    //   list 的 iterator 是 bidirectional,所以 std::reverse 可以用。
    //   底層做的事就是「兩端 swap,內向逼近」 — 必須能 --。
    // -----------------------------------------------------------------------
    std::reverse(l.begin(), l.end());
    std::cout << "after reverse: ";
    for (int x : l) std::cout << x << ' ';
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 3:std::set 內找最大值 = 走到 --end()
    //   set 內部已排序,這比 max_element 快很多 → O(1) vs O(n)。
    //   (注意:set 是紅黑樹,「最右下」就是最大值,--end() 等於走到那個節點)
    // -----------------------------------------------------------------------
    std::set<int> s = {3, 1, 4, 1, 5, 9, 2, 6};
    if (!s.empty()) {
        auto last = s.end();
        --last;     // BidirectionalIterator 才能這樣做!
        std::cout << "set max = " << *last << " (期望 9)\n";
    }

    // === LeetCode / 實務範例 ===
    void leetcode_125_valid_palindrome();
    void leetcode_977_squares_of_sorted_array();
    void practical_recently_used_cache_walk();
    void leetcode_344_reverse_string();
    void practical_undo_redo_browse();
    leetcode_125_valid_palindrome();
    leetcode_977_squares_of_sorted_array();
    practical_recently_used_cache_walk();
    leetcode_344_reverse_string();
    practical_undo_redo_browse();

    // -----------------------------------------------------------------------
    // 課程知識補充:為什麼 std::sort 對 std::list / std::set 不能用?
    //
    //   * std::sort 需要 RandomAccessIterator,因為它是 introsort
    //     (quicksort + heapsort + insertion sort 的混合),需要 it+n 與 it[n]
    //     才能做「pivot 位置選擇」、「中位數三取一」等操作。
    //   * std::list 的 iterator 只到 BidirectionalIterator,沒有 it+n。
    //     因此 std::list 自己提供成員函式 list::sort(),底層用 mergesort
    //     (對 linked list 是 O(n log n) 且 in-place,且只調整指標、不搬資料)。
    //   * std::set 是「排序樹」,元素本來就有序,沒有 sort 需求;要排序
    //     「另一個容器」的話就先把元素拷出來再 std::sort。
    //
    //   小結:
    //     - 連續記憶體 (vector / array / deque) → std::sort
    //     - linked list → list::sort / forward_list::sort
    //     - 已排序容器 (set / map) → 不必 sort
    // -----------------------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:為什麼 std::list 是 Bidirectional 而 std::forward_list 只是 Forward?
    //    A:list 是「雙向 linked list」,每個節點同時持有 prev 與 next 指標,
    //      所以 ++ 與 -- 都是 O(1)。forward_list 為了「省記憶體 (每節點少一個 prev)」
    //      只持有 next,因此走完就無法回頭,iterator 等級停在 Forward。
    //      實務上 forward_list 適合資料量大、節點數百萬以上,記憶體成本敏感的場景。
    //
    //  Q2:為什麼 std::set / std::map 的 iterator 是 Bidirectional 而不是 RandomAccess?
    //    A:set/map 底層是紅黑樹 (RB-tree)。++ 是「中序後繼」,-- 是「中序前驅」,
    //      平均攤銷 O(1)。但「跳第 N 個元素」(it+n) 沒有 O(1) 算法,只能走 n 步,
    //      不符 RandomAccess 的 O(1) 要求。因此標準把它定為 Bidirectional。
    //      也因此 std::sort / std::lower_bound (對 set 而言) 都不能用 — 但 set
    //      自己已經排序好了,改用 set::find / set::lower_bound 成員版即可。
    //
    //  Q3:為什麼從 begin() 再 -- 是 UB,而 --end() 是合法的?
    //    A:[begin, end) 是 STL 的標準有效範圍。end() 是「過尾哨兵」,不能解參考
    //      但可以 -- 退一格到最後元素 (前提是容器非空)。begin() 已是「第一個元素」,
    //      它「之前」沒有任何節點 — 對 list 而言 prev 指向哨兵節點,對 set 是
    //      未定義位置。再 -- 一次規範直接定為 UB,不要寫 if (it != begin) --it;
    //      這種看起來合理但仍會踩雷的程式碼。
    //

    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 125: Valid Palindrome
// ----------------------------------------------------------------
// 題目:給字串 s,只考慮字母與數字 (忽略大小寫),判斷是否為回文。
//      例:"A man, a plan, a canal: Panama" → true
//          "race a car" → false
//
// 為什麼這題對 BidirectionalIterator 完美:
//   * 雙指針從兩端往中間夾 — 這需要「從尾巴 --」的能力。
//   * 對 string、vector、deque 都通用 (它們都是 RandomAccess,自然涵蓋 Bidirectional)。
//   * 用 InputIterator 寫不出來 (沒有 --,無法從尾走)。
//
// 解法核心:
//   * l 從頭、r 從尾,先各自跳過非字母數字。
//   * 比較 tolower(*l) == tolower(*r),否則回 false。
//   * ++l, --r 直到相遇。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_125_valid_palindrome() {
    auto is_palindrome = [](const std::string& s) {
        auto l = s.begin();
        auto r = s.end();
        if (l == r) return true;
        --r;
        while (l < r) {
            // 注意:這裡用 < 是因為 string::iterator 是 random_access,
            // 邏輯上 ++l/--r 屬於 bidirectional 行為。
            while (l < r && !std::isalnum(*l)) ++l;
            while (l < r && !std::isalnum(*r)) --r;
            if (std::tolower(*l) != std::tolower(*r)) return false;
            ++l; if (l == r) break;
            --r;
        }
        return true;
    };

    std::cout << std::boolalpha;
    std::cout << "LC125 \"A man, a plan, a canal: Panama\" = "
              << is_palindrome("A man, a plan, a canal: Panama") << '\n';
    std::cout << "LC125 \"race a car\" = "
              << is_palindrome("race a car") << '\n';
}

// ----------------------------------------------------------------
// LeetCode 977: Squares of a Sorted Array
// ----------------------------------------------------------------
// 題目:給「已排序」陣列 nums (可能含負數),回傳「每個元素平方後再排序」的結果。
//      例:nums = [-7, -3, 2, 3, 11] → [4, 9, 9, 49, 121]
//      限制:O(n) 時間 (進階要求)。
//
// 為什麼這題對 BidirectionalIterator 完美:
//   * 「兩端往中間夾」的雙指針:負數的最大平方在最左邊、正數的最大平方在最右邊,
//     比較絕對值較大者放到 ans 尾端 — 需要 -- 的能力。
//   * 不要用 transform 平方再 sort:那是 O(n log n),浪費已排序的性質。
//
// 解法核心:
//   * l 指向 begin(),r 指向 prev(end()) (用 prev 避開 end())。
//   * 從 ans 的尾端往頭寫 — 每次取 |*l|^2 vs |*r|^2,大的放進 ans。
//   * 用 reverse_iterator 寫 ans 的「從尾往頭」更直觀。
//
// 複雜度:時間 O(n);空間 O(n) (回傳陣列)。
void leetcode_977_squares_of_sorted_array() {
    auto sorted_squares = [](const std::vector<int>& nums) {
        std::vector<int> ans(nums.size());
        auto l = nums.begin();
        auto r = std::prev(nums.end());            // 最後一個 (用 prev 避開 end())
        for (auto out = ans.rbegin(); out != ans.rend(); ++out) {
            int sq_l = (*l) * (*l);
            int sq_r = (*r) * (*r);
            if (sq_l > sq_r) { *out = sq_l; ++l; } // 較大者塞到 ans 尾端
            else             { *out = sq_r; if (l != r) --r; }
        }
        return ans;
    };
    std::vector<int> ss_in = {-7, -3, 2, 3, 11};
    auto ss = sorted_squares(ss_in);
    std::cout << "LC977 sorted squares: ";
    for (int x : ss) std::cout << x << ' ';        // 4 9 9 49 121
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:LRU 風格的「最近使用清單」走訪
// ----------------------------------------------------------------
// 場景:std::list 常被拿來當「LRU cache 的 list 部分」 — 最近用的放前面、
//      最久沒用的放後面。需求:列出「最久沒用的 N 個」,從尾巴往前掃。
//
// 重點:
//   * std::list 只有 bidirectional iterator → 沒有 it+n、沒有 it[n]。
//   * 用 --end() 取最後一個、--it 一路往前,標準的 bidirectional 用法。
//   * 也可以直接用 rbegin / rend 簡化 (見 06_reverse_iterator.cpp)。
void practical_recently_used_cache_walk() {
    // 模擬 LRU list:最前面是「最近使用」
    std::list<std::string> lru = {
        "doc-A.pdf",  // 最新
        "doc-B.pdf",
        "doc-C.pdf",
        "doc-D.pdf",
        "doc-E.pdf",  // 最舊
    };

    std::cout << "最久沒用的 2 筆 (從尾往前):\n";
    auto it2 = lru.end();
    for (int i = 0; i < 2 && it2 != lru.begin(); ++i) {
        --it2;
        std::cout << "  - " << *it2 << '\n';
    }
}

// ----------------------------------------------------------------
// LeetCode 344: Reverse String (反轉字串,in-place)
// ----------------------------------------------------------------
// 題目:把字串 (char vector) 原地反轉。例:['h','e','l','l','o'] → ['o','l','l','e','h']
//
// 為什麼這題對 BidirectionalIterator 完美:
//   * 雙指針從兩端往中間夾,每次 swap 一對,然後 ++l, --r。
//   * std::reverse 內部演算法的最低需求正是 BidirectionalIterator (要能 --)。
//   * 改用 InputIterator 沒辦法寫,因為無法從尾走。
//
// 解法:直接呼叫 std::reverse(begin, end),或手刻雙指針版本。
// 複雜度:時間 O(n)、空間 O(1)。
void leetcode_344_reverse_string() {
    std::vector<char> s{'h','e','l','l','o'};
    std::reverse(s.begin(), s.end());      // 內部就是 Bidirectional 雙指針 swap
    std::cout << "LC344 reversed: ";
    for (char c : s) std::cout << c;
    std::cout << " (期望 olleh)\n";
}

// ----------------------------------------------------------------
// 實務範例:瀏覽器歷史記錄的「上一頁 / 下一頁」
// ----------------------------------------------------------------
// 場景:瀏覽器 / 圖片檢視器要支援雙向走訪歷史記錄。用 std::list<string> 配合
// 一個 iterator 表示「目前位置」,Back / Forward 對應 -- 與 ++ — 完美對應
// BidirectionalIterator 的雙向走訪能力。
// 注意 list 的 iterator 在 insert / 部分位置 erase 後仍然有效,維護當前位置很容易。
void practical_undo_redo_browse() {
    std::list<std::string> history{"google.com", "github.com", "leetcode.com", "blog.com"};
    auto cur = std::next(history.begin(), 2);    // 目前位於 leetcode.com
    std::cout << "當前: " << *cur << '\n';
    // Back: --
    --cur;  std::cout << "Back: "    << *cur << '\n';
    --cur;  std::cout << "Back: "    << *cur << '\n';
    // Forward: ++
    ++cur;  std::cout << "Forward: " << *cur << '\n';
    ++cur;  std::cout << "Forward: " << *cur << '\n';
    // 預期: leetcode -> github -> google -> github -> leetcode
}

// === 預期輸出 (Expected output) ===
// 正向: 10 20 30 40 50
// 反向: 50 40 30 20 10
// rbegin/rend: 50 40 30 20 10
// after reverse: 50 40 30 20 10
// set max = 9 (期望 9)
// LC125 "A man, a plan, a canal: Panama" = true
// LC125 "race a car" = false
// LC977 sorted squares: 4 9 9 49 121
// 最久沒用的 2 筆 (從尾往前):
//   - doc-E.pdf
//   - doc-D.pdf
