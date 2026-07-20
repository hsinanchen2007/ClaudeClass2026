// ============================================================================
//  set.cpp — std::set 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra set.cpp -o set && ./set
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/set
//  參考 (cplusplus.com): https://cplusplus.com/reference/set/set/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::set 是「有序、不允許重複元素」的關聯容器。
//  典型實作為紅黑樹 (red-black tree) — 一種自平衡 BST。
//  元素本身就是 key,且必須是「可比較 (有 operator< 或自訂 Compare)」的型別。
//
//  ▌ 底層資料結構
//  紅黑樹節點:每個節點存一個元素 + 紅/黑顏色 + 父/左/右指標。
//  整棵樹的高度恆為 O(log n) ⇒ insert/erase/find 都是 O(log n)。
//
//  ▌ 所屬類別
//  Associative container (有序關聯容器)
//
//  ▌ 時間複雜度
//      insert / erase / find / count   O(log n)
//      lower_bound / upper_bound       O(log n)
//      size / empty                    O(1)
//      iterator ++/--                  攤銷 O(1) (走中序)
//
//  ▌ 與其他 container 的比較
//      set vs unordered_set : set 有序、O(log n);unordered_set 無序、平均 O(1)
//      set vs multiset      : set 不允許重複,multiset 允許
//      set vs vector + sort : 若資料量固定且查詢多,vector + sort + binary_search
//                             可能更快 (cache 友善)
//
//  ▌ 適用情境
//      ✅ 需要「自動排序」並去重的集合
//      ✅ 需要範圍查詢 (lower_bound / upper_bound)
//      ✅ 需要按順序走訪
//      ❌ 只關心存在性、不需要順序 → unordered_set 更快
//
//  ▌ Iterator 失效規則 ★
//      • insert     : 不影響任何既有 iterator
//      • erase      : 只有「被刪元素」的 iterator 失效
//      • clear      : 全部失效
//      ★ 這比 vector / unordered_* 都安全得多
//
// ============================================================================

/*
補充筆記：std::set
  - set 只保存 key，key 同時也是 value；元素在容器中不可直接改成會破壞排序的值。
  - 查找、插入、刪除通常是 O(log n)，並保持有序。
  - 需要快速平均查找但不需要順序時，unordered_set 可能更適合。
  - std::set 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::set
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. set 的元素為什麼不能修改？
//     答：因為元素本身就是排序依據，修改它會破壞紅黑樹的有序性，使容器內部結構不一致。
//         因此 std::set 的 iterator 與 const_iterator 都指向 const 元素（C++11 起明確規定）。
//         正確做法：erase 舊值再 insert 新值；或 C++17 起用 extract() 取出 node handle、
//         改完 value() 再 insert 回去（不需重新配置記憶體）。
//     追問：map 呢？（value_type 是 std::pair<const Key, T>——key 是 const、value 可改）
//
// 🔥 Q2. set 的底層是什麼？為什麼選 red-black tree 而不是 AVL tree？
//     答：標準只要求「有序走訪 + O(log n)」，沒有指定資料結構；主流實作都選 red-black
//         tree，find / insert / erase 都是 O(log n) worst case，且可有序走訪。
//         RB tree 平衡條件較寬鬆（最長路徑 ≤ 最短路徑的 2 倍），AVL 要求左右子樹高度差 ≤ 1；
//         所以 RB tree 在 insert / erase 時 rotation 次數較少，對插入刪除頻繁的通用容器更劃算。
//
// ⚠️ 陷阱. 對 set 做 insert 之後，舊的 iterator 會失效嗎？
//     答：不會。set / map 的 insert 完全不影響任何既有 iterator 與 reference（同 list）；
//         只有被刪除元素的 iterator / reference 會失效。
//         原因是紅黑樹 rebalance 只重接指標，節點位址不會改變。
//     為什麼會錯：把 vector「擴容就全失效」的直覺套到關聯容器。
// ═══════════════════════════════════════════════════════════════════════════

#include <set>
#include <iostream>
#include <string>
#include <vector>
#include <iterator>

template <typename T, typename C>
void print(const std::set<T, C>& s, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "{ ";
    for (const auto& e : s) std::cout << e << ' ';
    std::cout << "} (size=" << s.size() << ")\n";
}

// 對於 set<T> (預設 Compare = less<T>) 的 print 多載
template <typename T>
void print(const std::set<T>& s, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "{ ";
    for (const auto& e : s) std::cout << e << ' ';
    std::cout << "} (size=" << s.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::set<int> s1;                              // 空集合
    std::set<int> s2{3, 1, 4, 1, 5, 9, 2, 6};      // 自動排序 + 去重
    std::set<int> s3(s2.begin(), s2.end());        // iterator range
    std::set<int> s4(s2);                          // copy
    std::set<int> s5(std::move(s4));               // move
    std::set<int, std::greater<int>> s6{1,2,3};    // 自訂比較 (降冪)

    print(s2, "s2 (自動排序去重)   ");
    print(s5, "s5 (move 來)        ");
    print(s6, "s6 (greater)        ");

    // ========================================================================
    //  2. 元素存取 ★set 沒有 operator[] / at !
    // ========================================================================
    //  set 的「元素 = key」,且 set 中元素是 const,不能修改 (會破壞排序)。
    //  想取得元素 → 用 find / lower_bound 等。

    // ========================================================================
    //  3. Iterators
    // ========================================================================
    //  bidirectional iterator (不能隨機存取),走訪順序為「升冪」(用 less)。
    //  set 的 iterator 解 reference 後是 const T& — 不能修改元素值。

    std::cout << "\n[正向] ";
    for (auto it = s2.begin(); it != s2.end(); ++it) std::cout << *it << ' ';
    std::cout << "\n[反向] ";
    for (auto it = s2.rbegin(); it != s2.rend(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // ========================================================================
    //  4. 容量
    // ========================================================================
    std::cout << "\n[Capacity] size=" << s2.size()
              << ", empty=" << std::boolalpha << s2.empty() << '\n';

    // max_size():紅黑樹節點理論上限
    std::cout << "max_size = " << s2.max_size() << '\n';

    // get_allocator()
    auto sa = s2.get_allocator();
    (void)sa;
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  5. Modifiers
    // ========================================================================

    // ──── insert ────
    // 回傳 pair<iterator, bool>:bool 表示是否「真的插入」(false = 已存在)
    std::set<int> m;
    auto [it, ok] = m.insert(10);
    std::cout << "\ninsert 10 → ok=" << ok << ", *it=" << *it << '\n';
    auto [it2, ok2] = m.insert(10);   // 第二次插入,ok=false
    std::cout << "insert 10 again → ok=" << ok2 << '\n';

    m.insert({1, 2, 3, 4});
    print(m, "after insert(list)  ");

    // 帶 hint 的 insert (有效 hint 可以加速到 O(1) 攤銷)
    m.insert(m.end(), 5);              // 提示:5 應該放在最後
    print(m, "insert with hint    ");

    // ──── emplace ────
    // 直接以建構參數在容器內建構,避免一次拷貝
    std::set<std::string> ms;
    ms.emplace(5, 'A');                // string(5,'A') = "AAAAA"
    ms.emplace("hello");
    print(ms, "set<string> emplace ");

    // ──── erase ────
    // 三種多載:by iterator / by key / by range
    // by key 回傳「刪了幾個」(對 set 而言 0 或 1)
    std::set<int> e2{1,2,3,4,5};
    e2.erase(3);                       // by key
    e2.erase(e2.begin());              // by iterator
    print(e2, "after erase         ");

    // ──── extract / merge (C++17) ────
    // ★ 真正的「節點移交」,完全不會拷貝/搬移元素內容
    std::set<int> e3{1,2,3};
    auto node = e3.extract(2);         // 把 2 從 e3 取出 → e3 = {1,3}
    print(e3, "after extract(2)    ");
    if (!node.empty()) {
        // node.value() 可改寫 key (因為已不在容器中,不會破壞順序)
        node.value() = 20;
        e3.insert(std::move(node));
        print(e3, "re-insert as 20     ");
    }

    // merge: 把 other 中所有「目前 *this 沒有」的元素搬過來
    std::set<int> a{1,2,3}, b{2,3,4,5};
    a.merge(b);                        // a 多了 4, 5;b 剩下 2, 3
    print(a, "a after merge       ");
    print(b, "b (留下的)          ");

    // swap: O(1)
    std::set<int> sw1{1,2}, sw2{9,8};
    sw1.swap(sw2);
    print(sw1, "swap                ");

    // clear
    std::set<int> cl{1,2,3};
    cl.clear();
    print(cl, "after clear         ");

    // ========================================================================
    //  6. Lookup (查詢)
    // ========================================================================

    std::set<int> q{1, 3, 5, 7, 9};

    // ──── count(key) ────  set 中只能 0 或 1
    std::cout << "\ncount(3) = " << q.count(3)
              << ", count(4) = " << q.count(4) << '\n';

    // ──── find(key) ──── 回傳 iterator,找不到回 end()
    auto fi = q.find(5);
    if (fi != q.end()) std::cout << "find(5) → " << *fi << '\n';

    // ──── contains (C++20) ────  比 count 更直觀
    std::cout << "contains(7) = " << q.contains(7)
              << ", contains(8) = " << q.contains(8) << '\n';

    // ──── equal_range(key) ──── 回傳 [first, last) 的 pair
    auto [lo, hi] = q.equal_range(5);
    std::cout << "equal_range(5): ";
    for (auto it_ = lo; it_ != hi; ++it_) std::cout << *it_ << ' ';
    std::cout << '\n';

    // ──── lower_bound / upper_bound ────
    // lower_bound(k) : 第一個 ≥ k 的位置
    // upper_bound(k) : 第一個 > k 的位置
    auto lb = q.lower_bound(4);
    auto ub = q.upper_bound(7);
    std::cout << "[lb=4 .. ub=7): ";
    for (auto it_ = lb; it_ != ub; ++it_) std::cout << *it_ << ' ';
    std::cout << '\n';

    // ========================================================================
    //  7. Observers (觀察器)
    // ========================================================================
    //  key_comp() / value_comp(): 取得比較函式 (對 set 而言兩者相同)
    auto cmp = q.key_comp();
    std::cout << "\nkey_comp(3,5) = " << cmp(3, 5) << '\n';

    // ========================================================================
    //  8. 自訂 Compare
    // ========================================================================
    // 比較函式必須提供「strict weak ordering」 — 對等元素只能是「!cmp(a,b) && !cmp(b,a)」
    struct ByLength {
        bool operator()(const std::string& a, const std::string& b) const {
            if (a.size() != b.size()) return a.size() < b.size();
            return a < b;
        }
    };
    std::set<std::string, ByLength> by_len{"banana", "apple", "fig", "kiwi"};
    print(by_len, "by length           ");

    // ========================================================================
    //  9. C++14 透明比較 (transparent comparator) is_transparent
    // ========================================================================
    // 用 std::less<> (沒指定型別) 可以直接用「不同型別」做查詢,
    // 不必先建立 std::string 物件。
    std::set<std::string, std::less<>> trans{"apple", "banana"};
    auto t_it = trans.find("apple");      // const char* 直接查 — 不必先轉 string
    if (t_it != trans.end())
        std::cout << "transparent find: " << *t_it << '\n';

    // ========================================================================
    //  10. C++20 free functions
    // ========================================================================
    std::set<int> n1{1,2,3,4,5,6};
    std::erase_if(n1, [](int x){ return x % 2 == 0; });
    print(n1, "erase_if(even)      ");

    // ========================================================================
    //  11. 常見陷阱 (Pitfalls)
    // ========================================================================
    //
    //  (1) set 的元素是 const
    //      *it = newValue;   // 編譯錯誤 — 會破壞排序
    //      要修改 → 先 extract 再 modify 再 insert,或 erase 後重新 insert。
    //
    //  (2) 沒有 operator[]
    //      想知道有沒有 key → contains() (C++20) 或 count() == 1。
    //
    //  (3) Compare 必須 strict weak ordering
    //      若 cmp(a,b) 與 cmp(b,a) 都 false 表示「等價」(equivalent),
    //      set 視為同一元素。
    //
    //  (4) 自訂型別要記得提供 operator< 或自訂 Compare
    //      否則編譯失敗。
    //
    //  (5) iterator 是 bidirectional,不能 it+3
    //      用 std::next(it, 3) 替代 (但仍是 O(n))。
    //
    //  (6) extract / merge 要兩個容器使用「相同 Compare」
    //      否則 UB。

    // ========================================================================
    //  12. 最佳實踐
    // ========================================================================
    //
    //  • 需要「有序」集合 → set
    //  • 不在乎順序 → unordered_set 通常更快
    //  • 範圍查詢 (k1 ~ k2) → set 的 lower_bound + upper_bound 是利器
    //  • 大量批次插入後不再修改 → 考慮 sorted vector,cache 命中率更好
    //  • 想避免重複建構 string → 用 std::less<> 透明比較
    //  • 想合併兩個 set 而不複製元素 → merge() / extract() (C++17)

    // ========================================================================
    //  13. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // set 的最大優勢是「自帶排序 + lower_bound/upper_bound」, 在面試題裡常用於
    // 「找最近值」、「動態維護排序集合」、「掃描線問題 (sweep line)」。

    // ──── LC 220: Contains Duplicate III (附近重複元素 III) ────
    // 給陣列 nums,問是否存在 i, j 使得 |nums[i]-nums[j]| <= valueDiff 且
    // |i-j| <= indexDiff。經典「滑動視窗 + 有序集合」題。
    // 思路: 用 set 維護視窗內元素,新元素 x 插入前用 lower_bound(x - valueDiff)
    //        找最接近的;若該值與 x 之差不超過 valueDiff 即可。
    {
        auto contains_nearby_almost_dup =
            [](const std::vector<int>& nums, int indexDiff, int valueDiff) {
            std::set<long long> window;
            for (int i = 0; i < (int)nums.size(); ++i) {
                auto it = window.lower_bound((long long)nums[i] - valueDiff);
                if (it != window.end() && *it - nums[i] <= valueDiff) return true;
                window.insert(nums[i]);
                if ((int)window.size() > indexDiff) {
                    window.erase(nums[i - indexDiff]);
                }
            }
            return false;
        };
        std::cout << "\n[LC220 NearbyAlmostDup] ";
        std::cout << std::boolalpha
                  << contains_nearby_almost_dup({1,2,3,1}, 3, 0) << ' ';     // true
        std::cout << contains_nearby_almost_dup({1,5,9,1,5,9}, 2, 3) << '\n';// false
    }

    // ──── LC 729: My Calendar I (我的日程表 I) ────
    // 動態插入區間 [start, end),若與已存在區間重疊則拒絕。
    // 用 set<pair<int,int>> 排序,二分找前一個區間判斷是否重疊。
    //
    // ★ emplace_hint 應用:既然已用 lower_bound 找到「正確插入位置」,直接把
    //    該 iterator 當 hint 給 emplace_hint,可達「攤銷 O(1)」插入
    //    (vs emplace 的 O(log n))。這是 emplace_hint 最自然的 LC 套路 —
    //    凡是「lower_bound + insert」的題型都可這樣優化。
    {
        struct MyCalendar {
            std::set<std::pair<int,int>> book;   // 以 start 排序
            bool book_event(int start, int end) {
                auto it = book.lower_bound({start, end});
                // 檢查右鄰: it 的開始是否 < end
                if (it != book.end() && it->first < end) return false;
                // 檢查左鄰: 它的結束是否 > start
                if (it != book.begin() && std::prev(it)->second > start) return false;
                book.emplace_hint(it, start, end);     // ← hint = 正確位置 → 攤銷 O(1)
                return true;
            }
        };
        MyCalendar cal;
        std::cout << "[LC729 Calendar] " << std::boolalpha;
        std::cout << cal.book_event(10, 20) << ' ';   // true
        std::cout << cal.book_event(15, 25) << ' ';   // false (與 10~20 重疊)
        std::cout << cal.book_event(20, 30) << '\n';  // true
    }

    // ──── LC 217: Contains Duplicate (是否有重複元素) ────
    // 給整數陣列,判斷是否存在任何元素出現兩次。
    // 思路:邊掃描邊插入 set,若 insert 回傳的 pair.second 為 false,代表重複。
    // 為何用 set:這裡示範有序 set 解法;若不需排序,unordered_set 平均 O(n) 更快。
    {
        auto contains_duplicate = [](const std::vector<int>& nums) {
            std::set<int> seen;
            for (int x : nums) {
                // insert 回傳 pair<iterator,bool>:bool = false 代表 key 已存在
                if (!seen.insert(x).second) return true;
            }
            return false;
        };
        std::cout << "[LC217 ContainsDup] " << std::boolalpha
                  << contains_duplicate({1,2,3,1}) << ' '         // true
                  << contains_duplicate({1,2,3,4}) << '\n';       // false
    }

    // ========================================================================
    //  14. 實戰範例:IP 黑名單 + 區間封鎖 (Firewall Rule)
    // ========================================================================
    // 真實場景:防火牆要維護動態的「禁用 IP 集合」並支援:
    //   • 新增單一 IP → insert,O(log n)
    //   • 查詢 IP 是否在黑名單 → contains / find,O(log n)
    //   • 列出某 IP 區段內所有被封鎖 IP → lower_bound + upper_bound,O(log n + k)
    // set 比 unordered_set 在此勝出,因為要做「區段查詢」。
    {
        std::set<int> blocked_ips{
            192168001, 192168005, 192168010, 192168020, 192168033, 192168077
        };
        // 查詢 192.168.0.5~192.168.0.30 區段的封鎖 IP
        auto lo = blocked_ips.lower_bound(192168005);
        auto hi = blocked_ips.upper_bound(192168030);
        std::cout << "[Firewall] 區段內封鎖 IP: ";
        for (auto it = lo; it != hi; ++it) std::cout << *it << ' ';
        std::cout << '\n';
        // 預期輸出: 192168005 192168010 192168020
    }

    std::cout << "\n=== set demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：set 與 unordered_set 怎麼選?
    //    A：要「自動排序、區間查詢 (lower_bound/upper_bound)」就用 set,O(log n);
    //       單純判斷成員存在、計次去重就用 unordered_set,O(1) 平均。LC729 行事曆
    //       這類「找最接近的元素」題目,set 是最自然的解法。
    //
    //  Q2：為何 set 的 iterator 解參考是 const T&,不能修改元素?
    //    A：set 內部用 key 排序維護紅黑樹結構,若允許直接改 key 會破壞排序。
    //       要修改必須先 erase 再 insert,或 C++17 後用 extract 取得 node、
    //       改 value 再 insert(node) 回去 (零拷貝、零配置)。
    //
    //  Q3：lower_bound(x) 與 upper_bound(x) 差在哪?
    //    A：lower_bound(x) 回傳「第一個 ≥ x」的 iterator;upper_bound(x) 回傳
    //       「第一個 > x」的 iterator。equal_range(x) 等價於 {lower, upper}。
    //       這三個都是 O(log n),搜尋區間時非常實用。
    //
    return 0;
}

/*
============================================================================
  附錄:std::set 完整 member function 一覽
============================================================================
  Constructors / destructor / operator= / get_allocator

  Iterators:       begin/end/cbegin/cend/rbegin/rend/crbegin/crend
  Capacity:        empty, size, max_size

  Modifiers:       clear, insert, insert_range (C++23), emplace, emplace_hint,
                   erase, swap,
                   extract, merge          (C++17)

  Lookup:          count, find, contains (C++20), equal_range,
                   lower_bound, upper_bound

  Observers:       key_comp, value_comp

  Non-member:      operator==, <=> (C++20),
                   std::swap, std::erase_if (C++20)
============================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra set.cpp -o set

// === 預期輸出 (節錄) ===
// s2 (自動排序去重)   : { 1 2 3 4 5 6 9 } (size=7)
// s5 (move 來)        : { 1 2 3 4 5 6 9 } (size=7)
// s6 (greater)        : { 3 2 1 } (size=3)
//
// [正向] 1 2 3 4 5 6 9
// [反向] 9 6 5 4 3 2 1
//
// [Capacity] size=7, empty=false
// max_size = 461168601842738790
// get_allocator() OK
//
// insert 10 → ok=true, *it=10
// insert 10 again → ok=false
// after insert(list)  : { 1 2 3 4 10 } (size=5)
// insert with hint    : { 1 2 3 4 5 10 } (size=6)
// set<string> emplace : { AAAAA hello } (size=2)
// after erase         : { 2 4 5 } (size=3)
// after extract(2)    : { 1 3 } (size=2)
// re-insert as 20     : { 1 3 20 } (size=3)
// a after merge       : { 1 2 3 4 5 } (size=5)
// …（後略，完整輸出共 41 行）
