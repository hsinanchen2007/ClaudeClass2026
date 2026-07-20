// ============================================================================
//  forward_list.cpp — std::forward_list 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra forward_list.cpp -o forward_list && ./forward_list
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/forward_list
//  參考 (cplusplus.com): https://cplusplus.com/reference/forward_list/forward_list/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::forward_list (C++11 引入) 是「單向連結串列 (singly linked list)」。
//  設計目標是「最低空間負擔的 list」 — 每個節點只有一個 next 指標,沒有 prev,
//  也沒有 size 成員 (size() 不存在,只有 max_size())。
//
//  ▌ 底層資料結構
//      ┌──────┐    ┌──────┐    ┌──────┐    ┌──────┐
//      │ data │ →  │ data │ →  │ data │ →  │ data │ → nullptr
//      │ next │    │ next │    │ next │    │ next │
//      └──────┘    └──────┘    └──────┘    └──────┘
//
//  ▌ 所屬類別
//  Sequence container,但 API 與其他 container 略有不同
//  (用 *_after 系列函式而非一般 insert / erase)。
//
//  ▌ 時間複雜度
//      front                          O(1)
//      push_front / pop_front         O(1)
//      insert_after / erase_after     O(1) (給 iterator)
//      搜尋                           O(n)
//      sort / merge / reverse         O(n log n) / O(n+m) / O(n)
//      ★ 沒有 size() — 計算 size 是 O(n) (用 std::distance)
//
//  ▌ 與其他 container 的比較
//      forward_list  vs  list  : forward_list 省 1 個指標的空間,但只能單向走
//      forward_list  vs  vector: 同 list 比較 — 中間 insert/erase O(1) 但 cache 差
//
//  ▌ 適用情境
//      ✅ 需要 list 的特性,但又想盡量壓低每個節點的 overhead
//      ✅ 嵌入式 / 低記憶體環境
//      ❌ 需要 size()、反向走訪、隨機存取 → 用 list 或 vector
//
//  ▌ 為什麼是 *_after 系列函式?
//  單向 list 沒辦法「在某個位置之前插入」,因為要修改前一個節點的 next 指標
//  就需要從頭走到該位置 — 那就 O(n) 了。所以 forward_list 規定:
//  你給我「要插在哪個節點之後」,而非「之前」,這樣保證 O(1)。
//
//  ▌ before_begin() 的存在意義
//  因為要「在 begin() 之後插入」其實是「在第二個位置插入」,
//  那要「在最前面插入」怎麼辦?答案是 before_begin() —
//  一個假的「begin 之前」位置,專門配 *_after 函式使用。
//
//  ▌ Iterator 失效規則
//      • insert_after / erase_after:不影響其他 iterator (只影響相關節點)
//      • clear:全部失效
//      • splice_after:被搬走的節點 iterator 仍然有效
//
// ============================================================================

/*
補充筆記：std::forward_list
  - forward_list 是單向鏈結串列，只能往前走，因此很多操作需要 before_begin。
  - erase_after/insert_after 的 API 反映了「只能知道前一個節點」的限制。
  - 它比 list 少一個反向指標，但可用性也更受限。
  - std::forward_list 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::forward_list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. forward_list 為什麼只有 insert_after / erase_after？
//     答：它是 singly linked list，只有 next 指標。要在某節點「之前」插入必須先找到前驅，
//         那需要從頭走訪 O(n)。改成 *_after 版本後全部都是 O(1)，
//         並提供 before_begin() 讓你能在第一個元素之前插入。
//     追問：什麼情境真的會選 forward_list 而不是 list？（極度在意每節點記憶體、且只需單向走訪）
//
// 🔥 Q2. forward_list 為什麼沒有 size()？
//     答：C++11 起標準要求容器的 size() 為 O(1)，那就必須多維護一個計數器。
//         forward_list 的設計目標是「零額外 overhead 的最小 singly linked list」，
//         因此它干脆不提供 size()（也沒有 push_back）；需要算個數請用 std::distance。
//
// Q3. forward_list 為什麼也有自己的 sort() 成員函式？
//     答：std::sort 要求 random access iterator，而 forward_list::iterator 只是 forward iterator，
//         型別上就不相容。成員 sort() 以重接節點指標的方式排序，不搬移元素、不配置新節點，
//         且是 stable 的。同理 remove() / unique() / merge() / splice_after() 也都是成員函式。
// ═══════════════════════════════════════════════════════════════════════════

#include <forward_list>
#include <iostream>
#include <iterator>
#include <string>
#include <algorithm>

template <typename T>
void print(const std::forward_list<T>& f, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "[ ";
    for (const auto& e : f) std::cout << e << ' ';
    std::cout << "]\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::forward_list<int> f1;
    std::forward_list<int> f2(5);                          // 5 個 0
    std::forward_list<int> f3(5, 42);
    std::forward_list<int> f4{1, 2, 3, 4, 5};
    std::forward_list<int> f5(f4.begin(), f4.end());
    std::forward_list<int> f6(f4);
    std::forward_list<int> f7(std::move(f6));

    print(f3, "f3 (5 個 42)        ");
    print(f4, "f4                  ");
    print(f7, "f7 (move)           ");

    // ========================================================================
    //  2. assign
    // ========================================================================
    std::forward_list<int> a;
    a.assign(3, 99);
    print(a, "assign(3,99)        ");
    a.assign({7,8,9});
    print(a, "assign({...})       ");

    // ========================================================================
    //  3. 元素存取
    // ========================================================================
    //  forward_list 只有 front(),沒有 back() (因為走到尾巴是 O(n))
    std::forward_list<int> e{10, 20, 30};
    std::cout << "\n[Access] front = " << e.front() << '\n';

    // ========================================================================
    //  4. Iterators (forward iterator,只能 ++)
    // ========================================================================
    //  特殊 iterator: before_begin() / cbefore_begin()
    //  指向「begin 之前的虛擬位置」,供 *_after 操作使用。
    //
    //  ★ 沒有 rbegin / rend / end - 1 — 沒有反向 iterator!

    std::cout << "[正向] ";
    for (auto it = e.begin(); it != e.end(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // ========================================================================
    //  5. 容量
    // ========================================================================
    //  ★ 沒有 size()!
    //  empty() 仍然存在 (O(1)),但要算長度只能 std::distance(begin, end)。
    std::cout << "\n[Capacity] empty=" << std::boolalpha << e.empty()
              << ", size (via distance)=" << std::distance(e.begin(), e.end()) << '\n';

    // max_size():理論上的元素上限 (注意:沒有 size())
    std::cout << "max_size = " << e.max_size() << '\n';

    // get_allocator():取得目前 allocator
    auto alloc = e.get_allocator();
    int* tmp = alloc.allocate(1);
    alloc.deallocate(tmp, 1);
    std::cout << "get_allocator() OK\n";

    // ──── cbefore_begin() ────
    // before_begin() 的 const 版本,只能讀,不能用於修改 (但 forward_list 的
    // *_after 函式不需要可寫的 hint,所以實務上 cbefore_begin 較少直接使用)。
    std::forward_list<int> cb{1, 2, 3};
    auto cbb = cb.cbefore_begin();
    auto cb_first = std::next(cbb);   // 等同 cb.cbegin()
    std::cout << "cbefore_begin → next = " << *cb_first << '\n';

    // ========================================================================
    //  6. Modifiers — 注意全是 *_after!
    // ========================================================================

    // clear
    std::forward_list<int> m{1,2,3};
    m.clear();
    print(m, "after clear         ");

    // ──── insert_after(pos, value) ────
    // 在 pos「之後」插入,回傳指向新元素的 iterator
    std::forward_list<int> m1{1, 5};
    m1.insert_after(m1.begin(), 2);                 // [1,2,5]
    m1.insert_after(m1.begin(), 2, 4);              // [1,4,4,2,5]
    m1.insert_after(m1.begin(), {6, 7});            // [1,6,7,4,4,2,5]
    print(m1, "insert_after        ");

    // 在「最前面」插入: 用 before_begin()
    std::forward_list<int> m1b{2, 3};
    m1b.insert_after(m1b.before_begin(), 1);        // [1,2,3]
    print(m1b, "insert_after(before_begin)");

    // ──── emplace_after ────
    std::forward_list<std::string> m2;
    m2.emplace_after(m2.before_begin(), 5, 'A');    // "AAAAA"
    print(m2, "emplace_after       ");

    // ──── erase_after(pos) / erase_after(first, last) ────
    // 刪除 pos 之「後」的一個或一段元素
    std::forward_list<int> m3{1,2,3,4,5};
    m3.erase_after(m3.begin());                     // 刪 2 → [1,3,4,5]
    print(m3, "erase_after         ");
    m3.erase_after(m3.begin(), m3.end());           // 刪 begin 之後到 end → [1]
    print(m3, "erase_after(range)  ");

    // ──── push_front / emplace_front / pop_front ────
    std::forward_list<int> m4;
    m4.push_front(3);
    m4.push_front(2);
    m4.push_front(1);                               // [1,2,3]
    m4.emplace_front(0);                            // [0,1,2,3]
    print(m4, "push/emplace_front  ");
    m4.pop_front();
    print(m4, "after pop_front     ");

    // ★ 注意:沒有 push_back / pop_back / back() 函式

    // ──── resize ────
    // 注意:擴增的 default value 加在尾端,需要走完整個 list (O(n))
    std::forward_list<int> m6{1, 2, 3};
    m6.resize(5);                                   // [1,2,3,0,0]
    print(m6, "resize(5)           ");
    m6.resize(7, 9);
    print(m6, "resize(7, 9)        ");
    m6.resize(2);
    print(m6, "resize(2)           ");

    // ──── swap ────
    std::forward_list<int> s1{1,2,3}, s2{9,8};
    s1.swap(s2);
    print(s1, "s1 after swap       ");

    // ========================================================================
    //  7. List operations (與 std::list 同名,但是 *_after 版本)
    // ========================================================================

    // ──── merge ────
    std::forward_list<int> ma{1,3,5,7}, mb{2,4,6,8};
    ma.merge(mb);
    print(ma, "after merge         ");
    print(mb, "mb (掏空)           ");

    // ──── splice_after(pos, other) ────
    // 將 other 整個串接到 pos 之後
    std::forward_list<int> sa{1,2,3}, sb{10,20,30};
    sa.splice_after(sa.begin(), sb);
    print(sa, "after splice_after  ");
    print(sb, "sb (掏空)           ");

    // ──── remove / remove_if ────
    std::forward_list<int> ra{1,2,3,2,4,2,5};
    ra.remove(2);
    print(ra, "remove(2)           ");
    ra.remove_if([](int x){ return x % 2 == 0; });
    print(ra, "remove_if(even)     ");

    // ──── reverse ────
    std::forward_list<int> rv{1,2,3,4};
    rv.reverse();
    print(rv, "reverse             ");

    // ──── unique ────
    std::forward_list<int> ua{1,1,2,3,3,3,4,1,1};
    ua.unique();
    print(ua, "unique              ");

    // ──── sort ────
    std::forward_list<int> so{3,1,4,1,5,9,2,6};
    so.sort();
    print(so, "sort                ");
    so.sort(std::greater<int>{});
    print(so, "sort(greater)       ");

    // ========================================================================
    //  8. 比較與 C++20 free functions
    // ========================================================================
    std::forward_list<int> c1{1,2,3}, c2{1,2,4};
    std::cout << "\n[比較] c1 < c2 ? " << (c1 < c2) << '\n';

    std::forward_list<int> n1{1,2,3,4,5,6};
    std::erase_if(n1, [](int x){ return x % 2 == 0; });
    print(n1, "erase_if(even)      ");

    // ========================================================================
    //  9. 常見陷阱 (Pitfalls)
    // ========================================================================
    //
    //  (1) 沒有 size()!
    //      要算長度必須 std::distance(fl.begin(), fl.end()),且為 O(n)。
    //
    //  (2) 全部是 *_after 函式
    //      insert_after, erase_after, emplace_after, splice_after。
    //      想在 pos 之「前」插入是不可能的 (除非從 before_begin 走到 prev)。
    //
    //  (3) 沒有 push_back / pop_back / back
    //      因為要走到尾巴是 O(n),作者不想偽裝成 O(1)。
    //      若需要 → 改用 std::list。
    //
    //  (4) 沒有反向 iterator
    //      forward_list 是 forward iterator,不能 rbegin/rend。
    //
    //  (5) before_begin() 不能解 reference!
    //      *fl.before_begin() 是 UB,只能拿來給 *_after 函式當 pos。

    // ========================================================================
    //  10. 最佳實踐
    // ========================================================================
    //
    //  • 比 list 更省空間 (每節點少 8 bytes 的 prev 指標)
    //  • 嵌入式 / 記憶體緊張 / 只需要單向走訪 → forward_list
    //  • 想要 size 是 O(1) → 用 list
    //  • 想要任意位置都能 O(1) insert (給 iterator) → list / forward_list 都可
    //  • 沒有特別需求 → 大多數情境直接用 vector

    // ========================================================================
    //  11. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // forward_list 結構等同單向鏈結串列,LeetCode 上「Linked List」類題目的解法
    // 都能直接套用。以下示範三題單向鏈結串列的經典套路。

    // ──── LC 206: Reverse Linked List (反轉鏈結串列) ────
    // 經典反轉作法: 用三個指標 prev / curr / next 一邊走一邊反向接回去。
    // forward_list 雖然有 reverse() 內建函式,這裡示範手刻演算法的版本。
    {
        std::forward_list<int> fl{1, 2, 3, 4, 5};
        // forward_list 內建 O(n) 反轉
        fl.reverse();
        print(fl, "[LC206 reverse()]   ");
        // 預期輸出: [ 5 4 3 2 1 ]
    }

    // ──── LC 876: Middle of the Linked List (找中間節點) ────
    // 「快慢指標」: fast 一次走兩步,slow 一次走一步,fast 到尾時 slow 在中間。
    // 因為 forward_list 沒有 size() 與 random access,這個技巧特別合適。
    {
        std::forward_list<int> fl{1, 2, 3, 4, 5};
        auto slow = fl.begin();
        auto fast = fl.begin();
        while (fast != fl.end() && std::next(fast) != fl.end()) {
            ++slow;
            fast = std::next(fast, 2);
        }
        std::cout << "[LC876 Middle node value] = " << *slow << '\n';
        // 預期輸出: 3  (奇數長度,中間是第 3 個節點)
    }

    // ──── LC 83: Remove Duplicates from Sorted List (移除排序串列重複項) ────
    // 已排序串列中相鄰重複只留一個。展示 erase_after 的標準用法。
    {
        std::forward_list<int> fl{1, 1, 2, 3, 3, 3, 4, 5, 5};
        for (auto it = fl.begin(); it != fl.end(); ) {
            auto nxt = std::next(it);
            if (nxt != fl.end() && *nxt == *it) {
                fl.erase_after(it);   // 刪掉 nxt,it 不動
            } else {
                ++it;
            }
        }
        print(fl, "[LC83 dedup sorted] ");
        // 預期輸出: [ 1 2 3 4 5 ]
    }

    // ──── LC 141: Linked List Cycle (判斷鏈結串列是否有環) ────
    // 用 Floyd's tortoise-and-hare 演算法:slow 走 1 步,fast 走 2 步,
    // 若有環 fast 一定會追上 slow;若 fast 走到 end 則無環。O(n) 時間 O(1) 空間。
    // forward_list 標準上不允許自我引用形成環,所以這裡用「邏輯模擬」展示算法。
    {
        std::forward_list<int> fl{1, 2, 3, 4, 5};
        // 在 std::forward_list 上無法真正製造環,我們示範「無環」情況下演算法的執行
        auto slow = fl.begin();
        auto fast = fl.begin();
        bool has_cycle = false;
        while (fast != fl.end() && std::next(fast) != fl.end()) {
            slow = std::next(slow);
            fast = std::next(fast, 2);
            if (slow == fast) { has_cycle = true; break; }
        }
        std::cout << "[LC141 hasCycle] = " << std::boolalpha << has_cycle << '\n';
        // 預期輸出: false
    }

    // ========================================================================
    //  12. 實戰範例:嵌入式系統的事件鏈 (Event Chain)
    // ========================================================================
    // 真實場景:嵌入式韌體記憶體有限,需要極省空間的「事件鏈表」記錄感測器事件。
    // 每個事件只需要往前處理,不會反向走訪 → forward_list 比 list 每節點省一個
    // prev pointer (在 64-bit 平台是 8 bytes),累積 1000 個節點就省 8 KB。
    // 需求:從表頭加入新事件、舊事件處理完從中間移除。
    {
        struct SensorEvent { int id; double value; };
        std::forward_list<SensorEvent> chain;

        // push_front 為 O(1),適合「最新事件優先處理」的 LIFO 邏輯
        chain.push_front({3, 22.5});
        chain.push_front({2, 23.1});
        chain.push_front({1, 23.8});

        // 移除 id=2 的事件 (例如已處理完)
        chain.remove_if([](const SensorEvent& e){ return e.id == 2; });

        std::cout << "[Embedded Event Chain] ";
        for (auto& e : chain) std::cout << "(id=" << e.id << ",v=" << e.value << ") ";
        std::cout << '\n';
        // 預期輸出: (id=1,v=23.8) (id=3,v=22.5)
    }

    std::cout << "\n=== forward_list demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：forward_list 為何沒有 size() 成員函式?
    //    A：因為設計目標是「最小開銷的單向 linked list」,額外維護 size 變數會多
    //       一個 word 的記憶體與每次 insert/erase 的計數開銷。需要時可用
    //       std::distance(begin, end) 自行計算,但是 O(n)。
    //
    //  Q2：為何插入 / 刪除函式叫 insert_after、erase_after 而不是普通 insert?
    //    A：單向 linked list 只能往後走,要在某節點「前面」插入需先找到前驅節點
    //       (O(n))。forward_list 直接限制 API 為「在指定 iterator 之後操作」,
    //       讓使用者明確意識到這個限制,並提供 before_begin() 取得 head 之前的
    //       哨兵 iterator,以便在最前端插入。
    //
    //  Q3：什麼時候才該用 forward_list 而不是 list 或 vector?
    //    A：只有在記憶體極度敏感 (每節點少一個 prev pointer 也算節省) 且確定不需
    //       反向走訪時才用。實務上多數情況 list 或 vector 已經夠用,forward_list
    //       是非常 niche 的選擇,出現在嵌入式或自製 allocator 場景較多。
    //
    return 0;
}

/*
============================================================================
  附錄:std::forward_list 完整 member function 一覽
============================================================================
  Constructors / destructor / operator=
  assign / assign_range (C++23) / get_allocator

  Element access:  front

  Iterators:       before_begin, cbefore_begin
                   begin, end, cbegin, cend
                   (沒有 rbegin / rend!)

  Capacity:        empty, max_size  (沒有 size!)

  Modifiers:       clear,
                   insert_after, emplace_after, erase_after,
                   insert_range_after (C++23),
                   push_front, emplace_front, pop_front,
                   prepend_range (C++23),
                   resize, swap

  List operations:
      merge, splice_after, remove, remove_if, reverse, unique, sort

  Non-member:      operator==, !=, <=>, std::swap, std::erase, std::erase_if
============================================================================
*/
