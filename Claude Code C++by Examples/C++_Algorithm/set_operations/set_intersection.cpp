// ============================================================
// std::set_intersection
// 分類 (Category): Set operations on sorted ranges (有序集合運算)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/set_intersection
//   * https://cplusplus.com/reference/algorithm/set_intersection/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::set_intersection 解的問題:
//
//   「兩個『已排序』範圍的『交集』(A ∩ B) — 同時在兩邊出現的元素。」
//
// 重複元素規則:若 A 有 m 份等價元素、B 有 n 份,則輸出 min(m, n) 份 —
// 這是 multiset 語意,跟「集合」語意稍微不同。
//
// 範例:
//   a = {1, 2, 2, 2, 3},  b = {2, 2, 4}
//   intersection → {2, 2}    (min(3, 2) = 2 個 2)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼要排序?                                         │
// └────────────────────────────────────────────────────────────┘
//
// 排序後可以「兩個指針同步前進」線性掃描 — O(N + M)。
// 不排序的話需要 hash table (O(min(N, M)) 平均) 或巢狀比對 (O(N×M))。
//
// 對「資料天然就排序」(時序、字典序) 的場景,set_intersection 是最自然的選擇。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 找兩名單共同好友 (LC 349 / LC 350)
//   * 找兩份 log 共同的事件 ID
//   * 找兩個產品的「共通標籤」
//   * 集合運算 (交集 / 聯集 / 差集) 的三劍客之一
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt1, class InputIt2, class OutputIt>
//   OutputIt set_intersection(InputIt1 first1, InputIt1 last1,
//                             InputIt2 first2, InputIt2 last2,
//                             OutputIt d_first);
//
//   + 帶 Compare 的版本
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 2 × (N1 + N2) - 1 次比較 — O(N1 + N2)
//   空間: O(min(N1, N2)) (輸出)
//   需求: 兩範圍依 comp 已排序;輸出不可與輸入重疊。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 範圍必須已排序!否則結果未定。
//   2. 「multiset 語意」 — 重複保留 min(m, n) 份。要去重請先 unique。
//   3. 輸出「拷貝來自第一範圍」(若兩邊等價但物件不同,留 a 邊那份)。
//
// ============================================================

/*
補充筆記：std::set_intersection
  - std::set_intersection 是排序範圍上的集合運算；輸入必須已依同一比較器排序，否則交集、聯集、差集結果會錯。
  - 這些演算法處理的是 sorted range，不是 std::set 專用；vector 排序後也能使用。
  - set_union/set_intersection/set_difference 對重複元素有明確規則，結果數量不是單純去重後集合概念。
  - 輸出範圍要有足夠空間，或使用 back_inserter 讓容器自動追加。
  - merge/inplace_merge 合併的是兩段已排序資料；它們不會替未排序資料做完整排序。
  - 比較器必須與排序時一致；若輸入用小寫排序、運算時用大小寫不敏感比較，結果可能不符合前置條件。
  - std::set_intersection 的輸入是 sorted range；這裡的 set operation 不是只能用 std::set，而是依排序結果做線性合併。
  - std::set_intersection 對重複元素有 multiset 語意，例如交集會取兩邊出現次數的較小值，聯集會取較大值。
  - std::set_intersection 的輸出 iterator 代表寫入結果的結尾；若使用預先配置容器，結束後通常要 erase 多餘尾端。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::set_intersection
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. set_intersection 對重複元素的規則是什麼?
//     答：multiset 語意 — 某元素在 A 出現 m 次、B 出現 n 次,輸出 min(m, n) 次。
//         例如 A = {1,2,2,2,3}、B = {2,2,4} → 交集是 {2,2}。這不是數學上的
//         「集合交集」(那會只留一個 2),要純集合語意得自己先去重。
//     追問：兩個輸入都必須是 std::set 嗎?(答：不用,任何依同一 comp 排序的 range
//           都可以,例如排序過的 vector;複雜度是 O(N1 + N2) 的線性歸併)
//
// 🔥 Q2. LC 349 與 LC 350 都是「兩陣列交集」,為什麼寫法不同?
//     答：LC 350 要「出現 min(次數1, 次數2) 次」— 那就是 set_intersection 的預設
//         行為,排序後直接呼叫即可。LC 349 要「每個元素只出現一次」— 必須先對兩邊
//         各自 sort + unique,讓每個元素唯一,min(1,1) 才會等於 1。
//     追問：為什麼不乾脆一律先去重?(答：會改變語意,LC 350 那類「要數量」的需求就錯了)
//
// Q3. 輸出的元素是從哪一個範圍複製過來的?
//     答：標準規定前 min(m, n) 份是從「第一個範圍」複製。對 int 這種只看值的型別
//         沒差,但當元素是「比較鍵只看一部分欄位」的結構(例如只比 id、卻還帶著
//         payload)時,輸出帶的是 A 那邊的物件 — 這會影響結果的實際內容。
//
// ⚠️ 陷阱. 兩邊排序用的 comparator 不一致會怎樣?
//     答：UB。例如 A 用預設 less 排序、B 用大小寫不敏感的 comp 排序,或呼叫時傳的
//         comp 與當初排序用的不同,結果都不可信。
//     為什麼會錯：大家記得「要排序」,卻忘了前置條件是「依『同一個』準則排序」。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> a{1, 2, 3, 4, 5, 6};
    std::vector<int> b{2, 4, 6, 8, 10};

    // --- 範例 1: 基本交集 ---
    std::vector<int> inter;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::back_inserter(inter));
    std::cout << "intersection: ";
    for (int x : inter) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 重複元素 — 取 min(m, n) 份 ---
    std::vector<int> p{1, 2, 2, 2, 3};
    std::vector<int> q{2, 2, 4};
    std::vector<int> r;
    std::set_intersection(p.begin(), p.end(), q.begin(), q.end(),
                          std::back_inserter(r));
    std::cout << "with duplicates: ";
    for (int x : r) std::cout << x << ' ';
    std::cout << "(min(3,2)=2 個 2)\n";

    // --- 範例 3: 完全沒有交集 ---
    std::vector<int> x{1, 3, 5};
    std::vector<int> y{2, 4, 6};
    std::vector<int> none;
    std::set_intersection(x.begin(), x.end(), y.begin(), y.end(),
                          std::back_inserter(none));
    std::cout << "no intersection size = " << none.size() << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_349_intersection_two_arrays();
    void leetcode_350_intersection_two_arrays_ii();
    void practical_common_friends();
    void leetcode_1213_three_array_intersection();
    void practical_common_courses();
    leetcode_349_intersection_two_arrays();
    leetcode_350_intersection_two_arrays_ii();
    practical_common_friends();
    leetcode_1213_three_array_intersection();
    practical_common_courses();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 349: 兩個陣列的交集 (Intersection of Two Arrays)
// ----------------------------------------------------------------
// 題目:給兩個整數陣列 nums1, nums2,回傳它們的交集 (每個元素只出現一次)。
//
// 為什麼用 std::set_intersection:
//   先 sort + unique 兩邊 → 兩邊每個元素都唯一 → set_intersection 取 min(1,1) = 1 次,
//   結果「每個元素恰好出現一次」,符合 LC349 要求。
//
// 複雜度:時間 O(n log n + m log m);空間 O(n + m)。
void leetcode_349_intersection_two_arrays() {
    std::vector<int> a{1, 2, 2, 1};
    std::vector<int> b{2, 2};
    std::sort(a.begin(), a.end());
    a.erase(std::unique(a.begin(), a.end()), a.end());
    std::sort(b.begin(), b.end());
    b.erase(std::unique(b.begin(), b.end()), b.end());
    std::vector<int> out;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::back_inserter(out));
    std::cout << "LC349: ";
    for (int x : out) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 350: 兩個陣列的交集 II (Intersection of Two Arrays II)
// ----------------------------------------------------------------
// 題目:相同元素「應出現 min(出現次數1, 出現次數2) 次」 — 即 multiset 語意。
//
// 為什麼用 std::set_intersection:
//   set_intersection 對「未去重」的已排序輸入,預設行為就是 multiset 語意 —
//   題目要求剛好就是函式預設行為,排序後直接呼叫即可。
//
// 複雜度:時間 O(n log n + m log m);空間 O(min(n, m))。
void leetcode_350_intersection_two_arrays_ii() {
    std::vector<int> a{1, 2, 2, 1};
    std::vector<int> b{2, 2};
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    std::vector<int> out;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::back_inserter(out));
    std::cout << "LC350: ";
    for (int x : out) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:找兩名單的共同好友
// ----------------------------------------------------------------
// 場景:社群應用要計算「兩位使用者的共同好友」。
//      若各人的好友清單以「使用者 ID 排序」儲存,直接 set_intersection 即可。
//
// 為什麼用 std::set_intersection:
//   * 線性 O(n + m),比 hash 解法省記憶體、cache friendly。
//   * 程式語意明確 — 「找交集」一目了然。
void practical_common_friends() {
    std::vector<std::string> alice_friends{"bob", "carol", "dave", "erin"};
    std::vector<std::string> bob_friends{"alice", "carol", "erin", "frank"};
    std::sort(alice_friends.begin(), alice_friends.end());
    std::sort(bob_friends.begin(), bob_friends.end());
    std::vector<std::string> common;
    std::set_intersection(alice_friends.begin(), alice_friends.end(),
                          bob_friends.begin(), bob_friends.end(),
                          std::back_inserter(common));
    std::cout << "common friends:";
    for (const auto& f : common) std::cout << ' ' << f;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1213: 三個有序陣列的交集 (Intersection of Three Sorted Arrays)
// ----------------------------------------------------------------
// 題目:給三個已排序整數陣列,回傳「同時出現於三者」的元素。
//
// 為什麼用 std::set_intersection:
//   兩兩 intersect 兩次即可 — 結果 = A ∩ B ∩ C。
//
// 複雜度:時間 O(a + b + c);空間 O(min)。
void leetcode_1213_three_array_intersection() {
    std::vector<int> a{1, 2, 3, 4, 5};
    std::vector<int> b{1, 2, 5, 7, 9};
    std::vector<int> c{1, 3, 4, 5, 8};
    std::vector<int> ab, abc;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::back_inserter(ab));
    std::set_intersection(ab.begin(), ab.end(), c.begin(), c.end(),
                          std::back_inserter(abc));
    std::cout << "LC1213:";
    for (int x : abc) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:課程系統 — 找「兩個學生共同修過的課」
// ----------------------------------------------------------------
// 場景:課程清單已按課號排序;system 要找兩位同學「共同修過的課程」做配對推薦。
void practical_common_courses() {
    std::vector<std::string> alice{"CS101", "CS201", "MATH202", "PHY101"};
    std::vector<std::string> bob{"CS101", "CS301", "ENG101", "MATH202"};
    std::sort(alice.begin(), alice.end());
    std::sort(bob.begin(), bob.end());
    std::vector<std::string> common;
    std::set_intersection(alice.begin(), alice.end(),
                          bob.begin(), bob.end(),
                          std::back_inserter(common));
    std::cout << "common courses:";
    for (auto& c : common) std::cout << ' ' << c;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// intersection: 2 4 6
// with duplicates: 2 2 (min(3,2)=2 個 2)
// no intersection size = 0
// LC349: 2
// LC350: 2 2
// common friends: carol erin
// LC1213: 1 5
// common courses: CS101 MATH202
