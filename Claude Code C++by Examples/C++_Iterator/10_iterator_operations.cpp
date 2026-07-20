// =============================================================================
//  10_iterator_operations.cpp  —  iterator 工具函式
// =============================================================================
//  在 <iterator> 標頭中，STL 提供了幾個泛型函式來操作 iterator：
//
//   std::advance(it, n)
//      * 把 it 前進 n 步 (n 可為負數但要 BidirectionalIterator 以上)。
//      * 對 RandomAccessIterator 是 O(1)，對其他類別 O(|n|)。
//      * 沒有回傳值！直接修改 it 本身。
//
//   std::distance(first, last)
//      * 回傳兩個 iterator 距離 (difference_type)。
//      * RandomAccess: O(1) (直接相減)；其他: O(n) 邊走邊數。
//
//   std::next(it, n=1)              [C++11]
//      * 回傳 it 前進 n 步後的副本，不修改 it。
//
//   std::prev(it, n=1)              [C++11]
//      * 回傳 it 後退 n 步後的副本 (要 Bidirectional)。
//
//   std::begin(c) / std::end(c)     [C++11]
//      * 對容器或原生陣列都管用，常拿來寫泛型程式：
//          template <class C> void foo(C& c) {
//              for (auto it = std::begin(c); it != std::end(c); ++it) ...
//          }
//
//   std::cbegin / std::cend         [C++14]   ← 強制回傳 const_iterator
//   std::rbegin / std::rend         [C++14]
//   std::crbegin / std::crend       [C++14]
//
//  小提醒：使用 std::next/prev 不會破壞原本的 it，比 advance 更適合「鏈式」表達。
//
//  參考連結 (cppreference / cplusplus)：
//    https://en.cppreference.com/w/cpp/iterator/advance       — advance
//    https://en.cppreference.com/w/cpp/iterator/distance      — distance
//    https://en.cppreference.com/w/cpp/iterator/next          — next (C++11)
//    https://en.cppreference.com/w/cpp/iterator/prev          — prev (C++11)
//    https://en.cppreference.com/w/cpp/iterator/begin         — begin/cbegin
//    https://en.cppreference.com/w/cpp/iterator/end           — end/cend
//    https://en.cppreference.com/w/cpp/iterator/rbegin        — rbegin/crbegin
//    https://en.cppreference.com/w/cpp/iterator/size          — size (C++17)
//    https://en.cppreference.com/w/cpp/iterator/empty         — empty (C++17)
//    https://en.cppreference.com/w/cpp/iterator/data          — data (C++17)
//    https://en.cppreference.com/w/cpp/algorithm/iter_swap    — iter_swap
//    https://en.cppreference.com/w/cpp/algorithm/swap_ranges  — swap_ranges
// =============================================================================

/*
補充筆記：iterator_operations
  - iterator_operations 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - std::advance 會依 iterator category 選擇前進方式；random access 是 O(1)，list 則要一步步走。
  - std::distance 對 random access 可直接相減，對 input/forward 可能需要走完整段。
  - next/prev 回傳移動後的新 iterator，不修改原 iterator，適合寫更安全的邊界邏輯。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::distance / advance / next / prev
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::distance 和 std::advance 的複雜度是多少？
//     答：依 iterator category 分派（tag dispatch）：random access 是 O(1)（直接
//         last - first 或 it += n），其他 category 是 O(n)（必須逐步 ++）。這正是
//         category 存在的意義——同一個泛型介面在不同容器上取得各自最佳的實作。
//         std::next / std::prev（C++11）是回傳新 iterator 的非變動版本，複雜度同 advance。
//     追問：為什麼不能對 list::iterator 寫 it + 5？（bidirectional 沒有 operator+，編譯
//         錯誤；要用 std::advance 或 std::next）／advance 可以傳負數嗎？（bidirectional
//         以上可以，input iterator 不行）
//
// 🔥 Q2. ++it 和 it++ 有什麼差別？為什麼建議用 ++it？
//     答：++it 直接遞增並回傳自身的 reference；it++ 必須先複製一份舊值、遞增自身、再回傳
//         那個副本。對裸指標編譯器能把差異優化掉，但對 map 的 tree iterator 這類 class
//         型 iterator，會多出一次 copy construct 加 destruct。不需要舊值時一律用 ++it。
//     追問：post-increment 怎麼宣告？（It operator++(int)，那個 int 是用來區分 overload
//         的 dummy 參數）
//
// ⚠️ 陷阱. v.end() - 1 和 --v.end() 都合法嗎？
//     答：對 vector（random access）v.end() - 1 合法；但 --v.end() 的可攜性依實作而定：
//         end() 回傳的是 rvalue，若該實作的 iterator 是裸指標型別，對 rvalue 用內建 --
//         不合法；若是 class 型別，成員 operator-- 對 rvalue 可以呼叫。應改用
//         std::prev(v.end())。另外對空容器兩者都是 UB。
//     為什麼會錯：在自己的編譯器上剛好能編過，就以為是標準保證的寫法。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <vector>
#include <list>
#include <algorithm>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50, 60, 70};

    // -----------------------------------------------------------------------
    // 範例 1：advance / distance / next / prev
    // -----------------------------------------------------------------------
    auto it = v.begin();
    std::advance(it, 3);                      // 前進 3 → 指到 40
    std::cout << "advance 3 → " << *it << " (期望 40)\n";

    auto it2 = std::next(v.begin(), 5);       // 不破壞原 it
    std::cout << "next(begin,5) → " << *it2 << " (期望 60)\n";

    auto it3 = std::prev(v.end(), 2);         // 倒數第 2 個
    std::cout << "prev(end,2) → " << *it3 << " (期望 60)\n";

    auto d = std::distance(v.begin(), v.end());
    std::cout << "distance = " << d << '\n';

    // -----------------------------------------------------------------------
    // 範例 2：對 list (Bidirectional) 取「中間元素」
    //   list 沒有 random access，不能 list[i]，必須用 advance。
    // -----------------------------------------------------------------------
    std::list<int> l = {1, 2, 3, 4, 5, 6, 7};
    auto mid = l.begin();
    std::advance(mid, l.size() / 2);          // 走到中間 → O(n)
    std::cout << "list 中位 = " << *mid << " (期望 4)\n";

    // -----------------------------------------------------------------------
    // 範例 3：std::begin / std::end 對原生陣列也通用
    // -----------------------------------------------------------------------
    int arr[] = {7, 8, 9};
    std::cout << "arr 總和 = ";
    int sum = 0;
    for (auto p = std::begin(arr); p != std::end(arr); ++p) sum += *p;
    std::cout << sum << '\n';

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 1：
    //   LC 876. Middle of the Linked List
    //   (用 advance 取中位 — 也對應「快慢指針」的另一種寫法)
    // -----------------------------------------------------------------------
    auto find_middle = [](std::list<int>& nodes) {
        auto m = nodes.begin();
        // 偶數時取後半段第一個 (LeetCode 規定)
        std::advance(m, nodes.size() / 2);
        return m;
    };
    std::list<int> ll = {1, 2, 3, 4, 5};
    std::cout << "middle = " << *find_middle(ll) << " (期望 3)\n";

    std::list<int> ll2 = {1, 2, 3, 4, 5, 6};
    std::cout << "middle = " << *find_middle(ll2) << " (期望 4)\n";

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 2：
    //   LC 26. Remove Duplicates from Sorted Array
    //   std::unique 回傳「新邏輯結尾」，搭 distance 算新長度。
    // -----------------------------------------------------------------------
    std::vector<int> nums = {1, 1, 2, 2, 3, 4, 4, 5};
    auto new_end = std::unique(nums.begin(), nums.end());
    auto len = std::distance(nums.begin(), new_end);   // ← distance 出場
    std::cout << "unique 後長度 = " << len << " (期望 5)\n";
    std::cout << "前 " << len << " 項: ";
    for (auto p = nums.begin(); p != new_end; ++p) std::cout << *p << ' ';
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 4：cbegin / cend / rbegin / rend / crbegin / crend
    //   * c 系列強制回傳 const_iterator (寫起來不能改元素)，常用在「我只想讀」
    //     的泛型函式 — 表達意圖、避免誤改。
    //   * r 系列已在檔案 06 詳述，這裡只做整合速查。
    //   * c+r 組合 (crbegin/crend) = 反向且唯讀。
    // -----------------------------------------------------------------------
    std::vector<int> ro = {1, 2, 3};
    for (auto it = std::cbegin(ro); it != std::cend(ro); ++it)
        std::cout << "cbegin 走訪: " << *it << '\n';
    // *it = 99;  // ← 這行若打開會編譯錯：const_iterator 不可寫
    for (auto it = std::crbegin(ro); it != std::crend(ro); ++it)
        std::cout << "crbegin 走訪 (反向且唯讀): " << *it << '\n';

    // -----------------------------------------------------------------------
    // 範例 5：std::size / std::empty / std::data  (C++17)
    //   * 全部都是泛型工具：可吃 STL 容器，也可吃原生陣列。
    //   * std::data(c) 回傳指向「底層連續記憶體」的指標，僅對連續容器有意義
    //     (vector / string / array / 原生陣列)。
    // -----------------------------------------------------------------------
    int arr2[] = {7, 8, 9, 10};
    std::cout << "std::size(arr2)  = " << std::size(arr2)  << " (4)\n";
    std::cout << "std::empty(arr2) = " << std::boolalpha
              << std::empty(arr2) << " (false)\n";
    int* raw = std::data(ro);                      // == ro.data()
    std::cout << "std::data(ro)[0] = " << raw[0] << " (1)\n";

    // -----------------------------------------------------------------------
    // 範例 6：std::iter_swap / std::swap_ranges
    //   * iter_swap(a, b)  交換兩個 iterator 「指向」的元素 (不是 iterator 本身)。
    //   * swap_ranges(a1, a2, b1)  交換兩個等長序列 [a1,a2) 與 [b1, b1+(a2-a1))。
    //   * 兩者只需要 ForwardIterator 等級。
    // -----------------------------------------------------------------------
    std::vector<int> A = {1, 2, 3, 4};
    std::vector<int> B = {9, 8, 7, 6};
    std::iter_swap(A.begin(), B.begin());          // 交換 A[0] 與 B[0]
    std::cout << "after iter_swap: A[0]=" << A[0]
              << " B[0]=" << B[0] << " (期望 9, 1)\n";

    std::swap_ranges(A.begin(), A.begin() + 3,     // [1..3) 與 B 開頭三個交換
                     B.begin());
    std::cout << "after swap_ranges: A=";
    for (int x : A) std::cout << x << ' ';
    std::cout << " B=";
    for (int x : B) std::cout << x << ' ';
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::advance 跟 std::next 用法有什麼差別？
    //    A：std::advance(it, n) 直接「修改 it 本身」前進 n 步，沒有回傳值；
    //      std::next(it, n=1) 回傳「it 前進 n 步後的副本」，不影響 it。
    //      想保留原始 it 又得到新位置請用 std::next；只需要往前推進當前 it 用
    //      advance。注意兩者對 RandomAccess 都 O(1)，對其他類別 O(n)。
    //
    //  Q2：cbegin / cend (C++14) 跟在 const 容器上呼叫 begin() 差在哪？
    //    A：在 const 容器上 begin() 也會回傳 const_iterator，所以行為相同。但對
    //      非 const 容器，c.begin() 會回傳普通 iterator (可寫)，而 std::cbegin(c)
    //      強制回傳 const_iterator (唯讀)。寫泛型「我只想讀」的程式時用 cbegin
    //      表達意圖最清楚，也能讓編譯器擋掉誤改。
    //
    //  Q3：std::iter_swap 跟 std::swap 哪裡不同？
    //    A：std::swap(a, b) 交換兩個「物件」；std::iter_swap(it1, it2) 交換兩個
    //      iterator「指向的物件」(等同 swap(*it1, *it2))。寫排序、reverse、
    //      partition 之類的演算法時，操作的是 iterator，用 iter_swap 語意最直接，
    //      也對「proxy reference」(如 vector<bool>::iterator) 行為正確 —
    //      直接 swap(*it1, *it2) 對 proxy 可能編譯失敗或語意錯誤。
    //

    // -----------------------------------------------------------------------
    // LC 範例: LC 1213 — Intersection of Three Sorted Arrays (三排序陣列交集)
    // -----------------------------------------------------------------------
    // 給三個「已排序」陣列,回傳「三個都有」的元素。
    // 思路:三指針同步前進,std::next / std::distance 取代手寫迴圈索引,程式碼更乾淨。
    // 為何在這檔示範:三指針題完美展示「iterator 工具函式」的價值 —
    // 用 std::next + 比較替代 i++ / j++ 的索引算術,泛型且不限定容器型別 (vector / deque 都行)。
    {
        std::vector<int> a{1, 2, 3, 4, 5}, b{1, 2, 5, 7, 9}, c{1, 3, 4, 5, 8};
        auto i = a.begin(), j = b.begin(), k = c.begin();
        std::vector<int> ans;
        while (i != a.end() && j != b.end() && k != c.end()) {
            if (*i == *j && *j == *k) {
                ans.push_back(*i);
                ++i; ++j; ++k;
            } else {
                int mn = std::min({*i, *j, *k});
                if (*i == mn) ++i;
                if (*j == mn) ++j;
                if (*k == mn) ++k;
            }
        }
        std::cout << "LC1213 intersect3 = [ ";
        for (int x : ans) std::cout << x << ' ';
        std::cout << "] (期望 1 5)\n";
    }

    // -----------------------------------------------------------------------
    // 實戰範例:資料庫 cursor 風格的「跳頁 + 取片段」
    // -----------------------------------------------------------------------
    // 場景:對 std::list 做「跳到第 N 筆,取接下來 K 筆」 — 經典 cursor 用法。
    // 因為 list 是 bidirectional,no random access,只能用 std::advance / std::next
    // 一步一步走。這展示了「iterator 操作函式」在不同 iterator 等級下都通用的價值。
    {
        std::list<int> data{10,20,30,40,50,60,70,80,90,100};
        int offset = 3, limit = 4;
        auto first = std::next(data.begin(), offset);   // 跳到 index 3
        auto last  = std::next(first,
                               std::min<int>(limit, std::distance(first, data.end())));
        std::cout << "Cursor offset=" << offset << ",limit=" << limit << ": [ ";
        for (auto it = first; it != last; ++it) std::cout << *it << ' ';
        std::cout << "]\n";
        // 預期輸出: [ 40 50 60 70 ]
    }

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 10_iterator_operations.cpp -o 10_iterator_operations

// === 預期輸出 ===
// advance 3 → 40 (期望 40)
// next(begin,5) → 60 (期望 60)
// prev(end,2) → 60 (期望 60)
// distance = 7
// list 中位 = 4 (期望 4)
// arr 總和 = 24
// middle = 3 (期望 3)
// middle = 4 (期望 4)
// unique 後長度 = 5 (期望 5)
// 前 5 項: 1 2 3 4 5
// cbegin 走訪: 1
// cbegin 走訪: 2
// cbegin 走訪: 3
// crbegin 走訪 (反向且唯讀): 3
// crbegin 走訪 (反向且唯讀): 2
// crbegin 走訪 (反向且唯讀): 1
// std::size(arr2)  = 4 (4)
// std::empty(arr2) = false (false)
// std::data(ro)[0] = 1 (1)
// after iter_swap: A[0]=9 B[0]=1 (期望 9, 1)
// after swap_ranges: A=1 8 7 4  B=9 2 3 6
// LC1213 intersect3 = [ 1 5 ] (期望 1 5)
// Cursor offset=3,limit=4: [ 40 50 60 70 ]
