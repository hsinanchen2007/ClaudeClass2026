// ============================================================================
//  list.cpp — std::list 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra list.cpp -o list && ./list
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/list
//  參考 (cplusplus.com): https://cplusplus.com/reference/list/list/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::list 是「雙向連結串列 (doubly linked list)」。每個節點都同時持有
//  指向前後節點的指標,任意位置 insert/erase 皆為 O(1) (拿到 iterator 之後)。
//  代價是:不支援隨機存取、cache 不友善、每個元素多 2 個指標的空間負擔。
//
//  ▌ 底層資料結構
//      ┌──────┐    ┌──────┐    ┌──────┐    ┌──────┐
//      │ data │←→  │ data │←→  │ data │←→  │ data │
//      │ prev │    │ prev │    │ prev │    │ prev │
//      │ next │    │ next │    │ next │    │ next │
//      └──────┘    └──────┘    └──────┘    └──────┘
//
//  ▌ 所屬類別
//  Sequence container
//
//  ▌ 時間複雜度
//      隨機存取 (operator[])        ❌ 不支援
//      front / back                 O(1)
//      push_back / push_front       O(1)
//      pop_back  / pop_front        O(1)
//      insert / erase (給 iterator) O(1)
//      搜尋 (linear)                O(n)
//      sort                         O(n log n) — 但「不是」std::sort,而是 list::sort
//      splice                       O(1) (整段) 或 O(n)
//      size                         O(1) (C++11 起保證)
//
//  ▌ 與其他 container 的比較
//      list  vs  vector       : list 任意位置 O(1) insert,但無隨機存取、cache 差
//      list  vs  forward_list : forward_list 是「單向」,佔用更少空間但只能單向走
//      list  vs  deque        : deque 兩端 O(1),中間 O(n);list 任意位置 O(1)
//
//  ▌ 適用情境
//      ✅ 中間頻繁 insert / erase 且元素本身很大 (拷貝成本高)
//      ✅ 需要 splice (合併兩個 list) 的零拷貝 O(1) 操作
//      ✅ 需要 stable iterator (insert/erase 不會使其他 iterator 失效)
//      ❌ 需要隨機存取 → 改用 vector / deque
//      ❌ 元素小且需要遍歷效能 → 用 vector,cache locality 完勝 list
//
//  ▌ Iterator 失效規則 ★
//      • insert:不影響任何既有 iterator
//      • erase :只有「被刪除」的元素 iterator 失效
//      • splice:被搬走的節點 iterator 仍然有效 (★ 這是 splice 的核心特性)
//      • clear :所有 iterator 失效
//
// ============================================================================

/*
補充筆記：std::list
  - list 是雙向鏈結串列，插入刪除節點不會搬移其他元素。
  - 它不支援 random access，因此 std::sort 不能直接用，要用 list::sort。
  - 節點配置成本高、快取區域性差；只有在穩定 iterator 或 splice 很重要時才值得。
  - std::list 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector、list、deque 如何選？為什麼實務上 list 常常比 vector 慢？
//     答：list 是 doubly linked list，已持有 iterator 時任意位置插入/刪除都是 O(1)，
//         但無 random access、查找 O(n)。雖然理論複雜度好看，實測常輸 vector：
//         (1) 節點分散在 heap，cache locality 差；(2) 每次插入都要一次 heap allocation；
//         (3) 每節點多兩個指標的記憶體 overhead。預設選 vector，真的需要穩定性才選 list。
//     追問：那 list 的真正優勢是什麼？（iterator / reference 穩定性 + O(1) splice）
//
// 🔥 Q2. std::list 的 size() 是 O(1) 還是 O(n)？splice 呢？
//     答：C++11 起標準要求所有容器的 size() 為 O(1)，所以 list::size() 是 O(1)（內部維護計數器）。
//         代價落在 splice：接「整個 list」是 O(1)，
//         但接部分區間的 splice(pos, other, first, last) 因為要更新元素個數，是 O(distance(first,last))。
//
// Q3. 為什麼 std::list 有自己的 sort()，而不能用 std::sort？
//     答：std::sort 需要 random access iterator（introsort 要做 partition 與 heap 操作），
//         而 list::iterator 只是 bidirectional iterator，型別上就不相容。
//         list::sort() 採 merge sort，透過重接節點指標排序，不搬移元素也不配置新節點，且是 stable。
//
// ⚠️ 陷阱. 對 list 做 insert / sort 之後，舊的 iterator 還有效嗎？
//     答：都有效。list 的 insert 完全不影響既有 iterator；
//         只有「被刪除元素」的 iterator / reference 會失效。
//         sort() 之後 iterator 仍指向同一個元素，只是該元素在序列中的位置變了。
//     為什麼會錯：拿 vector「插入就可能全失效」的直覺套到所有容器。
// ═══════════════════════════════════════════════════════════════════════════

#include <list>
#include <iostream>
#include <algorithm>
#include <string>
#include <unordered_map>     // for LeetCode LRU 範例
#include <iterator>          // std::next

template <typename T>
void print(const std::list<T>& l, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "[ ";
    for (const auto& e : l) std::cout << e << ' ';
    std::cout << "] (size=" << l.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::list<int> l1;
    std::list<int> l2(5);                          // 5 個 0
    std::list<int> l3(5, 42);
    std::list<int> l4{1, 2, 3, 4, 5};
    std::list<int> l5(l4.begin(), l4.end());
    std::list<int> l6(l4);
    std::list<int> l7(std::move(l6));

    print(l1, "l1                  ");
    print(l3, "l3 (5 個 42)        ");
    print(l4, "l4                  ");
    print(l7, "l7 (move)           ");

    // ========================================================================
    //  2. assign
    // ========================================================================
    std::list<int> a;
    a.assign(3, 99);
    print(a, "assign(3,99)        ");
    a.assign({7,8,9});
    print(a, "assign({...})       ");

    // ========================================================================
    //  3. 元素存取
    // ========================================================================
    //  list 只有 front() / back(),沒有 operator[] / at() / data()
    //  原因:無法 O(1) 隨機存取,也沒有連續記憶體。
    std::list<int> e{10, 20, 30, 40};
    std::cout << "\n[Access]\n";
    std::cout << "front = " << e.front() << '\n';
    std::cout << "back  = " << e.back() << '\n';

    // ========================================================================
    //  4. Iterators (bidirectional iterator,不是 random access)
    // ========================================================================
    //  ★ 不能 it + 3、不能 it1 < it2,只能 ++ / --。
    std::cout << "\n[正向] ";
    for (auto it = e.begin(); it != e.end(); ++it) std::cout << *it << ' ';
    std::cout << "\n[反向] ";
    for (auto it = e.rbegin(); it != e.rend(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // 想要前進 N 步要用 std::advance / std::next
    auto it = std::next(e.begin(), 2);
    std::cout << "std::next(begin,2) = " << *it << '\n';

    // ========================================================================
    //  5. 容量
    // ========================================================================
    //  empty / size / max_size  (沒有 capacity / reserve / shrink_to_fit)
    //  C++11 起 size() 保證 O(1) — 之前可能是 O(n)。

    std::cout << "\n[Capacity] size=" << e.size()
              << ", empty=" << std::boolalpha << e.empty() << '\n';

    // max_size():理論元素上限
    std::cout << "max_size = " << e.max_size() << '\n';

    // get_allocator():取得 allocator
    auto alloc = e.get_allocator();
    int* tmp = alloc.allocate(1);
    alloc.deallocate(tmp, 1);
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  6. Modifiers
    // ========================================================================

    // clear
    std::list<int> m{1,2,3};
    m.clear();
    print(m, "after clear         ");

    // insert
    std::list<int> m1{1, 2, 5};
    auto it1 = m1.begin();
    std::advance(it1, 2);                          // 指向 5
    m1.insert(it1, 3);                             // [1,2,3,5]
    m1.insert(it1, 2, 4);                          // [1,2,3,4,4,5] (在 5 前插入兩個 4)
    m1.insert(m1.end(), {6, 7});                   // [1,2,3,4,4,5,6,7]
    print(m1, "after insert        ");

    // emplace
    std::list<std::string> m2;
    m2.emplace(m2.end(), 5, 'A');                  // string(5,'A') = "AAAAA"
    print(m2, "after emplace       ");

    // erase
    std::list<int> m3{1,2,3,4,5};
    auto eit = m3.begin();
    std::advance(eit, 2);                          // 指向 3
    m3.erase(eit);                                 // [1,2,4,5]
    print(m3, "after erase(it→3)   ");

    // ──── push_back / push_front ────
    std::list<int> m4;
    m4.push_back(2);
    m4.push_back(3);
    m4.push_front(1);
    m4.push_front(0);
    print(m4, "push_back/front     ");

    // emplace_back / emplace_front
    std::list<std::pair<int,std::string>> m5;
    m5.emplace_back(1, "b");
    m5.emplace_front(0, "f");
    std::cout << "emplace_back/front: ";
    for (auto& [k,v] : m5) std::cout << '(' << k << ',' << v << ") ";
    std::cout << '\n';

    // pop_back / pop_front
    m4.pop_back();
    m4.pop_front();
    print(m4, "after pop_b/pop_f   ");

    // resize
    std::list<int> m6{1,2,3};
    m6.resize(5);
    print(m6, "resize(5)           ");
    m6.resize(7, 9);
    print(m6, "resize(7,9)         ");

    // swap
    std::list<int> s1{1,2,3}, s2{9,8};
    s1.swap(s2);
    print(s1, "s1 after swap       ");

    // ========================================================================
    //  7. List 獨有操作 (Operations)
    // ========================================================================
    //  list 提供一系列「不能用 std:: 演算法」的成員函式,
    //  原因是 std:: 演算法需要 random access,list 不支援。

    // ──── merge(other) ────
    // 把另一個「已排序」list 合併進來,O(n+m),other 變空
    std::list<int> ma{1,3,5,7}, mb{2,4,6,8};
    ma.merge(mb);
    print(ma, "after merge         ");
    print(mb, "mb (被掏空)         ");

    // ──── splice(pos, other [, it [, last]]) ────
    // 把 other 的節點直接搬過來,O(1) (整段) 或 O(n) (區間)
    // ★ 是真正的「節點移交」,不會有任何拷貝
    std::list<int> sa{1,2,3}, sb{10,20,30};
    sa.splice(std::next(sa.begin()), sb);  // 把 sb 整個插入 sa.begin()+1 之前
    print(sa, "after splice all    ");
    print(sb, "sb (被掏空)         ");

    // ──── remove / remove_if ────
    std::list<int> ra{1,2,3,2,4,2,5};
    ra.remove(2);                    // 刪除所有 == 2 的元素
    print(ra, "remove(2)           ");
    ra.remove_if([](int x){ return x % 2 == 0; });
    print(ra, "remove_if(even)     ");

    // ──── reverse ────
    std::list<int> rv{1,2,3,4};
    rv.reverse();
    print(rv, "reverse             ");

    // ──── unique ────
    // 移除「連續」重複元素 (要先 sort 才能完全去重)
    std::list<int> ua{1,1,2,3,3,3,4,1,1};
    ua.unique();
    print(ua, "unique (連續去重)   ");

    // ──── sort ────
    // ★ 必須用 list::sort,不能 std::sort (因為非 random access)
    std::list<int> so{3,1,4,1,5,9,2,6};
    so.sort();
    print(so, "sort()              ");
    so.sort(std::greater<int>{});
    print(so, "sort(greater)       ");

    // ========================================================================
    //  8. 比較與 C++20 free functions
    // ========================================================================
    std::list<int> c1{1,2,3}, c2{1,2,4};
    std::cout << "\n[比較] c1 < c2 ? " << (c1 < c2) << '\n';

    std::list<int> n1{1,2,3,4,5,6};
    std::erase_if(n1, [](int x){ return x % 2 == 0; });
    print(n1, "erase_if(even)      ");

    // ========================================================================
    //  9. 常見陷阱 (Pitfalls)
    // ========================================================================
    //
    //  (1) 不能用 std::sort
    //      std::sort 需要 random access iterator,改用 list::sort()。
    //
    //  (2) 不能 it + n / it - n
    //      bidirectional iterator 只能 ++/--,要前進多步用 std::advance()。
    //
    //  (3) cache 不友善
    //      節點散佈在 heap 各處,遍歷比 vector 慢數倍以上。
    //      除非元素拷貝成本很大且需要中間 insert/erase,否則優先 vector。
    //
    //  (4) splice 與 merge 的前後 list 必須屬於同一個 allocator
    //      否則行為未定義。
    //
    //  (5) 不要忘了 size 是 O(1) (從 C++11 起)
    //      但每次 splice 部分區間時,新舊兩個 list 的 size 都要重算 O(n)。
    //
    //  (6) 元素的 reference / iterator 在 splice 後仍然有效
    //      這是 list / forward_list 獨有的「節點生命週期不變」特性。

    // ========================================================================
    //  10. 最佳實踐
    // ========================================================================
    //
    //  • 真正需要中間頻繁 insert/erase 才用 list
    //  • 想要 splice 的零拷貝合併 → list 是唯一選擇 (forward_list 也支援)
    //  • 一般情況下 vector 通常比 list 快很多 — 別反射性地用 list
    //  • 不需要反向 iterator 且想省空間 → 用 forward_list

    // ========================================================================
    //  11. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // std::list 在面試題的最重要應用是「LRU Cache (LC 146)」 — 將 list 配合
    // unordered_map,可在 O(1) 時間實現任意位置刪除 + 插入到頭部的雙向組合。

    // ──── LC 146: LRU Cache (最近最少使用快取) ────
    // 用 list<pair<key,value>> + unordered_map<key, list_iterator> 實現:
    //   • get(key)      : O(1) 找到節點,splice 到 list 最前 (標記為「最近用」)
    //   • put(key,val)  : O(1) 插入或更新,超出容量則 pop_back (淘汰最久未用)
    // 關鍵: list 的 splice / iterator 在搬移後仍然有效,所以 hashmap 存的
    //       iterator 永遠指向同一個節點,不會失效。這就是必須用 list 的原因。
    {
        struct LRUCache {
            int cap;
            std::list<std::pair<int,int>> dll;   // front = 最近用,back = 最久沒用
            std::unordered_map<int, std::list<std::pair<int,int>>::iterator> mp;
            explicit LRUCache(int c) : cap(c) {}
            int get(int key) {
                auto it = mp.find(key);
                if (it == mp.end()) return -1;
                dll.splice(dll.begin(), dll, it->second);   // 搬到最前
                return it->second->second;
            }
            void put(int key, int val) {
                auto it = mp.find(key);
                if (it != mp.end()) {
                    it->second->second = val;
                    dll.splice(dll.begin(), dll, it->second);
                    return;
                }
                if ((int)dll.size() == cap) {
                    mp.erase(dll.back().first);
                    dll.pop_back();
                }
                dll.emplace_front(key, val);
                mp[key] = dll.begin();
            }
        };
        LRUCache c(2);
        c.put(1, 1);
        c.put(2, 2);
        std::cout << "\n[LC146 LRU] get(1)=" << c.get(1) << '\n';   // 1
        c.put(3, 3);                                                 // 淘汰 key=2
        std::cout << "[LC146 LRU] get(2)=" << c.get(2) << '\n';     // -1
        c.put(4, 4);                                                 // 淘汰 key=1
        std::cout << "[LC146 LRU] get(1)=" << c.get(1) << '\n';     // -1
        std::cout << "[LC146 LRU] get(3)=" << c.get(3) << '\n';     // 3
        std::cout << "[LC146 LRU] get(4)=" << c.get(4) << '\n';     // 4
    }

    // ──── 切割合併 demo: 把奇數 splice 到另一個 list ────
    // 展示 list::splice 的「O(1) 零拷貝搬移」威力 — 對 vector 來說這要 O(n)。
    {
        std::list<int> all{1, 2, 3, 4, 5, 6, 7, 8};
        std::list<int> odds;
        for (auto it = all.begin(); it != all.end(); ) {
            if (*it % 2 == 1) {
                auto next_it = std::next(it);
                odds.splice(odds.end(), all, it);    // O(1) 搬移節點
                it = next_it;
            } else {
                ++it;
            }
        }
        print(all, "[splice] evens left ");
        print(odds, "[splice] odds moved ");
        // 預期輸出:
        //   [splice] evens left : [ 2 4 6 8 ] (size=4)
        //   [splice] odds moved : [ 1 3 5 7 ] (size=4)
    }

    // ──── LC 21: Merge Two Sorted Lists (合併兩個已排序鏈結串列) ────
    // 用 std::list::merge 一行解決,展示 list 內建合併運算的便利性。
    // list::merge 要求兩個 list 已排序,合併後是穩定且原地的 (O(n))。
    // 為何用 list:這是教科書級「不需要 random access」的順序合併,合併過程僅
    //              改節點 next 指標,完全不拷貝元素。
    {
        std::list<int> a{1, 3, 5, 7};
        std::list<int> b{2, 4, 6, 8};
        a.merge(b);                  // b 變空,a 為合併結果
        print(a, "[LC21 Merge a]     ");
        print(b, "[LC21 Merge b]     ");
        // 預期輸出:
        //   a: [ 1 2 3 4 5 6 7 8 ] (size=8)
        //   b: [  ] (size=0)
    }

    // ========================================================================
    //  12. 實戰範例:任務排程器 (Round-Robin Task Queue)
    // ========================================================================
    // 真實場景:作業系統的 round-robin 排程器要把目前 task 從佇列前頭移到尾端,
    // 並在執行過程中可能「插隊」新任務或「踢掉」某個 task。需求:
    //   • 從頭部取出當前任務 → pop_front
    //   • 任務未完成,放回尾部 → push_back
    //   • 中途取消某個任務 → erase (用 iterator,O(1))
    // list 的「O(1) 任意位置刪除 + iterator 不失效」特性,讓 cancel 操作很乾淨。
    {
        std::list<std::string> tasks{"taskA", "taskB", "taskC", "taskD"};
        // 假設我們先記下 taskC 的 iterator,稍後要取消
        auto it_cancel = std::next(tasks.begin(), 2);

        // 模擬執行一輪:把 taskA 移到尾端 (尚未做完)
        tasks.splice(tasks.end(), tasks, tasks.begin());

        // 取消 taskC — 注意 it_cancel 在前面的 splice 之後仍然有效!
        tasks.erase(it_cancel);

        std::cout << "[Round-Robin] 排程剩下: [ ";
        for (auto& t : tasks) std::cout << t << ' ';
        std::cout << "]\n";
        // 預期輸出: [ taskB taskD taskA ]
    }

    std::cout << "\n=== list demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：list::splice 為何能保證 O(1) 把另一個 list 整段搬過來?
    //    A：list 是 doubly linked list,splice 只需要修改幾個 prev/next 指標,
    //       不需要拷貝、不需要配置記憶體;兩個 list 共用 allocator 時甚至不用更動
    //       節點所有權。這是 list 相對 vector 唯一壓倒性的優勢。
    //
    //  Q2：list 任意位置 insert 是 O(1),那為何實務上常輸給 vector?
    //    A：「找到位置」本身要 O(n);加上每個元素獨立 heap 配置 (cache miss 嚴重),
    //       走訪速度通常比 vector 慢 5~10 倍。除非你能保留 iterator 或要做 splice,
    //       否則 vector 幾乎都贏。
    //
    //  Q3：list 的 iterator 在 erase 時失效規則為何?和 vector 差在哪?
    //    A：list 只「失效被刪除元素的 iterator」,其他 iterator 完全有效;這也是
    //       為何 LRU Cache 通常用 list + unordered_map<key, list::iterator>。
    //       vector 則是「erase 位置之後 (含) 的 iterator 全部失效」。
    //
    return 0;
}

/*
============================================================================
  附錄:std::list 完整 member function 一覽
============================================================================
  Constructors / destructor / operator=
  assign / assign_range (C++23) / get_allocator

  Element access:  front, back

  Iterators:       begin/end/cbegin/cend/rbegin/rend/crbegin/crend

  Capacity:        empty, size, max_size

  Modifiers:       clear, insert, insert_range (C++23), emplace, erase,
                   push_front, emplace_front, pop_front,
                   push_back,  emplace_back,  pop_back,
                   prepend_range / append_range (C++23),
                   resize, swap

  List operations:
      merge, splice, remove, remove_if, reverse, unique, sort

  Non-member:      operator==, !=, <, <=, >, >=, <=> (C++20)
                   std::swap, std::erase, std::erase_if (C++20)
============================================================================
*/
