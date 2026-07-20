// ============================================================
// std::set_union
// 分類 (Category): Set operations on sorted ranges (有序集合運算)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/set_union
//   * https://cplusplus.com/reference/algorithm/set_union/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::set_union 解的問題:
//
//   「兩個『已排序』範圍的『聯集』(A ∪ B) — 兩邊任一邊出現的元素都收集起來。」
//
// 重複元素規則:若 A 有 m 份等價元素、B 有 n 份,則輸出 max(m, n) 份 —
// 這是 multiset 語意,跟集合語意稍微不同。
//
// 範例:
//   a = {1, 2, 2, 3},  b = {2, 4}
//   union → {1, 2, 2, 3, 4}    (max(2, 1) = 2 個 2,不是 3 個)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、set_union vs merge 的差別                              │
// └────────────────────────────────────────────────────────────┘
//
//   * merge:     重複元素「全部保留」 (m + n 份)
//   * set_union: 重複元素只保留 max(m, n) 份 (集合語意)
//
// 範例:a = {1, 2, 2, 3},  b = {2, 4}
//   merge      → {1, 2, 2, 2, 3, 4}    (a 的 2 + b 的 2 = 3 個)
//   set_union  → {1, 2, 2, 3, 4}       (max(2, 1) = 2 個)
//
// 「我要『資料合併』(時序串接)」 → merge
// 「我要『集合聯集』(去多餘重複)」 → set_union
// 「我要『嚴格去重』」 → set_union 後再 unique;或先把兩邊各自去重再 set_union
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 合併兩份 config keys (預設鍵 + 使用者覆寫鍵)
//   * 合併兩份標籤清單
//   * 合併兩份權限清單
//   * 集合運算三劍客 (intersection / union / difference) 的「聯集」端
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt1, class InputIt2, class OutputIt>
//   OutputIt set_union(InputIt1 first1, InputIt1 last1,
//                      InputIt2 first2, InputIt2 last2,
//                      OutputIt d_first);
//
//   + 帶 Compare 的版本
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 2 × (N1 + N2) - 1 次比較 — O(N1 + N2)
//   空間: O(N1 + N2) (輸出)
//   需求: 兩範圍依 comp 已排序;輸出不可與輸入重疊。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 與 merge 不同 — 重複元素只保留 max(m, n) 份。
//   2. 範圍必須已排序。
//   3. 想嚴格去重 → 在 set_union 後再 unique,或先去重兩邊。
//
// ============================================================

/*
補充筆記：std::set_union
  - std::set_union 是排序範圍上的集合運算；輸入必須已依同一比較器排序，否則交集、聯集、差集結果會錯。
  - 這些演算法處理的是 sorted range，不是 std::set 專用；vector 排序後也能使用。
  - set_union/set_intersection/set_difference 對重複元素有明確規則，結果數量不是單純去重後集合概念。
  - 輸出範圍要有足夠空間，或使用 back_inserter 讓容器自動追加。
  - merge/inplace_merge 合併的是兩段已排序資料；它們不會替未排序資料做完整排序。
  - 比較器必須與排序時一致；若輸入用小寫排序、運算時用大小寫不敏感比較，結果可能不符合前置條件。
  - std::set_union 的輸入是 sorted range；這裡的 set operation 不是只能用 std::set，而是依排序結果做線性合併。
  - std::set_union 對重複元素有 multiset 語意，例如交集會取兩邊出現次數的較小值，聯集會取較大值。
  - std::set_union 的輸出 iterator 代表寫入結果的結尾；若使用預先配置容器，結束後通常要 erase 多餘尾端。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::set_union
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. set_union 對重複元素怎麼處理?和 std::merge 差在哪?
//     答：multiset 語意 — 某元素在 A 出現 m 次、B 出現 n 次,輸出 max(m, n) 次
//         (先從 A 複製 m 份,再從 B 補 max(n - m, 0) 份)。std::merge 則是兩邊
//         全保留,共 m + n 份。所以 set_union 不等於「去重」。
//     追問：那要真正去重怎麼辦?(答：先對兩邊各自 sort + unique,或 union 後再 unique)
//
// 🔥 Q2. 這一族 set_xxx 演算法要求輸入必須是 std::set 嗎?前置條件是什麼?
//     答：不需要。它們作用在「已依同一個 comp 排序的 range」— 排序過的 vector、
//         array、deque 都可以。前置條件是兩個輸入都已排序且用同一個準則;複雜度
//         O(N1 + N2) 的線性歸併,遠優於對每個元素做一次 find。輸出寫到 output
//         iterator,大小未知時搭配 std::back_inserter。
//     追問：std::set 能直接當輸入嗎?(答：可以,它本來就有序;unordered_set 不行)
//
// ⚠️ 陷阱. 輸入沒排序會發生什麼事?
//     答：這是違反前置條件的 UB。它「不會報錯」,會安靜地吐出一個看起來像結果、
//         實際上錯誤的序列 — 這種 bug 最難抓。
//     為什麼會錯：多數人以為演算法會自己處理、或至少會 assert;實際上它只做單趟
//         雙指標掃描,完全信任呼叫端已經排好序。
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
    std::vector<int> a{1, 2, 2, 3};
    std::vector<int> b{2, 4, 5};

    // --- 範例 1: 聯集 (注意 a 的兩個 2 + b 的一個 2 → max(2,1)=2 個) ---
    std::vector<int> u;
    std::set_union(a.begin(), a.end(),
                   b.begin(), b.end(),
                   std::back_inserter(u));
    std::cout << "union: ";
    for (int x : u) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 完全不相交 ---
    std::vector<int> p{1, 2, 3};
    std::vector<int> q{4, 5, 6};
    std::vector<int> uo;
    std::set_union(p.begin(), p.end(), q.begin(), q.end(),
                   std::back_inserter(uo));
    std::cout << "disjoint union: ";
    for (int x : uo) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 一個是另一個的子集 ---
    std::vector<int> sub{2, 3};
    std::vector<int> sup{1, 2, 3, 4};
    std::vector<int> us;
    std::set_union(sub.begin(), sub.end(), sup.begin(), sup.end(),
                   std::back_inserter(us));
    std::cout << "subset union: ";
    for (int x : us) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_2215_union_concept();
    void practical_merge_config_keys();
    void leetcode_2overlap_union_three_arrays();
    void practical_merge_subscriber_lists();
    leetcode_2215_union_concept();
    practical_merge_config_keys();
    leetcode_2overlap_union_three_arrays();
    practical_merge_subscriber_lists();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 2215 概念延伸:兩陣列的「去重後聯集」
// ----------------------------------------------------------------
// 題目:LC 2215「Find the Difference of Two Arrays」原題要的是兩邊各自獨有的元素。
//      這裡示範概念延伸 — 計算兩陣列「去重後的聯集」(unique merged set)。
//
// 為什麼用 std::set_union:
//   先 sort + unique 兩邊 → 兩邊每個元素唯一 → set_union 取 max(1,1) = 1 次,
//   結果中每個元素恰好出現一次。
//
// 複雜度:時間 O(n log n + m log m);空間 O(n + m)。
void leetcode_2215_union_concept() {
    std::vector<int> nums1{1, 2, 3, 3};
    std::vector<int> nums2{1, 1, 2, 2};
    std::sort(nums1.begin(), nums1.end());
    nums1.erase(std::unique(nums1.begin(), nums1.end()), nums1.end());
    std::sort(nums2.begin(), nums2.end());
    nums2.erase(std::unique(nums2.begin(), nums2.end()), nums2.end());
    std::vector<int> u;
    std::set_union(nums1.begin(), nums1.end(),
                   nums2.begin(), nums2.end(),
                   std::back_inserter(u));
    std::cout << "LC2215 union: ";
    for (int x : u) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:合併兩份「設定鍵清單」並去重
// ----------------------------------------------------------------
// 場景:系統有「預設鍵」與「使用者覆寫鍵」兩份名單,要彙整成一份
//      「所有可能會用到的鍵」(用於 schema 驗證 / 渲染 UI)。
//
// 為什麼用 std::set_union:
//   兩份各自唯一、且都已排序 → set_union 一次取得「所有鍵的聯集」,O(n+m)。
void practical_merge_config_keys() {
    std::vector<std::string> defaults{"host", "port", "timeout"};
    std::vector<std::string> overrides{"host", "retries", "timeout", "tls"};
    std::sort(defaults.begin(), defaults.end());
    std::sort(overrides.begin(), overrides.end());
    std::vector<std::string> all_keys;
    std::set_union(defaults.begin(), defaults.end(),
                   overrides.begin(), overrides.end(),
                   std::back_inserter(all_keys));
    std::cout << "all config keys:";
    for (const auto& k : all_keys) std::cout << ' ' << k;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 概念題:三個有序陣列的「去重聯集」
// ----------------------------------------------------------------
// 題目:給三個有序陣列,合併為唯一元素的聯集 — 兩次 set_union 即可。
//
// 為什麼用 std::set_union:
//   多源合併「聯集」常用法,兩兩 union 即得結果。
//
// 複雜度:時間 O(a + b + c);空間 O(a + b + c)。
void leetcode_2overlap_union_three_arrays() {
    std::vector<int> a{1, 2, 5}, b{2, 3, 5, 7}, c{1, 7, 9};
    std::vector<int> ab, abc;
    std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                   std::back_inserter(ab));
    std::set_union(ab.begin(), ab.end(), c.begin(), c.end(),
                   std::back_inserter(abc));
    std::cout << "union3:";
    for (int x : abc) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:合併多個訂閱者清單 (Newsletter)
// ----------------------------------------------------------------
// 場景:從各管道 (Email、SMS、App push) 各自收到訂閱者清單,
//      要彙整成一份「不重複的全體聯絡名單」 — set_union 一次完成。
void practical_merge_subscriber_lists() {
    std::vector<std::string> email{"a@x.com", "b@x.com", "c@x.com"};
    std::vector<std::string> sms{"b@x.com", "d@x.com"};
    std::sort(email.begin(), email.end());
    std::sort(sms.begin(), sms.end());
    std::vector<std::string> all;
    std::set_union(email.begin(), email.end(), sms.begin(), sms.end(),
                   std::back_inserter(all));
    std::cout << "subscribers:";
    for (auto& s : all) std::cout << ' ' << s;
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra set_union.cpp -o set_union

// === 預期輸出 ===
// union: 1 2 2 3 4 5
// disjoint union: 1 2 3 4 5 6
// subset union: 1 2 3 4
// LC2215 union: 1 2 3
// all config keys: host port retries timeout tls
// union3: 1 2 3 5 7 9
// subscribers: a@x.com b@x.com c@x.com d@x.com
