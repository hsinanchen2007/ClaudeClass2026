// ============================================================
// std::set_difference
// 分類 (Category): Set operations on sorted ranges (有序集合運算)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/set_difference
//   * https://cplusplus.com/reference/algorithm/set_difference/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::set_difference 解的問題:
//
//   「兩個『已排序』範圍的『差集』(A \ B) — 在 A 但不在 B 的元素。」
//
// 重複元素規則:若 A 有 m 份等價元素、B 有 n 份,則輸出 max(m - n, 0) 份。
//
// 範例:
//   a = {1, 2, 2, 2, 3},  b = {2, 2}
//   diff (A\B) → {1, 2, 3}    (max(3-2, 0) = 1 個 2 留下)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼順序重要?                                       │
// └────────────────────────────────────────────────────────────┘
//
// 差集是「不對稱」的 — set_difference(A, B) ≠ set_difference(B, A):
//
//   * A \ B = 「在 A 但不在 B」
//   * B \ A = 「在 B 但不在 A」
//
// 兩者意義不同,寫程式時要想清楚「我要哪一邊獨有的元素」。
//
// 想要「兩邊各自獨有的聯集」 → 用 set_symmetric_difference (對稱差)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * RBAC 權限「diff」:old \ new = 要撤銷的;new \ old = 要授予的
//   * 「下個版本要刪除哪些 feature」(舊清單 - 新清單)
//   * 「哪些檔案是新增的」(新清單 - 舊清單)
//   * 通用的「找差異」工具
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt1, class InputIt2, class OutputIt>
//   OutputIt set_difference(InputIt1 first1, InputIt1 last1,
//                           InputIt2 first2, InputIt2 last2,
//                           OutputIt d_first);
//
//   + 帶 Compare 的版本
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 2 × (N1 + N2) - 1 次比較 — O(N1 + N2)
//   空間: O(N1) (輸出最多)
//   需求: 兩範圍依 comp 已排序;輸出不可與輸入重疊。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 順序重要 — set_difference(A, B) ≠ set_difference(B, A)。
//   2. 範圍必須已排序。
//   3. 想要「兩邊各自獨有的聯集」用 set_symmetric_difference。
//
// ============================================================

/*
補充筆記：std::set_difference
  - std::set_difference 是排序範圍上的集合運算；輸入必須已依同一比較器排序，否則交集、聯集、差集結果會錯。
  - 這些演算法處理的是 sorted range，不是 std::set 專用；vector 排序後也能使用。
  - set_union/set_intersection/set_difference 對重複元素有明確規則，結果數量不是單純去重後集合概念。
  - 輸出範圍要有足夠空間，或使用 back_inserter 讓容器自動追加。
  - merge/inplace_merge 合併的是兩段已排序資料；它們不會替未排序資料做完整排序。
  - 比較器必須與排序時一致；若輸入用小寫排序、運算時用大小寫不敏感比較，結果可能不符合前置條件。
  - std::set_difference 的輸入是 sorted range；這裡的 set operation 不是只能用 std::set，而是依排序結果做線性合併。
  - std::set_difference 對重複元素有 multiset 語意，例如交集會取兩邊出現次數的較小值，聯集會取較大值。
  - std::set_difference 的輸出 iterator 代表寫入結果的結尾；若使用預先配置容器，結束後通常要 erase 多餘尾端。
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
    std::vector<int> b{2, 4, 6};

    // --- 範例 1: A \ B ---
    std::vector<int> diff;
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(),
                        std::back_inserter(diff));
    std::cout << "A \\ B: ";
    for (int x : diff) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: B \ A (順序顛倒,結果不同!) ---
    std::vector<int> diff2;
    std::set_difference(b.begin(), b.end(), a.begin(), a.end(),
                        std::back_inserter(diff2));
    std::cout << "B \\ A: ";
    for (int x : diff2) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 重複元素 max(m - n, 0) ---
    std::vector<int> p{1, 2, 2, 2, 3};
    std::vector<int> q{2, 2};
    std::vector<int> r;
    std::set_difference(p.begin(), p.end(), q.begin(), q.end(),
                        std::back_inserter(r));
    std::cout << "{1,2,2,2,3} \\ {2,2} = ";
    for (int x : r) std::cout << x << ' ';
    std::cout << "(max(3-2,0)=1 個 2 留下)\n";

    // === LeetCode / 實務範例 ===
    void leetcode_2215_find_difference_two_arrays();
    void practical_permission_diff();
    void leetcode_2overlap_unique_to_a();
    void practical_friend_unfollow_list();
    leetcode_2215_find_difference_two_arrays();
    practical_permission_diff();
    leetcode_2overlap_unique_to_a();
    practical_friend_unfollow_list();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 2215: 找出兩陣列的差異 (Find the Difference of Two Arrays)
// ----------------------------------------------------------------
// 題目:給兩個整數陣列 nums1, nums2,回傳 size 2 的 list:
//      answer[0] = nums1 中存在但 nums2 中不存在的「不同元素」(去重)
//      answer[1] = nums2 中存在但 nums1 中不存在的「不同元素」(去重)
//
// 為什麼用 std::set_difference:
//   * 題目要的就是兩個方向的差集 — set_difference 做兩次即可。
//   * 因要求結果元素唯一,先 sort + unique 兩個輸入。
//
// 複雜度:時間 O(n log n + m log m);空間 O(n + m)。
void leetcode_2215_find_difference_two_arrays() {
    std::vector<int> nums1{1, 2, 3, 3};
    std::vector<int> nums2{1, 1, 2, 2};
    std::sort(nums1.begin(), nums1.end());
    nums1.erase(std::unique(nums1.begin(), nums1.end()), nums1.end());
    std::sort(nums2.begin(), nums2.end());
    nums2.erase(std::unique(nums2.begin(), nums2.end()), nums2.end());

    std::vector<int> only_in_1, only_in_2;
    std::set_difference(nums1.begin(), nums1.end(),
                        nums2.begin(), nums2.end(),
                        std::back_inserter(only_in_1));
    std::set_difference(nums2.begin(), nums2.end(),
                        nums1.begin(), nums1.end(),
                        std::back_inserter(only_in_2));
    std::cout << "LC2215 only_in_1:";
    for (int x : only_in_1) std::cout << ' ' << x;
    std::cout << " | only_in_2:";
    for (int x : only_in_2) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:RBAC 權限「diff」(撤銷 / 授予)
// ----------------------------------------------------------------
// 場景:一份權限設定要從 old 更新為 new。要找出:
//   removed = old \ new  (要從使用者身上撤銷的權限)
//   added   = new \ old  (要授予的新權限)
//
// 為什麼用 std::set_difference:
//   題意精準對應「兩個方向的差集」 — set_difference 雙向呼叫即可,語意清晰。
void practical_permission_diff() {
    std::vector<std::string> old_perms{"admin", "delete", "read", "write"};
    std::vector<std::string> new_perms{"audit", "read", "write"};
    std::sort(old_perms.begin(), old_perms.end());
    std::sort(new_perms.begin(), new_perms.end());

    std::vector<std::string> removed, added;
    std::set_difference(old_perms.begin(), old_perms.end(),
                        new_perms.begin(), new_perms.end(),
                        std::back_inserter(removed));
    std::set_difference(new_perms.begin(), new_perms.end(),
                        old_perms.begin(), old_perms.end(),
                        std::back_inserter(added));
    std::cout << "perm removed:";
    for (const auto& p : removed) std::cout << ' ' << p;
    std::cout << " | added:";
    for (const auto& p : added) std::cout << ' ' << p;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 概念題:找出「只在 A 出現過」的元素 (兩集合差集)
// ----------------------------------------------------------------
// 題目:給整數集合 A、B,輸出只屬於 A 的元素 (含重複)。
//
// 為什麼用 std::set_difference:
//   題意 100% 對應 — 對排序好的輸入直接呼叫即可。
//
// 複雜度:時間 O(n + m);空間 O(n)。
void leetcode_2overlap_unique_to_a() {
    std::vector<int> A{1, 2, 2, 5, 8};
    std::vector<int> B{2, 5, 9};
    std::sort(A.begin(), A.end());
    std::sort(B.begin(), B.end());
    std::vector<int> only_a;
    std::set_difference(A.begin(), A.end(), B.begin(), B.end(),
                        std::back_inserter(only_a));
    std::cout << "only in A:";
    for (int x : only_a) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:社交平台 — 取消追蹤名單 (only in old, not in new)
// ----------------------------------------------------------------
// 場景:使用者上次追蹤清單與本次相比,要算出「本次取消追蹤」的人數。
//      diff(old, new) 就是「old 中有、new 中沒有」 = 取消追蹤者。
void practical_friend_unfollow_list() {
    std::vector<std::string> last{"a", "b", "c", "d"};
    std::vector<std::string> now{"a", "c", "e"};
    std::sort(last.begin(), last.end());
    std::sort(now.begin(), now.end());
    std::vector<std::string> unfollowed;
    std::set_difference(last.begin(), last.end(), now.begin(), now.end(),
                        std::back_inserter(unfollowed));
    std::cout << "unfollowed:";
    for (auto& u : unfollowed) std::cout << ' ' << u;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// A \ B: 1 3 5
// B \ A: 6
// {1,2,2,2,3} \ {2,2} = 1 2 3 (max(3-2,0)=1 個 2 留下)
// LC2215 only_in_1: 3 | only_in_2:
// perm removed: admin delete | added: audit
// only in A: 1 2 8
// unfollowed: b d
