// =============================================================================
//  11_iterator_invalidation.cpp  —  Iterator 失效規則 (Iterator Invalidation)
// =============================================================================
//  「失效 (invalidation)」= 一個 iterator/pointer/reference 原本指向有意義的
//  元素，但容器發生某些操作後，這個 iterator 變得「不能再用」(用了 = UB)。
//
//  C++ 工程師最常踩的雷之一就是：在 for-loop 裡 erase 元素，
//  造成 iterator 失效，後續 ++it 直接 UB。
//
//  各容器失效規則 (記住「不變」的部分通常更實用)：
//
//  ┌─────────────────────────┬──────────────────────────────────────────────┐
//  │ vector / string         │ 任何 reallocation (push_back 超過 capacity、 │
//  │                         │   reserve、resize 變大) → 全部失效            │
//  │                         │ insert/erase(pos) → pos 起到 end() 之前全失效 │
//  │                         │ clear → 全失效                                │
//  │                         │ size 不變且不 reallocation → 不失效            │
//  ├─────────────────────────┼──────────────────────────────────────────────┤
//  │ deque                   │ insert/erase 中間 → 通常全部失效             │
//  │                         │ push_front/push_back → iterator 全失效，       │
//  │                         │   但 reference 仍有效 (奇特特例！)           │
//  │                         │ pop_front/pop_back → 只「被刪那端」失效       │
//  ├─────────────────────────┼──────────────────────────────────────────────┤
//  │ list / forward_list /   │ insert / push_* → 完全不失效                 │
//  │ map / set / multimap /  │ erase(pos)    → 只「pos」這一個失效，其他都安全│
//  │ multiset                │ splice (list) → 不失效 (節點被搬走但仍有效)   │
//  ├─────────────────────────┼──────────────────────────────────────────────┤
//  │ unordered_*             │ insert / 任何 rehash → iterator 全失效，      │
//  │                         │   但 reference/pointer 仍有效                │
//  │                         │ erase → 只「被刪那個」失效                   │
//  ├─────────────────────────┼──────────────────────────────────────────────┤
//  │ array                   │ 永遠不失效 (固定長度)                         │
//  └─────────────────────────┴──────────────────────────────────────────────┘
//
//  記憶要點：
//   * 「節點型容器 (list / set / map)」對 insert 都不失效，erase 只壞那一個。
//   * vector 是「連續型」的代價 → reallocation 一旦發生通通失效。
//   * unordered_* 是 rehash 殺手，但 ref 不會失效因為節點還在。
//
//  這個檔案用 capacity 觀察 + erase 慣用語來示範：
//
//  參考連結 (cppreference / cplusplus)：
//    https://en.cppreference.com/w/cpp/container                       — 容器總覽
//    (各容器頁面下方都有 "Iterator invalidation" 表格)
//    https://en.cppreference.com/w/cpp/algorithm/remove                — remove
//    https://en.cppreference.com/w/cpp/algorithm/unique                — unique
//    https://cplusplus.com/reference/algorithm/remove/                 — 簡明
// =============================================================================

/*
補充筆記：iterator_invalidation
  - iterator_invalidation 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - iterator invalidation 是容器修改後舊位置是否仍可用的規則，和指標生命週期一樣重要。
  - vector 擴容會讓所有 iterator/reference/pointer 失效；list 插入通常不影響其他元素 iterator。
  - erase 常回傳下一個有效 iterator，迴圈刪除元素時應使用這個回傳值接續。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】iterator invalidation
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 iterator invalidation？各容器的規則是什麼？
//     答：容器被修改後，先前取得的 iterator / pointer / reference 可能不再指向有效元素，
//         再使用即 UB。規則：
//         * vector：發生 reallocation → 全部失效；未 reallocation 時 insert / erase →
//           插入點或刪除點「及其之後」失效。
//         * deque：兩端 insert → iterator 全失效，但 reference / pointer 仍有效；中間
//           insert / erase → 全部失效；兩端 erase → 只有被刪元素失效。
//         * list / forward_list / map / set：只有「被刪除元素」的 iterator 失效，
//           insert 完全不影響任何既有 iterator。
//         * unordered_*：insert 若觸發 rehash → 所有 iterator 失效，但 reference /
//           pointer 永遠有效；erase 只影響被刪元素。
//     追問：unordered 容器「iterator 失效但 reference 有效」怎麼解釋？（separate chaining
//         ——元素放在各自獨立配置的節點上，rehash 只是重接 bucket array 與 next 指標，
//         節點本身沒有搬家）
//
// 🔥 Q2. 在迴圈中刪除元素的正確寫法？
//     答：C++11 起所有標準序列容器與關聯容器的 erase(iterator) 都回傳「下一個有效
//         iterator」，所以統一寫法是：
//             for (auto it = c.begin(); it != c.end(); ) {
//                 if (pred(*it)) it = c.erase(it);
//                 else ++it;
//             }
//         舊寫法 c.erase(it++) 是 C++98 針對 map / set / list 的技巧（當年 erase 回傳
//         void），對 vector / deque 並不成立——後續 iterator 也一併失效了。
//     追問：對 vector 逐個 erase 的複雜度？（O(n^2)，應改用 erase-remove idiom）／C++20
//         有更簡單的寫法嗎？（std::erase_if(c, pred)）
//
// ⚠️ 陷阱. 兩個不同容器的 iterator 可以互相比較或相減嗎？
//     答：不行，是 UB。iterator 的比較與相減只在同一個容器（同一序列）內有定義。即使兩個
//         vector 內容完全相同，v1.begin() == v2.begin() 仍是未定義行為；v2.end() -
//         v1.begin() 同樣無意義。要比較內容請用 c1 == c2 或 std::equal。
//     為什麼會錯：實務上這種比較通常「剛好」回傳 false 而不當掉，於是被當成可用；標準並
//         沒有這個保證。libstdc++ 開 _GLIBCXX_DEBUG 就會 assert 出來。
//
// Q3. vector 擴容時到底長大幾倍？
//     答：標準只規定 push_back 是 amortized O(1)，並「沒有」規定成長倍率——那是
//         implementation-defined：libstdc++ 約 2 倍、MSVC 約 1.5 倍。要避免中途
//         reallocation，正解是事先 reserve()，而不是去推算倍率。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <string>

int main() {
    // -----------------------------------------------------------------------
    // 範例 1：vector reallocation 造成 pointer/iterator 失效
    // -----------------------------------------------------------------------
    std::vector<int> v;
    v.reserve(2);                // capacity = 2
    v.push_back(10);
    v.push_back(20);

    int* p_old = &v[0];          // 還在 capacity 內，這個指標有效
    auto it_old = v.begin();
    (void)it_old;                // 留著名字示意「它存在」，但 realloc 後就 UB 不能用

    std::cout << "before realloc: cap=" << v.capacity()
              << " *p_old=" << *p_old << '\n';

    v.push_back(30);             // 超過 capacity → reallocation 發生
    // 此時 p_old 與 it_old 「都失效」，下面這一行是 UB，「只是示意」不要照做：
    //     std::cout << *p_old;   // ← 千萬別這樣
    int* p_new = &v[0];
    std::cout << "after  realloc: cap=" << v.capacity()
              << " p_old==p_new ? " << std::boolalpha
              << (p_old == p_new) << " (通常 false)\n";

    // -----------------------------------------------------------------------
    // 範例 2：vector erase 慣用語 — erase 會回傳「下一個有效 iterator」
    // -----------------------------------------------------------------------
    std::vector<int> v2 = {1, 2, 3, 4, 5, 6};
    for (auto it = v2.begin(); it != v2.end(); /* 不要寫 ++it */ ) {
        if (*it % 2 == 0) {
            it = v2.erase(it);   // ← erase 後用回傳值取代 it，正確！
        } else {
            ++it;
        }
    }
    std::cout << "vector after erase even: ";
    for (int x : v2) std::cout << x << ' ';     // 1 3 5
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 3：list — insert 完全不失效，erase 只壞被刪那一個
    // -----------------------------------------------------------------------
    std::list<int> lst = {10, 20, 30, 40};
    auto it_30 = std::find(lst.begin(), lst.end(), 30);

    lst.push_front(5);          // node-based → it_30 仍然有效
    lst.push_back(50);          // 還是不失效
    std::cout << "list 的 it_30 仍指向: " << *it_30 << " (期望 30)\n";

    lst.erase(it_30);           // 現在 it_30 失效，但其它的不受影響
    std::cout << "list after erase 30: ";
    for (int x : lst) std::cout << x << ' ';    // 5 10 20 40 50
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 4：map 的 erase-while-iterate 慣用語 (C++11 起 erase 回傳 next iterator)
    // -----------------------------------------------------------------------
    std::map<std::string, int> m = {
        {"apple", 1}, {"banana", 2}, {"cherry", 3}, {"date", 4}
    };
    for (auto it = m.begin(); it != m.end(); ) {
        if (it->second % 2 == 0) it = m.erase(it);   // 用回傳值前進
        else ++it;
    }
    std::cout << "map after erase even-value:\n";
    for (auto& [k, v] : m) std::cout << "  " << k << " → " << v << '\n';

    // -----------------------------------------------------------------------
    // 範例 5：unordered_map 的「reference 不失效」特性 — 寫法可以更靈活
    //   只要不 erase 該 key、不 rehash，reference 一直有效。
    // -----------------------------------------------------------------------
    // (略，僅作概念提醒)

    // -----------------------------------------------------------------------
    // LeetCode 風格示範：
    //   LC 27. Remove Element (移除指定值，回傳新長度)
    //   標準雙指針寫法，避免「邊走邊 erase」的失效坑。
    // -----------------------------------------------------------------------
    auto removeElement = [](std::vector<int>& nums, int val) {
        int k = 0;                                // 寫入位置
        for (int x : nums) {
            if (x != val) nums[k++] = x;          // 只是「覆蓋」，不真 erase
        }
        return k;                                  // 新邏輯長度
    };

    std::vector<int> nums = {3, 2, 2, 3};
    int newLen = removeElement(nums, 3);
    std::cout << "removeElement: newLen=" << newLen << "  前 " << newLen << " 項 = ";
    for (int i = 0; i < newLen; ++i) std::cout << nums[i] << ' ';
    std::cout << "(期望 2 2)\n";

    // 也可以用 std::remove + erase (erase-remove idiom)：
    std::vector<int> nums2 = {0, 1, 2, 2, 3, 0, 4, 2};
    nums2.erase(std::remove(nums2.begin(), nums2.end(), 2), nums2.end());
    std::cout << "erase-remove idiom: ";
    for (int x : nums2) std::cout << x << ' ';
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 2：
    //   LC 283. Move Zeroes
    //   * 把所有 0 移到尾巴，非 0 元素「保持原順序」。要求 in-place.
    //   * 經典「兩個寫入指針」技巧：
    //       - write 指向「下一個非零該寫入的位置」.
    //       - 第一輪：把所有非零搬到前面 (read 走訪整個陣列).
    //       - 第二輪：把 write..end 全部填 0.
    //   * 為什麼這題重要 ── 它示範「不真的 erase、純粹覆蓋」就能在 O(n) 解掉，
    //     完全避免 vector 失效問題。
    // -----------------------------------------------------------------------
    auto move_zeroes = [](std::vector<int>& v) {
        std::size_t write = 0;
        for (std::size_t read = 0; read < v.size(); ++read) {
            if (v[read] != 0) v[write++] = v[read];
        }
        for (; write < v.size(); ++write) v[write] = 0;
    };
    std::vector<int> mz = {0, 1, 0, 3, 12};
    move_zeroes(mz);
    std::cout << "move zeroes: ";
    for (int x : mz) std::cout << x << ' ';        // 1 3 12 0 0
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 課程知識補充：std::remove / std::remove_if 的「假象」
    //   * std::remove 不真的「刪除」元素 — 它只把「不要的」往尾端推，
    //     並回傳「新邏輯結尾」位置.
    //   * 真正要釋放空間請接 erase：
    //         v.erase(std::remove(v.begin(), v.end(), x), v.end());  ← 慣用語
    //   * 為什麼 STL 要這樣設計？因為演算法層級「不知道容器」，
    //     沒辦法叫 vector 縮短長度. 這個責任留給容器自己 (erase) 來做.
    //
    //   * 對應：std::unique 也只是「重排 + 回傳新尾巴」，記得搭配 erase.
    // -----------------------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：vector 跟 list 的 iterator 失效規則差在哪？
    //    A：vector 是「連續記憶體」，capacity 不夠時 push_back 會 reallocation，
    //      所有 iterator/pointer/reference 一次全失效；中間 insert/erase 也會讓
    //      pos 起到 end 之前全失效。list 是「節點型容器」，insert/push_* 永不失效，
    //      erase(pos) 只壞 pos 那一個，其餘指向其它節點的 iterator 都安全。
    //      這也是「實時更新的容器」常選 list/map 的原因。
    //
    //  Q2：deque 的 push_front / push_back 後 iterator 跟 reference 命運一樣嗎？
    //    A：不一樣，這是 deque 特有的怪異規則：「iterator 全部失效，但 reference
    //      仍有效」。原因是 push 可能加新 chunk，導致 chunk 索引表重新配置 →
    //      iterator 內部存的「(chunk_index, offset)」失準。但元素本體 (在某個
    //      chunk 內) 沒搬動，所以 reference / pointer 還能用。實務上 push 後請
    //      重新取得 iterator。
    //
    //  Q3：邊走邊 erase 為什麼正確的寫法是 it = c.erase(it)？
    //    A：因為 erase 後傳入的 it 已經失效，再 ++it 是 UB。C++11 起所有標準容器
    //      的 erase 都回傳「下一個有效 iterator」，所以慣用語是：
    //          for (auto it = c.begin(); it != c.end(); /*不要 ++it*/)
    //              if (cond) it = c.erase(it); else ++it;
    //      對 vector 還有更地道的「erase-remove idiom」：
    //          v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
    //      一次重排 + 一次裁剪，O(n) 完成。
    //

    // -----------------------------------------------------------------------
    // LC 範例: LC 80 — Remove Duplicates from Sorted Array II
    // 難度: medium
    // -----------------------------------------------------------------------
    // 已排序陣列允許每個值最多出現兩次,移除多餘的。
    // 為何在此檔示範:示範「不真 erase,只用 write/read 雙指針覆蓋」這個避開
    // iterator 失效的黃金套路。直接呼叫 erase 每筆 O(n),整體 O(n^2);用雙指針 O(n)。
    {
        std::vector<int> nums{1,1,1,2,2,3};
        int k = 0;   // 寫入位置
        for (size_t i = 0; i < nums.size(); ++i) {
            // 只要與 k-2 不同即可寫入 (允許保留兩個重複)
            if (k < 2 || nums[i] != nums[k - 2]) nums[k++] = nums[i];
        }
        nums.resize(k);
        std::cout << "LC80 result: ";
        for (int x : nums) std::cout << x << ' ';
        std::cout << "(期望 1 1 2 2 3)\n";
    }

    // -----------------------------------------------------------------------
    // 實戰範例:邊掃描 map 邊刪除「過期 session」(避開失效坑)
    // -----------------------------------------------------------------------
    // 場景:Web Server 定期清理「最後活動時間 < threshold」的 session,
    // 直接走訪 std::map<string, last_active> 時要 erase 必須注意:
    // C++11 起 map::erase(it) 回傳下一個有效 iterator,所以慣用語為 `it = erase(it)`。
    // 此寫法跨 std::map / std::set / std::list 都通用,是工程上最常見的「邊走邊刪」模式。
    {
        std::map<std::string, int> sessions{
            {"sid_A", 100}, {"sid_B", 200}, {"sid_C", 50}, {"sid_D", 300}
        };
        int threshold = 150;
        for (auto it = sessions.begin(); it != sessions.end(); ) {
            if (it->second < threshold) it = sessions.erase(it);   // 過期清掉
            else                        ++it;
        }
        std::cout << "Sessions after cleanup: ";
        for (auto& [k, v] : sessions) std::cout << k << "(" << v << ") ";
        std::cout << "(期望保留 sid_B、sid_D)\n";
    }

    return 0;
}
