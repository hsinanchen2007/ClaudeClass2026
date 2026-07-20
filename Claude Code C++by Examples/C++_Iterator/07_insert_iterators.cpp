// =============================================================================
//  07_insert_iterators.cpp  —  插入迭代器三兄弟
// =============================================================================
//  類別：
//    std::back_insert_iterator     ← std::back_inserter(c)   呼叫 c.push_back
//    std::front_insert_iterator    ← std::front_inserter(c)  呼叫 c.push_front
//    std::insert_iterator          ← std::inserter(c, pos)   呼叫 c.insert(pos, x)
//
//  全部都是 OutputIterator (寫入 + 單向)，不能讀。
//
//  為什麼要它們？
//   * std::copy(src, dst) 的 dst 是「逐個 *dst++ = x」，
//     所以 dst 一定要事先有足夠空間。用 inserter 系列就能「一邊寫一邊長大」。
//
//  容器需要的成員函式：
//     back_inserter   要求容器有 push_back   → vector / list / deque / string
//     front_inserter  要求容器有 push_front  → list / deque / forward_list
//                     (vector / string 沒有 push_front!)
//     inserter        要求容器有 insert(pos,x) → 幾乎所有容器 (set / map 也行)
//
//  陷阱：
//   * 對 std::set 用 back_inserter 不會編譯通過 (set 沒有 push_back)。
//     對 set 要用 std::inserter(s, s.end())。
//   * inserter 是「在 pos 前插入」，每寫一筆 pos 會自動往前移。
//
//  參考連結 (cppreference / cplusplus)：
//    https://en.cppreference.com/w/cpp/iterator/back_insert_iterator   — back
//    https://en.cppreference.com/w/cpp/iterator/back_inserter          — 工廠
//    https://en.cppreference.com/w/cpp/iterator/front_insert_iterator  — front
//    https://en.cppreference.com/w/cpp/iterator/front_inserter         — 工廠
//    https://en.cppreference.com/w/cpp/iterator/insert_iterator        — insert
//    https://en.cppreference.com/w/cpp/iterator/inserter               — 工廠
//    https://cplusplus.com/reference/iterator/                         — 簡明
// =============================================================================

/*
補充筆記：insert_iterators
  - insert_iterators 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - back_inserter 呼叫 push_back，front_inserter 呼叫 push_front，inserter 在指定位置插入。
  - insert iterator 讓演算法能寫入空容器，而不是要求目的範圍已經有足夠大小。
  - front_inserter 會反轉插入順序，因為每次都插在最前面。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】insert iterators
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. back_inserter、front_inserter、inserter 三者差在哪？
//     答：都是 insert iterator（output iterator adaptor），把「賦值」轉譯成「插入容器」：
//         std::back_inserter(c) 呼叫 c.push_back()；std::front_inserter(c) 呼叫
//         c.push_front()（vector 沒有這個成員，不能用）；std::inserter(c, pos) 呼叫
//         c.insert(pos, ...)，關聯容器要用這個。
//     追問：對 std::set 該用哪一個？（inserter；set 沒有 push_back / push_front）
//
// 🔥 Q2. 為什麼需要 insert iterator？直接 std::copy 到 dst.begin() 不行嗎？
//     答：不行。演算法只拿到 iterator，不知道容器型別，也就無法改變容器大小；往空的或
//         容量不足的 dst 寫是 UB。insert iterator 正是用來補上「邊寫邊長大」這件事：
//         std::copy(src.begin(), src.end(), std::back_inserter(dst));
//     追問：那先 dst.resize(n) 再 copy 可不可以？（可以，而且少了逐次插入的開銷；但要
//         自己算對 n）
//
// ⚠️ 陷阱. std::copy 搭 front_inserter，結果順序為什麼是反的？
//     答：因為每一筆都插到最前面，後寫入的會壓在前面，最終順序與來源相反。要保持原順序
//         就用 back_inserter，或改用 inserter 指定位置。
//     為什麼會錯：把 front_inserter 想成「從前面開始依序填入」，實際上它每次都插在
//         容器的最前端，不會隨著寫入而往後移動。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <vector>
#include <list>
#include <set>
#include <algorithm>
#include <string>

template <class C>
void print(const std::string& tag, const C& c) {
    std::cout << tag << ": ";
    for (const auto& x : c) std::cout << x << ' ';
    std::cout << '\n';
}

int main() {
    std::vector<int> src = {1, 2, 3};

    // -----------------------------------------------------------------------
    // 範例 1：back_inserter
    // -----------------------------------------------------------------------
    std::vector<int> a;
    std::copy(src.begin(), src.end(), std::back_inserter(a));   // → 1 2 3
    print("back_inserter", a);

    // -----------------------------------------------------------------------
    // 範例 2：front_inserter
    //   注意：front_inserter 每次都從前面塞 → 結果順序會反過來！
    // -----------------------------------------------------------------------
    std::list<int> b;
    std::copy(src.begin(), src.end(), std::front_inserter(b));  // → 3 2 1
    print("front_inserter", b);

    // -----------------------------------------------------------------------
    // 範例 3：inserter — 在中間插入
    // -----------------------------------------------------------------------
    std::vector<int> c = {10, 20, 30};
    auto pos = c.begin() + 1;        // 指向 20
    std::copy(src.begin(), src.end(), std::inserter(c, pos));
    // 預期: 10 1 2 3 20 30
    print("inserter (中間)", c);

    // -----------------------------------------------------------------------
    // 範例 4：inserter 配 std::set (set 沒有 push_back!)
    // -----------------------------------------------------------------------
    std::set<int> s;
    std::vector<int> nums = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    std::copy(nums.begin(), nums.end(), std::inserter(s, s.end()));
    print("set 自動去重排序", s);    // 1 2 3 4 5 6 9

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 1：
    //   LC 1002. Find Common Characters
    //   兩個 set 取交集 → std::set_intersection 的目的端用 inserter
    // -----------------------------------------------------------------------
    std::set<int> A = {1, 2, 3, 4, 5};
    std::set<int> B = {3, 4, 5, 6, 7};
    std::set<int> inter;
    std::set_intersection(A.begin(), A.end(),
                          B.begin(), B.end(),
                          std::inserter(inter, inter.end()));
    print("intersection", inter);    // 3 4 5

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 2：
    //   LC 217. Contains Duplicate (用 inserter 把元素塞 set，邊塞邊判重)
    // -----------------------------------------------------------------------
    auto contains_duplicate = [](const std::vector<int>& v) {
        std::set<int> seen;
        for (int x : v) {
            // insert 回傳 pair<iterator, bool>，bool 是「是否真的插入」
            if (!seen.insert(x).second) return true;
        }
        return false;
    };
    std::cout << std::boolalpha
              << "contains dup [1,2,3,1] = " << contains_duplicate({1,2,3,1}) << '\n'
              << "contains dup [1,2,3,4] = " << contains_duplicate({1,2,3,4}) << '\n';

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 3：
    //   LC 349. Intersection of Two Arrays
    //   * 給兩個陣列，回傳「都出現過」的元素，不重複。
    //   * 想法：先把兩邊都丟進 set 去重排序，再用 set_intersection 取交集。
    //   * 這把「插入迭代器」用得很扎實：
    //       - 把 vector 灌進 set 用 std::inserter (因為 set 沒有 push_back)
    //       - 把交集結果接到 vector 用 std::back_inserter
    //   * 重點：演算法本身不需要關心容器類型，全靠 inserter 系列「橋接」。
    // -----------------------------------------------------------------------
    auto intersection_of_arrays = [](std::vector<int> n1, std::vector<int> n2) {
        std::set<int> s1, s2;
        std::copy(n1.begin(), n1.end(), std::inserter(s1, s1.end()));
        std::copy(n2.begin(), n2.end(), std::inserter(s2, s2.end()));
        std::vector<int> ans;
        std::set_intersection(s1.begin(), s1.end(),
                              s2.begin(), s2.end(),
                              std::back_inserter(ans));
        return ans;
    };
    auto inter_ans = intersection_of_arrays({4,9,5}, {9,4,9,8,4});
    std::cout << "intersection LC349: ";
    for (int x : inter_ans) std::cout << x << ' ';   // 4 9
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // LeetCode 風格示範 4：
    //   LC 2215. Find the Difference of Two Arrays
    //   * 給 nums1, nums2，回傳 [A\B, B\A] (各自去重)。
    //   * 想法：兩邊先丟進 set 自動去重排序，再用 std::set_difference 兩次。
    //   * 為什麼這題對應「插入迭代器」？
    //       - std::set_difference 的目的端是 OutputIterator，
    //         我們要把結果寫進「會自動長大」的 vector → 必用 back_inserter。
    //       - vector 沒有預先 reserve 空間，演算法不會幫你 push_back，
    //         少了 back_inserter 就會 segfault (寫入未配置記憶體)。
    //   * 跟示範 1 (set_intersection) 是雙胞胎：演算法不同，但 inserter 用法一致。
    //     這正是 STL 的核心美學 — 容器、演算法、iterator 三者完全解耦.
    // -----------------------------------------------------------------------
    auto find_difference = [](std::vector<int> n1, std::vector<int> n2) {
        std::set<int> s1(n1.begin(), n1.end());
        std::set<int> s2(n2.begin(), n2.end());
        std::vector<std::vector<int>> ans(2);
        // A \ B：在 s1 但不在 s2
        std::set_difference(s1.begin(), s1.end(),
                            s2.begin(), s2.end(),
                            std::back_inserter(ans[0]));
        // B \ A：在 s2 但不在 s1
        std::set_difference(s2.begin(), s2.end(),
                            s1.begin(), s1.end(),
                            std::back_inserter(ans[1]));
        return ans;
    };
    auto diff_ans = find_difference({1,2,3}, {2,4,6});
    std::cout << "LC2215 A\\B: ";
    for (int x : diff_ans[0]) std::cout << x << ' ';   // 1 3
    std::cout << '\n';
    std::cout << "LC2215 B\\A: ";
    for (int x : diff_ans[1]) std::cout << x << ' ';   // 4 6
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 課程知識補充：std::inserter 「在 pos 前插」 的含意
    //   * 三個 inserter 的「方向感」很常被搞混：
    //       back_inserter(c)        → 永遠 push_back   → 結果順序「同來源」
    //       front_inserter(c)       → 永遠 push_front  → 結果順序「反過來」
    //       inserter(c, pos)        → 在 pos 「之前」插入；插完後 pos 自動往後挪一格
    //                                   → 結果順序「同來源」(連續插在 pos 前)
    //
    //   * 換句話說：inserter(c, c.begin()) 的順序「同來源」(不是反過來！)
    //     因為每寫一筆，pos 就指向「剛寫的下一個」位置。這跟 front_inserter 不同！
    //
    //   * 對 set/map 用 inserter(c, c.end()) 是常見慣用語：
    //     set 是排序樹，「在 end() 前插」其實就是「插到正確位置」(由 set 自己決定)。
    //     傳 c.end() 是「給樹一個 hint」，讓插入接近 amortized O(1).
    // -----------------------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：back_inserter / front_inserter / inserter 三者「方向感」差在哪？
    //    A：back_inserter 永遠 push_back → 結果順序「同來源」。
    //      front_inserter 永遠 push_front → 後來的會擠到前面 → 結果「反過來」。
    //      inserter(c, pos) 在 pos「之前」插入，但每寫完一筆 pos 自動往後挪一格，
    //      所以連續插入仍是「同來源順序」(這跟 front_inserter 不同，常被搞混)。
    //
    //  Q2：為什麼對 std::set 不能用 back_inserter，而要用 inserter(s, s.end())？
    //    A：back_inserter 要求容器有 push_back，但 set 的位置由排序樹決定，沒有
    //      push_back / push_front 概念。set 的 insert(hint, x) 會把 x 插到正確
    //      位置，hint 只是給樹一個「從這附近找」的提示 — 傳 s.end() 是常見慣用語，
    //      若資料已升序時可達到 amortized O(1)。
    //
    //  Q3：insert_iterator 屬於哪個 iterator 等級？
    //    A：跟其他兩兄弟一樣都是 OutputIterator (只能寫、單向、不需 ==)。
    //      實作上 *it 與 ++it 都是 no-op 回傳自己，真正動作藏在 operator= 裡，
    //      呼叫容器的 insert(pos, x)。等級這麼低也是設計意圖 — 演算法看不到容器
    //      類型，只負責「寫」，容器要 push_back 還是 insert 是 inserter 的事。
    //

    // -----------------------------------------------------------------------
    // LC 範例: LC 1002 — Find Common Characters (找出所有字串共同字元)
    // -----------------------------------------------------------------------
    // 給字串陣列,回傳「每個字串都有」的字元 (含重複次數)。
    // 思路:對每個字串建一個字元計數 (multiset),逐字串取「交集」 (set_intersection)。
    // 為何用 insert_iterator:set_intersection 把結果寫到一個會動態長大的 vector,
    // 必須用 back_inserter 才不會踩到「未配置記憶體」。
    {
        std::vector<std::string> words{"bella","label","roller"};
        // 把第一個字串排序後當基底
        std::string common(words[0]);
        std::sort(common.begin(), common.end());
        for (size_t i = 1; i < words.size(); ++i) {
            std::string s = words[i];
            std::sort(s.begin(), s.end());
            std::string tmp;
            std::set_intersection(common.begin(), common.end(),
                                  s.begin(), s.end(),
                                  std::back_inserter(tmp));    // ★ 重點
            common = std::move(tmp);
        }
        std::cout << "LC1002 common chars: ";
        for (char c : common) std::cout << c << ' ';
        std::cout << "(期望 e l l)\n";
    }

    // -----------------------------------------------------------------------
    // 實戰範例:Web Server 收 form 資料,filter 後寫到 DB buffer
    // -----------------------------------------------------------------------
    // 場景:HTTP form 帶來一堆欄位 (key-value),要過濾掉 admin 開頭的內部欄位,
    // 然後分別寫到「DB insert buffer (vector)」和「session set (set)」兩個容器。
    // 同樣一段 copy_if + 不同 inserter,展示 STL「演算法不變、目的容器隨意換」的威力。
    {
        std::vector<std::string> form{
            "user_name", "user_age", "admin_token", "user_email", "admin_role"
        };
        auto not_admin = [](const std::string& s){
            return s.rfind("admin_", 0) != 0;    // 不是 admin_ 開頭
        };
        // 目的 1: 寫到 vector (DB insert buffer)
        std::vector<std::string> db_buf;
        std::copy_if(form.begin(), form.end(),
                     std::back_inserter(db_buf), not_admin);
        // 目的 2: 寫到 set (session 去重)
        std::set<std::string> session_set;
        std::copy_if(form.begin(), form.end(),
                     std::inserter(session_set, session_set.end()), not_admin);
        std::cout << "DB buf: ";
        for (auto& s : db_buf) std::cout << s << ' ';
        std::cout << "\nSession set: ";
        for (auto& s : session_set) std::cout << s << ' ';
        std::cout << '\n';
        // 預期: DB buf 順序保持輸入順序;set 自動排序
    }

    return 0;
}
