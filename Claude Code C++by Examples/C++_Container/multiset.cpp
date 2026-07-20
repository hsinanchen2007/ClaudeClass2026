// ============================================================================
//  multiset.cpp — std::multiset 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra multiset.cpp -o multiset && ./multiset
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/multiset
//  參考 (cplusplus.com): https://cplusplus.com/reference/set/multiset/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::multiset 與 std::set 幾乎完全相同,差別只有一個:
//      ★ multiset「允許」重複元素。
//  仍然有序、底層仍是紅黑樹。
//
//  ▌ 與 std::set 的差異
//      • insert 永遠成功,回傳 iterator (而非 pair<iterator,bool>)
//      • count(k) 可能 > 1
//      • erase(k) 會刪除「所有」等於 k 的元素,回傳刪除個數
//      • equal_range(k) 真正有意義 (set 中只會是 0 或 1 個元素)
//
//  ▌ 時間複雜度
//      insert / erase(by iterator)       O(log n)
//      erase(by key)                     O(log n + k)  k 為相等元素數
//      find / lower_bound / upper_bound  O(log n)
//      count                             O(log n + k)
//
//  ▌ 適用情境
//      ✅ 需要排序 + 允許重複元素 (e.g. 直方圖、優先佇列的可走訪版本)
//      ✅ 想要範圍查詢 + 重複元素都能保留
//      ❌ 不需要順序但允許重複 → unordered_multiset
//
//  ▌ Iterator 失效規則
//      與 set 完全相同 — insert 不影響任何 iterator;
//      erase 只影響被刪元素自己的 iterator。
//
// ============================================================================

/*
補充筆記：std::multiset
  - multiset 允許重複 key，count 可能大於 1。
  - erase(value) 會刪除所有等於 value 的元素；只刪一個時要先 find 再 erase(iterator)。
  - equal_range 是處理同值群組的主要工具。
  - std::multiset 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::multiset
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. multiset 和 set 的差別？
//     答：multiset 允許重複元素，insert 永遠成功並回傳 iterator（非 pair<iterator,bool>）；
//         count(key) 可能 > 1，equal_range(key) 才真正有意義。
//         兩者底層同樣是 red-black tree，insert / find / erase(by iterator) 都是 O(log n)，
//         但 count(key) 與 erase(key) 是 O(log n + k)（k 為等值元素個數）。
//     追問：C++11 對等值元素的順序有保證嗎？（有，維持插入順序）
//
// ⚠️ 陷阱 1. multiset::erase(key) 會刪掉幾個？
//     答：全部等值元素，回傳刪除個數。「只刪一個」必須寫 erase(ms.find(key))。
//     為什麼會錯：直覺以為 erase 是「刪一筆」，對允許重複的容器而言它是「刪一組」。
//
// ⚠️ 陷阱 2. 可以透過 iterator 修改 multiset 的元素嗎？
//     答：不行。元素本身就是排序依據，修改會破壞紅黑樹的有序性，
//         因此 iterator 與 const_iterator 都指向 const 元素。
//         要改就 erase 舊值再 insert 新值，或 C++17 起用 extract() 取出 node handle、改完再 insert 回去。
// ═══════════════════════════════════════════════════════════════════════════

#include <set>            // multiset 與 set 共用 <set>
#include <iostream>
#include <string>
#include <vector>         // for LeetCode 範例
#include <iterator>       // std::next

template <typename T, typename C = std::less<T>>
void print(const std::multiset<T, C>& s, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "{ ";
    for (const auto& e : s) std::cout << e << ' ';
    std::cout << "} (size=" << s.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子 (與 set 相同介面)
    // ========================================================================
    std::multiset<int> m1;
    std::multiset<int> m2{3, 1, 4, 1, 5, 9, 2, 6, 5, 3};   // 注意:重複元素都會保留
    std::multiset<int> m3(m2.begin(), m2.end());
    std::multiset<int> m4(m2);
    std::multiset<int, std::greater<int>> m5{1, 2, 2, 3};

    print(m2, "m2 (含重複)         ");
    print(m5, "m5 (greater)        ");

    // ========================================================================
    //  2. Iterators
    // ========================================================================
    //  與 set 一樣是 bidirectional,順序為比較函式定義的順序。
    //  相等元素的相對順序在 C++11 後保證為「插入順序」(stable)。
    std::cout << "\n[正向] ";
    for (auto it = m2.begin(); it != m2.end(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // ========================================================================
    //  3. 容量
    // ========================================================================
    std::cout << "\n[Capacity] size=" << m2.size()
              << ", empty=" << std::boolalpha << m2.empty() << '\n';

    // max_size():理論元素上限
    std::cout << "max_size = " << m2.max_size() << '\n';

    // get_allocator()
    auto msa = m2.get_allocator();
    (void)msa;
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  4. Modifiers
    // ========================================================================

    // ──── insert ──── 永遠成功,回傳 iterator (★ 與 set 不同)
    std::multiset<int> m;
    auto it = m.insert(10);
    std::cout << "\ninsert 10 → *it=" << *it << '\n';
    m.insert(10);                         // 第二個 10 也會被加入
    m.insert({1, 2, 2, 3, 3, 3});
    print(m, "after insert        ");

    // 帶 hint
    m.insert(m.end(), 4);
    print(m, "insert hint(end,4)  ");

    // ──── emplace / emplace_hint ────
    std::multiset<std::string> ms;
    ms.emplace(5, 'A');
    ms.emplace(5, 'A');                   // 兩個 "AAAAA" 都會存
    ms.emplace_hint(ms.end(), "Z");
    std::cout << "ms: { ";
    for (const auto& s : ms) std::cout << s << ' ';
    std::cout << "}\n";

    // ──── erase ────
    // by key:刪除「所有」等於 key 的元素,回傳刪除數量
    std::multiset<int> e{1,2,2,2,3,4,5};
    std::cout << "\nerase(2) → 刪了 " << e.erase(2) << " 個\n";
    print(e, "after erase(2)      ");

    // by iterator:只刪這一個
    e.erase(e.begin());
    print(e, "after erase(begin)  ");

    // by range
    auto [lo, hi] = e.equal_range(4);
    e.erase(lo, hi);                      // 用 equal_range 配合刪除某個 key 的「一部分」
    print(e, "after erase(range)  ");

    // ──── extract / merge (C++17) ────
    std::multiset<int> a{1,2,3}, b{2,3,4};
    a.merge(b);                           // multiset 的 merge 不會去重 — 全部搬過來
    print(a, "a after merge       ");
    print(b, "b (掏空)            ");

    // swap / clear
    std::multiset<int> sw1{1,2}, sw2{9,8};
    sw1.swap(sw2);
    print(sw1, "after swap          ");
    sw1.clear();
    print(sw1, "after clear         ");

    // ========================================================================
    //  5. Lookup
    // ========================================================================
    std::multiset<int> q{1, 2, 2, 3, 3, 3, 4, 5};

    // count: ★ multiset 中可能 > 1
    std::cout << "\ncount(3) = " << q.count(3) << '\n';   // 3
    std::cout << "count(2) = " << q.count(2) << '\n';   // 2

    // find: 回傳「某一個」== key 的 iterator (不保證是第一個,實作可能是)
    auto fi = q.find(3);
    if (fi != q.end()) std::cout << "find(3) → " << *fi << '\n';

    // contains (C++20)
    std::cout << "contains(4) = " << q.contains(4) << '\n';

    // ──── equal_range ──── multiset 的真正用武之地
    auto [b1, e1] = q.equal_range(3);
    std::cout << "all 3s: ";
    for (auto it_ = b1; it_ != e1; ++it_) std::cout << *it_ << ' ';
    std::cout << '\n';

    // lower_bound / upper_bound
    auto lb = q.lower_bound(2);
    auto ub = q.upper_bound(3);
    std::cout << "[lb=2..ub=3): ";
    for (auto it_ = lb; it_ != ub; ++it_) std::cout << *it_ << ' ';
    std::cout << '\n';

    // ========================================================================
    //  6. Observers (key_comp / value_comp)
    // ========================================================================
    // 對 set / multiset 而言,key_comp() 與 value_comp() 是同一個比較函式,
    // 因為元素本身就是 key (沒有獨立的 value)。
    auto kc = q.key_comp();
    auto vc = q.value_comp();
    std::cout << "\nkey_comp(2,3)   = " << kc(2, 3) << '\n';     // 1 (true)
    std::cout << "value_comp(3,3) = " << vc(3, 3) << '\n';     // 0 (相等視為 not less)

    // ========================================================================
    //  7. C++20 free functions
    // ========================================================================
    std::multiset<int> n1{1,2,2,3,3,3,4};
    std::erase_if(n1, [](int x){ return x % 2 == 0; });
    print(n1, "erase_if(even)      ");

    // ========================================================================
    //  7. 常見陷阱 (Pitfalls)
    // ========================================================================
    //
    //  (1) erase(key) 會刪「所有」等於 key 的元素
    //      若只想刪一個 → erase(iterator) 或 erase(find(key))。
    //
    //  (2) find(key) 不保證回傳第一個
    //      若要「第一個」,用 lower_bound(key)。
    //
    //  (3) merge 不會去重
    //      multiset 設計就是允許重複,merge 會把全部都搬過來。
    //
    //  (4) iterator 走訪到「相等元素群」時,順序為插入順序 (C++11 stable)
    //      但 find() 仍可能跳到中間某個元素。
    //
    //  (5) 比較相等的元素仍要符合 strict weak ordering
    //      cmp(a,b) 和 cmp(b,a) 都 false 表示「等價」,multiset 視為「相等」。

    // ========================================================================
    //  8. 最佳實踐
    // ========================================================================
    //
    //  • 需要排序 + 允許重複 → multiset
    //  • 想做「直方圖」(計數) → 用 map<key, int> 通常更直觀,而非 multiset
    //  • 不在乎順序 + 允許重複 → unordered_multiset 平均更快
    //  • 想在 multiset 中只刪除「一個」key → 用 erase(find(key))

    // ========================================================================
    //  9. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // multiset 在「滑動視窗中需要動態查詢最大/最小/中位數」的題目特別好用,
    // 因為它能 O(log n) 插入、刪除任意元素,而且自帶排序。

    // ──── LC 1696 風格: 滑動視窗中的最大值 (multiset 版) ────
    // 這題用 deque 是 O(n),但用 multiset 解的好處是邏輯直觀:
    // begin() 是最小、rbegin() 是最大,任意時刻都能 O(log n) 拿。
    {
        std::vector<int> nums{1, 3, -1, -3, 5, 3, 6, 7};
        int k = 3;
        std::multiset<int> window;
        std::vector<int> ans;
        for (int i = 0; i < (int)nums.size(); ++i) {
            window.insert(nums[i]);
            if ((int)window.size() > k) {
                window.erase(window.find(nums[i - k]));   // ★ 只刪一個
            }
            if ((int)window.size() == k) ans.push_back(*window.rbegin());
        }
        std::cout << "\n[multiset Sliding Max] = [ ";
        for (int x : ans) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出: [ 3 3 5 5 6 7 ]
    }

    // ──── LC 1296: Divide Array in Sets of K Consecutive Numbers ────
    // 題目簡述:
    //   給定一個陣列 nums 與整數 k,問能否把陣列「剛好分成」若干組,
    //   每一組都是「連續的 k 個整數」(例如 [3,4,5,6])。
    //
    // 為什麼最自然用 multiset:
    //   1. multiset 自動排序 → 每次 begin() 直接拿到目前最小值。
    //   2. 允許重複元素 → 陣列裡的重複數字也能正確處理。
    //   3. find + erase(iterator) 是 O(log n),能精準刪「一個」元素。
    //
    // 核心思路 (Greedy):
    //   反覆取出目前最小的元素 m,則該組必然為 m, m+1, ..., m+k-1。
    //   依序在 multiset 中找這 k 個數字並逐一刪除;若中途有缺,直接 false。
    //
    // 時間複雜度: O(n log n)
    {
        auto can_divide = [](std::vector<int> nums, int k) {
            if ((int)nums.size() % k != 0) return false;     // 數量必須是 k 的倍數
            std::multiset<int> ms(nums.begin(), nums.end());
            while (!ms.empty()) {
                int start = *ms.begin();                     // 目前最小者必為某組的起點
                for (int i = 0; i < k; ++i) {
                    auto it = ms.find(start + i);
                    if (it == ms.end()) return false;        // 缺了某個連續值 → 不行
                    ms.erase(it);                            // ★ 一次只刪一個 (傳 iterator)
                }
            }
            return true;
        };
        std::cout << "[LC1296] " << std::boolalpha
                  << can_divide({1,2,3,3,4,4,5,6}, 4) << ' '   // true → [1,2,3,4],[3,4,5,6]
                  << can_divide({3,2,1,2,3,4,3,4,5,9,10,11}, 3) << ' '  // true
                  << can_divide({1,2,3,4}, 3) << '\n';         // false (4 不是 3 的倍數)
        // 預期輸出: true true false
    }

    // ──── LC 350: Intersection of Two Arrays II (兩陣列交集 II) ────
    // 給兩個陣列,回傳交集 (每個元素出現次數取 min)。
    // 思路:把 nums1 全部丟進 multiset,然後走訪 nums2 — 若 find 到就加入結果並 erase 一個。
    // 為何用 multiset:「重複次數要對應」是 multiset 的看家本領,erase(iterator) 只刪一個。
    {
        std::vector<int> nums1{1,2,2,1};
        std::vector<int> nums2{2,2};
        std::multiset<int> ms(nums1.begin(), nums1.end());
        std::vector<int> intersect;
        for (int x : nums2) {
            auto it = ms.find(x);
            if (it != ms.end()) {
                intersect.push_back(*it);
                ms.erase(it);     // 只刪一個,確保「次數取 min」
            }
        }
        std::cout << "[LC350 Intersect] = [ ";
        for (int x : intersect) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出: [ 2 2 ]
    }

    // ========================================================================
    //  12. 實戰範例:訂單系統 — 維護動態的「成交價序列」
    // ========================================================================
    // 真實場景:撮合系統不斷有成交 (允許相同價格多筆),需即時取得:
    //   • 最高 / 最低成交價   → *rbegin() / *begin()  皆 O(1)
    //   • 中位成交價         → advance(begin(), size()/2)  O(n) 走訪,但比排序快
    //   • 移除特定一筆 (回滾) → find + erase  O(log n)
    // multiset 在這類「需要排序 + 重複 + 動態增刪」的場景無可取代。
    {
        std::multiset<double> prices{101.5, 101.5, 101.7, 102.0, 101.3, 101.8};
        std::cout << "[Order Book] 最低=" << *prices.begin()
                  << ",最高=" << *prices.rbegin()
                  << ",總筆數=" << prices.size() << '\n';

        // 假設要回滾一筆 101.5 (只移除一筆,不是全部)
        auto it = prices.find(101.5);
        if (it != prices.end()) prices.erase(it);
        std::cout << "[Order Book] 回滾後 101.5 剩 " << prices.count(101.5) << " 筆\n";
        // 預期輸出: 最低=101.3,最高=102,總筆數=6 / 回滾後 101.5 剩 1 筆
    }

    std::cout << "\n=== multiset demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：multiset.erase(value) 跟 multiset.erase(iterator) 行為差在哪?
    //    A：erase(value) 會「一次刪除所有等於 value 的元素」並回傳被刪數量;
    //       erase(iterator) 只刪該 iterator 指向的單一節點。要只刪一筆重複值,
    //       常用 it = ms.find(value); if (it != ms.end()) ms.erase(it);
    //
    //  Q2：為何 multiset 可以維護 sliding window 中位數比 priority_queue 更通用?
    //    A：multiset 支援「O(log n) 任意刪除指定值」(priority_queue 只能刪 top),
    //       讓滑動視窗離開的舊值能精準移除;同時保留排序,可用 advance(begin, k)
    //       直接拿到第 k 小元素 (LC480 解法核心)。
    //
    //  Q3：multiset 內相同元素的相對順序是否保證穩定?
    //    A：C++11 起標準保證「插入順序在等價元素間被保留」,所以 equal_range 走訪
    //       出來的順序與插入順序一致。這跟 multimap 的保證是一致的。
    //
    return 0;
}

/*
============================================================================
  附錄:std::multiset 完整 member function 一覽
============================================================================
  介面與 std::set 相同,主要差異:
      • insert 回傳 iterator (而非 pair<iterator, bool>)
      • count(key) 可能 > 1
      • erase(key) 回傳刪除數量 (可能 > 1)
      • equal_range / lower_bound / upper_bound 真正有用
      • merge 不會去重

  所有函式名稱與 set 一致:
      Constructors / destructor / operator= / get_allocator
      begin/end/cbegin/cend/rbegin/rend/crbegin/crend
      empty, size, max_size
      clear, insert, insert_range (C++23), emplace, emplace_hint,
      erase, swap, extract, merge
      count, find, contains (C++20), equal_range, lower_bound, upper_bound
      key_comp, value_comp
      operator==, <=> (C++20), std::swap, std::erase_if (C++20)
============================================================================
*/
