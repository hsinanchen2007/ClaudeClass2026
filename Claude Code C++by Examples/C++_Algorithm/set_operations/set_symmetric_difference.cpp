// ============================================================
// std::set_symmetric_difference
// 分類 (Category): Set operations on sorted ranges (有序集合運算)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/set_symmetric_difference
//   * https://cplusplus.com/reference/algorithm/set_symmetric_difference/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::set_symmetric_difference 解的問題:
//
//   「兩個『已排序』範圍的『對稱差』(A △ B) — 只在其中一邊出現的元素。」
//
// 數學定義:
//
//   A △ B = (A \ B) ∪ (B \ A) = (A ∪ B) \ (A ∩ B)
//
// 也就是「在 A 或 B,但不同時在兩邊」的元素。
//
// 重複元素規則:若 A 有 m 份等價元素、B 有 n 份,則輸出 |m - n| 份 —
// 從多的那一邊複製。若 m == n,該元素完全消除。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼有用?                                           │
// └────────────────────────────────────────────────────────────┘
//
// 對「找差異」這個需求,對稱差有兩個優點:
//
//   1. 順序顛倒結果相同 (對稱):set_symmetric_difference(A, B) ==
//                                set_symmetric_difference(B, A)。
//   2. 一次取「兩邊各自獨有的聯集」 — 不必呼叫兩次 set_difference。
//
// 缺點:它把「來自 A」和「來自 B」混在一起 — 無法區分哪些是 A 獨有、
// 哪些是 B 獨有。如果你要區分,還是要 set_difference 兩次。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 「兩份名單比對 — 列出所有不一致的人員」
//   * 「兩份檔案 diff — 列出所有變動的行」(初步比對)
//   * 「報名 vs 出席 — 列出所有異常」
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt1, class InputIt2, class OutputIt>
//   OutputIt set_symmetric_difference(InputIt1 first1, InputIt1 last1,
//                                     InputIt2 first2, InputIt2 last2,
//                                     OutputIt d_first);
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
//   1. 對稱 — 順序顛倒結果相同。
//   2. 重複元素規則:留下 |m - n| 份。
//   3. 想分別知道「A 獨有」「B 獨有」 → 用 set_difference 雙向。
//
// ============================================================

/*
補充筆記：std::set_symmetric_difference
  - std::set_symmetric_difference 是排序範圍上的集合運算；輸入必須已依同一比較器排序，否則交集、聯集、差集結果會錯。
  - 這些演算法處理的是 sorted range，不是 std::set 專用；vector 排序後也能使用。
  - set_union/set_intersection/set_difference 對重複元素有明確規則，結果數量不是單純去重後集合概念。
  - 輸出範圍要有足夠空間，或使用 back_inserter 讓容器自動追加。
  - merge/inplace_merge 合併的是兩段已排序資料；它們不會替未排序資料做完整排序。
  - 比較器必須與排序時一致；若輸入用小寫排序、運算時用大小寫不敏感比較，結果可能不符合前置條件。
  - std::set_symmetric_difference 的輸入是 sorted range；這裡的 set operation 不是只能用 std::set，而是依排序結果做線性合併。
  - std::set_symmetric_difference 對重複元素有 multiset 語意，例如交集會取兩邊出現次數的較小值，聯集會取較大值。
  - std::set_symmetric_difference 的輸出 iterator 代表寫入結果的結尾；若使用預先配置容器，結束後通常要 erase 多餘尾端。
*/
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> a{1, 2, 3, 4, 5};
    std::vector<int> b{4, 5, 6, 7};

    // --- 範例 1: 對稱差 (兩邊各自獨有的聯集) ---
    std::vector<int> sd;
    std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(),
                                  std::back_inserter(sd));
    std::cout << "symmetric difference: ";
    for (int x : sd) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 重複元素 |m-n| 份 ---
    std::vector<int> p{1, 2, 2, 2, 3};
    std::vector<int> q{2, 2, 4};
    std::vector<int> r;
    std::set_symmetric_difference(p.begin(), p.end(), q.begin(), q.end(),
                                  std::back_inserter(r));
    std::cout << "with duplicates: ";
    for (int x : r) std::cout << x << ' ';
    std::cout << "(|3-2|=1 個 2)\n";

    // --- 範例 3: 完全相同 → 空 ---
    std::vector<int> x{1, 2, 3};
    std::vector<int> y{1, 2, 3};
    std::vector<int> empty;
    std::set_symmetric_difference(x.begin(), x.end(), y.begin(), y.end(),
                                  std::back_inserter(empty));
    std::cout << "identical -> size = " << empty.size() << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_2215_symmetric_difference();
    void practical_attendees_diff();
    void leetcode_concept_xor_sets();
    void practical_file_sync_changes();
    leetcode_2215_symmetric_difference();
    practical_attendees_diff();
    leetcode_concept_xor_sets();
    practical_file_sync_changes();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 2215 對稱差版:找出兩陣列的差異
// ----------------------------------------------------------------
// 題目:同 LC 2215 — 給兩個陣列,要回傳 [A 中獨有, B 中獨有]。
//
// 為什麼用 std::set_symmetric_difference:
//   若把問題視為「兩邊各自獨有合在一起」(A△B),那就是經典的對稱差。
//   sort + unique 兩個輸入後,一次 set_symmetric_difference 即可同時
//   拿到「兩邊獨有」的聯集。 (若還要區分哪邊獨有,再用 set_difference 兩次。)
//
// 複雜度:時間 O(n log n + m log m);空間 O(n + m)。
void leetcode_2215_symmetric_difference() {
    std::vector<int> nums1{1, 2, 3, 3};
    std::vector<int> nums2{1, 1, 2, 2, 4};
    std::sort(nums1.begin(), nums1.end());
    nums1.erase(std::unique(nums1.begin(), nums1.end()), nums1.end());
    std::sort(nums2.begin(), nums2.end());
    nums2.erase(std::unique(nums2.begin(), nums2.end()), nums2.end());
    std::vector<int> sym;
    std::set_symmetric_difference(nums1.begin(), nums1.end(),
                                  nums2.begin(), nums2.end(),
                                  std::back_inserter(sym));
    std::cout << "LC2215 sym:";
    for (int x : sym) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:報名 vs 出席名單比對
// ----------------------------------------------------------------
// 場景:活動結束,要列出「報了沒到」與「來了沒報」的所有異常人員。
//
// 為什麼用 std::set_symmetric_difference:
//   一次取兩類異常的聯集,適合「我只想看誰有狀況」的場景。
//   若要區分哪邊獨有,再做 set_difference 兩次。
void practical_attendees_diff() {
    std::vector<std::string> registered{"alice", "bob", "carol", "dave"};
    std::vector<std::string> attended{"alice", "carol", "erin"};
    std::sort(registered.begin(), registered.end());
    std::sort(attended.begin(), attended.end());
    std::vector<std::string> mismatch;
    std::set_symmetric_difference(registered.begin(), registered.end(),
                                  attended.begin(), attended.end(),
                                  std::back_inserter(mismatch));
    std::cout << "attendees mismatch:";
    for (const auto& n : mismatch) std::cout << ' ' << n;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 概念:兩集合的 XOR (對稱差) 元素
// ----------------------------------------------------------------
// 題目:給整數陣列 A、B,輸出「只在 A 或只在 B」的元素 (不在交集)。
//
// 為什麼用 std::set_symmetric_difference:
//   題意 100% 對應 — 排序後一行解決。
//
// 複雜度:時間 O(n log n + m log m);空間 O(n + m)。
void leetcode_concept_xor_sets() {
    std::vector<int> A{1, 2, 3, 5, 8};
    std::vector<int> B{2, 4, 5, 7, 8};
    std::sort(A.begin(), A.end());
    std::sort(B.begin(), B.end());
    std::vector<int> xor_set;
    std::set_symmetric_difference(A.begin(), A.end(),
                                  B.begin(), B.end(),
                                  std::back_inserter(xor_set));
    std::cout << "xor:";
    for (int x : xor_set) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:檔案同步 — 列出 source 與 dest 不一致的檔案
// ----------------------------------------------------------------
// 場景:rsync / 雲端同步要找「兩端不一致」的檔案 (任一端有、另一端沒有)。
//      symmetric_difference 一行給出聯集 — 之後再決定誰要拉誰要推。
void practical_file_sync_changes() {
    std::vector<std::string> src{"a.txt", "b.txt", "c.txt", "e.txt"};
    std::vector<std::string> dst{"a.txt", "c.txt", "d.txt", "e.txt"};
    std::sort(src.begin(), src.end());
    std::sort(dst.begin(), dst.end());
    std::vector<std::string> diff;
    std::set_symmetric_difference(src.begin(), src.end(),
                                  dst.begin(), dst.end(),
                                  std::back_inserter(diff));
    std::cout << "out-of-sync files:";
    for (auto& f : diff) std::cout << ' ' << f;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// symmetric difference: 1 2 3 6 7
// with duplicates: 1 2 3 4 (|3-2|=1 個 2)
// identical -> size = 0
// LC2215 sym: 3 4
// attendees mismatch: bob dave erin
// xor: 1 3 4 7
// out-of-sync files: b.txt d.txt
