// =============================================================================
//  03_forward_iterator.cpp  —  ForwardIterator
// =============================================================================
//  特性 (LegacyForwardIterator)：
//   * 同時滿足 InputIterator 與 OutputIterator (若指向的是非 const 元素)。
//   * 「可多次走訪 (multi-pass)」：
//       it1 == it2  比較
//       拷貝出去後，兩個獨立的 iterator 走過相同序列會得到相同結果。
//   * 仍然「只能往前走 (++)」，不能 --。
//
//  典型容器：
//   * std::forward_list<T>           (單向 linked list)
//   * std::unordered_set / map       (雜湊表，bucket 內單向走訪)
//
//  能做但 InputIterator 做不到的事：
//   * 兩遍走訪：例如 std::adjacent_find、std::search、std::unique。
//
//  陷阱：
//   * forward_list 沒有 size()，因為 O(n) 會違反 STL「成員函式 O(1)」的精神，
//     需要時請用 std::distance(begin, end) (O(n))。
//   * forward_list 沒有 push_back / back / 一般的 insert，
//     插入要用 insert_after、erase_after。
//
//  參考連結 (cppreference / cplusplus)：
//    https://en.cppreference.com/w/cpp/named_req/ForwardIterator   — 規格定義
//    https://en.cppreference.com/w/cpp/container/forward_list       — 典型容器
//    https://cplusplus.com/reference/iterator/ForwardIterator/      — 簡明說明
// =============================================================================

/*
補充筆記：forward_iterator
  - forward_iterator 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - forward iterator 可多次走訪同一範圍，支援 multi-pass guarantee。
  - forward_list 的 iterator 是 forward iterator，因此不能往回走，也不能隨機跳。
  - 需要多次掃描但不需倒退的演算法，可把需求降到 forward iterator。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】ForwardIterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. ForwardIterator 比 InputIterator 多了什麼保證？
//     答：multi-pass guarantee——可以複製 iterator，兩份各自往前走會得到相同的序列，也能
//         先記住一個位置、之後再回來解參考。此外 forward iterator 要求 default
//         constructible，且 *it 回傳真正的 reference（T& 或 const T&），不是暫存值。
//     追問：這個保證讓哪些演算法成為可能？（需要走兩遍或回頭比對的，如 std::search、
//         std::adjacent_find、std::unique）
//
// 🔥 Q2. 哪些標準容器的 iterator 只到 forward 這一級？為什麼？
//     答：forward_list 和所有 unordered_* 容器。原因是底層都是 singly linked list：節點
//         只有 next 指標，沒有 prev，天生無法反向走訪，所以沒有 operator--，也就沒有
//         rbegin() / rend()。
//     追問：那 std::advance(it, -1) 對 unordered_map 的 iterator 可以嗎？（不行，category
//         不符）／std::reverse 可以用嗎？（不行，它需要 bidirectional）
//
// Q3. Forward iterator 一定是唯讀的嗎？
//     答：不是。forward iterator 分 mutable 與 constant 兩種：前者 *it 回傳 T&，可以寫；
//         後者回傳 const T&。set 的 iterator 是後者（改了 key 會破壞排序不變式）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <forward_list>
#include <iterator>
#include <algorithm>
#include <unordered_set>
#include <string>
#include <vector>
#include <cstdlib>    // std::abs (毛刺偵測)

int main() {
    // -----------------------------------------------------------------------
    // 範例 1：forward_list 的基本走訪
    // -----------------------------------------------------------------------
    std::forward_list<int> fl = {1, 2, 2, 3, 4, 4, 4, 5};

    std::cout << "forward_list: ";
    for (auto it = fl.begin(); it != fl.end(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // size() 不存在 → 自己用 distance (注意：O(n))
    auto n = std::distance(fl.begin(), fl.end());
    std::cout << "size = " << n << '\n';

    // -----------------------------------------------------------------------
    // 範例 2：std::adjacent_find — 需要 multi-pass，所以 ForwardIterator 即可
    //   找出第一個「與下一個相鄰元素相等」的位置
    // -----------------------------------------------------------------------
    auto dup = std::adjacent_find(fl.begin(), fl.end());
    if (dup != fl.end())
        std::cout << "first adjacent dup = " << *dup << " (期望 2)\n";

    // -----------------------------------------------------------------------
    // 範例 3：在 forward_list 內就地反轉 (其實 forward_list::reverse() 已內建)
    //   注意：std::reverse 需要 BidirectionalIterator，所以不能直接套在 fl 上！
    //   這提醒我們：iterator 類別 = 演算法的「最低門檻」。
    // -----------------------------------------------------------------------
    fl.reverse();   // 用容器自帶成員函式
    std::cout << "reversed: ";
    for (int x : fl) std::cout << x << ' ';
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // LeetCode 風格示範：
    //   LC 141. Linked List Cycle (Floyd 龜兔賽跑)
    //   ListNode* 的「next 指標」就是 forward iterator 的本質：只能往前走。
    // -----------------------------------------------------------------------
    struct ListNode { int val; ListNode* next; ListNode(int v):val(v),next(nullptr){} };

    auto has_cycle = [](ListNode* head) {
        ListNode* slow = head;
        ListNode* fast = head;
        while (fast && fast->next) {
            slow = slow->next;          // 走 1 步 (forward only)
            fast = fast->next->next;    // 走 2 步
            if (slow == fast) return true;
        }
        return false;
    };

    // 建一個有環的鏈表 1→2→3→4→2 (4 連回 2)
    ListNode* a = new ListNode(1);
    a->next = new ListNode(2);
    a->next->next = new ListNode(3);
    a->next->next->next = new ListNode(4);
    a->next->next->next->next = a->next;
    std::cout << "has cycle? " << std::boolalpha << has_cycle(a) << '\n';

    // 釋放 (打斷環之後逐一 delete)
    a->next->next->next->next = nullptr;
    while (a) { auto* tmp = a; a = a->next; delete tmp; }

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 2：
    //   LC 206. Reverse Linked List
    //   * 把單向鏈表反轉。「單向」= forward only = ForwardIterator 等級。
    //   * 反轉的關鍵：你只能往前走，所以必須「在走每一步時就把指標倒回來」。
    //     需要三個指標：prev / curr / next_tmp，這個技巧很值得記。
    // -----------------------------------------------------------------------
    auto reverse_list = [](ListNode* head) {
        ListNode* prev = nullptr;
        ListNode* curr = head;
        while (curr) {
            ListNode* next_tmp = curr->next; // 先存下「下一站」(因為馬上要改 curr->next)
            curr->next = prev;                // 把目前節點的指針反向
            prev = curr;                      // 推進 prev
            curr = next_tmp;                  // 推進 curr
        }
        return prev;                          // prev 此時指向新的頭
    };

    // 建一個 1→2→3→4→5
    ListNode* h = new ListNode(1);
    h->next = new ListNode(2);
    h->next->next = new ListNode(3);
    h->next->next->next = new ListNode(4);
    h->next->next->next->next = new ListNode(5);

    ListNode* rh = reverse_list(h);
    std::cout << "reversed: ";
    for (ListNode* p = rh; p; p = p->next) std::cout << p->val << ' '; // 5 4 3 2 1
    std::cout << '\n';
    while (rh) { auto* t = rh; rh = rh->next; delete t; }

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 3：
    //   LC 14. Longest Common Prefix
    //   * 給一堆字串，回傳所有字串的最長共同前綴。
    //   * 想法：用 std::mismatch 一次比對兩字串的「前綴差異點」。
    //   * 為什麼這題對應 ForwardIterator？
    //       - std::mismatch 的需求 = ForwardIterator (要 multi-pass：
    //         逐個位置比對 *a == *b，並記住第一個不相等的位置)。
    //       - 它不需要 -- 也不需要跳躍，往前走一格就夠 → Forward 等級。
    //       - string::iterator 雖然是 RandomAccess (強於 Forward)，但這裡
    //         只用到 Forward 的能力，演算法因此能套到任何 Forward 容器.
    //   * std::mismatch 回傳一對 iterator: (a 端不相等的位置, b 端不相等的位置)，
    //     若整段相等則回傳 (a 的 last, b 對應位置).
    // -----------------------------------------------------------------------
    auto longest_common_prefix = [](const std::vector<std::string>& strs) {
        if (strs.empty()) return std::string{};
        std::string prefix = strs[0];   // 從第一個字串當起點
        for (std::size_t i = 1; i < strs.size(); ++i) {
            // 比對 prefix 與 strs[i]，找出第一個不相等的位置
            // 注意 mismatch 會自動處理「兩端長度不同」(較短者用完即停)
            auto [p_it, s_it] = std::mismatch(
                prefix.begin(), prefix.end(),
                strs[i].begin(), strs[i].end());
            // 把 prefix 截到不相等的位置 (含)
            prefix.erase(p_it, prefix.end());
            if (prefix.empty()) break;  // 已經沒有共同前綴，提早結束
        }
        return prefix;
    };
    std::cout << "LC14 LCP [\"flower\",\"flow\",\"flight\"] = \""
              << longest_common_prefix({"flower","flow","flight"}) << "\" (期望 \"fl\")\n";
    std::cout << "LC14 LCP [\"dog\",\"racecar\",\"car\"]    = \""
              << longest_common_prefix({"dog","racecar","car"}) << "\" (期望 \"\")\n";

    // -----------------------------------------------------------------------
    // 課程知識補充：「multi-pass」到底是什麼意思？
    //   * Forward 之所以比 Input 強，重點不在 ++，而在「拷貝後可以重走」：
    //         auto a = c.begin();
    //         auto b = a;                 // 拷貝
    //         while (a != c.end()) ++a;   // 走完
    //         while (b != c.end()) ++b;   // 再從頭走一次 → 結果保證一樣
    //   * 對 InputIterator 不能保證 — istream 讀過一次資料就消失了，
    //     就算你拷貝了 iterator，下次讀的東西也已經不存在了。
    //   * 為什麼 std::adjacent_find / std::unique / std::search 需要 Forward？
    //     因為它們需要在「比較成功 / 失敗」後 *記住位置回頭參考*，
    //     等同於跑了好幾次的 multi-pass.
    // -----------------------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 forward_list 沒有 size() 成員，但 list 有？
    //    A：STL 規範容器成員函式應為 O(1)。forward_list 為了「節省每節點的記憶體」
    //      故意不維護 size 欄位，要算長度只能 O(n) 走過。為了不違反「成員 O(1)」，
    //      乾脆不提供 size()。需要長度時請用 std::distance(begin, end)。list 則
    //      內部維護 size_ 變數，所以 list::size() 自 C++11 起保證 O(1)。
    //
    //  Q2：multi-pass 真正的意涵是什麼？跟「能 ++」差在哪？
    //    A：multi-pass 不是「能多走幾步」，而是「拷貝 iterator 後，兩條副本各自從
    //      同一位置出發走訪相同序列，結果保證一致」。InputIterator 雖然也能 ++，
    //      但它沒有此保證 — istream 走過的字元已消失。Forward 之後的等級都保證
    //      multi-pass，所以 std::search / std::adjacent_find 才能「往前看一格、
    //      不對就回退繼續找」。
    //
    //  Q3：unordered_set / unordered_map 的 iterator 為什麼只到 Forward 等級？
    //    A：雜湊容器底層是「bucket 陣列 + 每 bucket 一個 linked list」。走訪時
    //      要先找到下一個非空 bucket、再往該 bucket 的 list 內部走 — 全程只能
    //      單向往前，沒有反向走訪的順序保證 (順序由 hash 決定，沒有「前一個」的
    //      自然概念)。因此標準把它定為 ForwardIterator，不能用 std::reverse 或
    //      reverse_iterator。
    //

    // -----------------------------------------------------------------------
    // LC 範例: LC 83 — Remove Duplicates from Sorted List (移除排序串列重複項)
    // -----------------------------------------------------------------------
    // 給已排序的單向鏈結串列,移除重複節點。
    // 為何對 ForwardIterator 完美:整段只需 ++ 與 *,而且只走一遍 — 完全契合
    // ForwardIterator 的最小能力集。底層用 forward_list,跟原題的單向鏈結結構吻合。
    // 演算法:逐節點比較 *it 與 *next(it),若相等就 erase_after。
    {
        std::forward_list<int> fl{1, 1, 2, 3, 3, 4};
        for (auto it = fl.begin(); it != fl.end(); ) {
            auto nxt = std::next(it);
            if (nxt != fl.end() && *nxt == *it) {
                fl.erase_after(it);   // 刪除 nxt;it 不動,下一輪繼續比
            } else {
                ++it;
            }
        }
        std::cout << "LC83 dedup sorted: ";
        for (int x : fl) std::cout << x << ' ';
        std::cout << '\n';
        // 預期輸出: 1 2 3 4
    }

    // -----------------------------------------------------------------------
    // 實戰範例:單向鏈結資料的「滑動兩元素」分析
    // -----------------------------------------------------------------------
    // 場景:即時資料 (網路封包、感測讀數) 以單向鏈結結構到達,需要計算「相鄰兩筆
    // 差異」做毛刺偵測。std::adjacent_find 與 lambda 完美對應 — 它的需求就是
    // ForwardIterator (要 multi-pass 比較 *it 與 *next(it))。
    {
        std::forward_list<int> readings{20, 21, 20, 35, 36, 22};   // 35 是毛刺
        // 找出「下一筆與本筆差距 > 10」的位置
        auto it = std::adjacent_find(readings.begin(), readings.end(),
            [](int a, int b){ return std::abs(b - a) > 10; });
        if (it != readings.end())
            std::cout << "毛刺偵測:" << *it << " -> " << *std::next(it) << '\n';
        else
            std::cout << "無毛刺\n";
        // 預期輸出: 毛刺偵測:20 -> 35
    }

    return 0;
}
