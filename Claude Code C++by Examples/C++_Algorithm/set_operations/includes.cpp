// ============================================================
// std::includes
// 分類 (Category): Set operations on sorted ranges (有序集合運算)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/includes
//   * https://cplusplus.com/reference/algorithm/includes/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::includes 解的問題:
//
//   「在已排序的 S1 中,是否包含整個已排序的 S2?(即 S1 ⊇ S2)」
//
// 這是「子集判定」的線性時間 STL 工具。
//
// 「包含」的定義包含「重複次數」:
//   * S1 中等價元素至少要 ≥ S2 中等價元素的個數。
//
// 範例:
//   S1 = {1, 2, 2, 3},  S2 = {2, 2, 2}  →  false  (S1 只有 2 個 2,不夠)
//   S1 = {1, 2, 2, 2},  S2 = {2, 2}     →  true   (S1 有 3 個 2,夠)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼比 hash 解法好?                                 │
// └────────────────────────────────────────────────────────────┘
//
// 對「資料天然就排序」(時序、字典序) 或「短陣列」的場景:
//
//   * std::includes 是線性 O(N + M),不需建 hash table。
//   * Cache friendly — 連續記憶體存取。
//   * 一行表達意圖,可讀性高。
//
// 對「資料無法排序」或「重複多次查詢同一個 S1」的場景,
// hash set 解法 (建表 O(N) + 查詢 O(M) per query) 可能更划算。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、空集合的特殊規則                                       │
// └────────────────────────────────────────────────────────────┘
//
//   includes(任何, 空) → true  (空集合是任何集合的子集 — 數學定義)
//
// 這是 vacuous truth — 跟 all_of(空) 回 true 同樣的邏輯。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * RBAC 權限「子集檢查」:owned ⊇ required → 放行
//   * 「能力 (capability) 檢查」:user_caps ⊇ needed_caps
//   * 「庫存判斷」:stock ⊇ order_items (multiset 語意,要看數量)
//   * 「multiset 子集」 — 字符出現次數比對 (例如異位詞)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt1, class InputIt2>
//   bool includes(InputIt1 first1, InputIt1 last1,
//                 InputIt2 first2, InputIt2 last2);
//
//   template <class InputIt1, class InputIt2, class Compare>
//   bool includes(InputIt1 first1, InputIt1 last1,
//                 InputIt2 first2, InputIt2 last2,
//                 Compare comp);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 2 × (N1 + N2) - 1 次比較 — O(N1 + N2)
//   空間: O(1)
//   需求: 兩範圍依「同一個」comp 已排序;否則 UB。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 兩範圍必須已依「同一個」comp 排序,否則 UB。
//   2. 「重複次數要對得上」 — multiset 語意,別誤會成 set 子集。
//   3. 空 S2 → 永遠 true。
//
// ============================================================

/*
補充筆記：std::includes
  - std::includes 是排序範圍上的集合運算；輸入必須已依同一比較器排序，否則交集、聯集、差集結果會錯。
  - 這些演算法處理的是 sorted range，不是 std::set 專用；vector 排序後也能使用。
  - set_union/set_intersection/set_difference 對重複元素有明確規則，結果數量不是單純去重後集合概念。
  - 輸出範圍要有足夠空間，或使用 back_inserter 讓容器自動追加。
  - merge/inplace_merge 合併的是兩段已排序資料；它們不會替未排序資料做完整排序。
  - 比較器必須與排序時一致；若輸入用小寫排序、運算時用大小寫不敏感比較，結果可能不符合前置條件。
  - std::includes 的輸入是 sorted range；這裡的 set operation 不是只能用 std::set，而是依排序結果做線性合併。
  - std::includes 對重複元素有 multiset 語意，例如交集會取兩邊出現次數的較小值，聯集會取較大值。
  - std::includes 的輸出 iterator 代表寫入結果的結尾；若使用預先配置容器，結束後通常要 erase 多餘尾端。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::includes
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::includes 判斷什麼?前置條件與複雜度?
//     答：子集判定 — 問「第一個有序範圍是否包含第二個」(S1 ⊇ S2),回傳 bool。
//         前置條件與整族 set_xxx 相同:兩個輸入都必須已依「同一個」comp 排序,
//         但不必是 std::set — 排序過的 vector 就行。單趟雙指標掃描,O(N1 + N2),
//         遠優於對 S2 每個元素去 S1 做一次 find。
//     追問：參數順序可以交換嗎?(答：不行,它不是對稱關係;includes(a, b) 問的是
//           b ⊆ a,想問反向必須自己交換兩對迭代器)
//
// 🔥 Q2. 「包含」是只看元素種類,還是也看出現次數?
//     答：也看次數,是 multiset 語意 — S1 中等價元素的份數必須 ≥ S2 中的份數。
//         所以 S1 = {1,2,2,3} 並不包含 S2 = {2,2,2}(只有 2 個 2,不夠)。
//         本檔的 LC 383 贖金信範例正是靠這個語意:magazine 的字元「夠不夠」拼出
//         ransomNote,排序後一次 includes 就解決。
//     追問：想要忽略次數的純集合子集怎麼做?(答：兩邊先各自 sort + unique 再 includes)
//
// Q3. 第二個範圍是空的時候回傳什麼?
//     答：true。空集合是任何集合的子集,這是 vacuous truth — 和 std::all_of 對空
//         範圍回傳 true 是同一種邏輯。邊界條件題很愛考這個。
//
// ⚠️ 陷阱. 兩邊都排序了,為什麼結果還是錯?
//     答：很可能是「排序準則」與「傳給 includes 的 comp」不一致 — 例如資料用
//         預設 less 排序,卻傳了大小寫不敏感的 comp。這一樣是違反前置條件的 UB,
//         不會報錯,只會給出安靜錯誤的 true/false。
//     為什麼會錯：只記得「要排序」,漏掉前置條件其實是「依同一個準則排序」。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::cout << std::boolalpha;

    // --- 範例 1: 子集 ---
    std::vector<int> big{1, 2, 3, 4, 5, 6};
    std::vector<int> sub{2, 4, 6};
    std::cout << "{1..6} contains {2,4,6}? "
              << std::includes(big.begin(), big.end(), sub.begin(), sub.end())
              << '\n';

    // --- 範例 2: 不是子集 (有元素不在裡面) ---
    std::vector<int> nope{2, 7};
    std::cout << "{1..6} contains {2,7}? "
              << std::includes(big.begin(), big.end(), nope.begin(), nope.end())
              << '\n';

    // --- 範例 3: 重複元素 — S2 的重複比 S1 多 → false ---
    std::vector<int> s1{1, 2, 2, 3};
    std::vector<int> s2{2, 2, 2};
    std::cout << "{1,2,2,3} contains {2,2,2}? "
              << std::includes(s1.begin(), s1.end(), s2.begin(), s2.end())
              << '\n';

    // --- 範例 4: 空 S2 → 永遠 true (vacuous truth) ---
    std::vector<int> empty;
    std::cout << "any contains empty? "
              << std::includes(big.begin(), big.end(),
                               empty.begin(), empty.end()) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_subset_check();
    void leetcode_383_ransom_note_concept();
    void practical_permission_subset();
    void leetcode_1002_common_chars_concept();
    void practical_feature_dependency_check();
    leetcode_subset_check();
    leetcode_383_ransom_note_concept();
    practical_permission_subset();
    leetcode_1002_common_chars_concept();
    practical_feature_dependency_check();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 子題:陣列子集判定
// ----------------------------------------------------------------
// 題目:給兩個整數陣列 A、B,判斷 B 是否為 A 的「multiset 子集」
//      (即 B 的每個元素都能在 A 中找到對應出現次數)。
//
// 為什麼用 std::includes:
//   兩邊各自 sort 後,直接 includes 一行解決,O(N + M)。
//   不必去建 hash 計次表。
//
// 複雜度:時間 O(n log n + m log m);空間 O(1)。
void leetcode_subset_check() {
    std::vector<int> A{4, 1, 3, 2, 5, 3};
    std::vector<int> B{3, 1, 5};
    std::sort(A.begin(), A.end());
    std::sort(B.begin(), B.end());
    bool ok = std::includes(A.begin(), A.end(), B.begin(), B.end());
    std::cout << "subset check: " << ok << '\n';
}

// ----------------------------------------------------------------
// LeetCode 383: 贖金信 (Ransom Note) 概念
// ----------------------------------------------------------------
// 題目:給兩字串 ransomNote 與 magazine,判斷 magazine 中的字元
//      是否「足以」拼成 ransomNote (每個字元只能用一次)。
//
// 為什麼用 std::includes:
//   把兩字串各自排序 → magazine 是「multiset 包含」 ransomNote
//   就等於「夠用」。一行 includes 完成。
//
// 複雜度:時間 O(n log n + m log m);空間 O(1)。
void leetcode_383_ransom_note_concept() {
    std::string ransomNote = "aab";
    std::string magazine   = "baaab";
    std::sort(ransomNote.begin(), ransomNote.end());
    std::sort(magazine.begin(), magazine.end());
    bool ok = std::includes(magazine.begin(), magazine.end(),
                            ransomNote.begin(), ransomNote.end());
    std::cout << "LC383: " << ok << '\n';
}

// ----------------------------------------------------------------
// 實務範例:RBAC 權限子集檢查
// ----------------------------------------------------------------
// 場景:API 端點需要某組權限 required,使用者持有 owned。
//      若 owned ⊇ required → 放行。
//
// 為什麼用 std::includes:
//   * 線性、簡潔、語意明確。
//   * 對短權限清單比 hash set 還快 (常數小)。
void practical_permission_subset() {
    std::vector<std::string> owned{"read", "write", "delete", "admin"};
    std::vector<std::string> required{"read", "write"};
    std::sort(owned.begin(), owned.end());
    std::sort(required.begin(), required.end());
    bool granted = std::includes(owned.begin(), owned.end(),
                                 required.begin(), required.end());
    std::cout << "permission granted? " << granted << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1002 概念:包含關係驗證 (用 includes 判定字串包含關係)
// ----------------------------------------------------------------
// 題目:給多個字串,判斷第一個是否「字元 multiset」包含其他全部 (簡化版)。
//
// 為什麼用 std::includes:
//   對每個其他字串,排序後 includes 檢查是否「夠用」 — 一行表達語意。
//
// 複雜度:時間 O(k × n log n);空間 O(1)。
void leetcode_1002_common_chars_concept() {
    std::string base = "bellabell";   // 排序後 abbbeell l
    std::vector<std::string> others{"bell", "lab"};
    std::sort(base.begin(), base.end());
    bool ok = true;
    for (auto s : others) {
        std::sort(s.begin(), s.end());
        if (!std::includes(base.begin(), base.end(), s.begin(), s.end())) {
            ok = false; break;
        }
    }
    std::cout << "LC1002 base covers all: " << std::boolalpha << ok << '\n';
}

// ----------------------------------------------------------------
// 實務範例:安裝套件 — 檢查依賴是否「已安裝」(子集合判定)
// ----------------------------------------------------------------
// 場景:安裝某套件前,要驗證它列出的所有 dependencies 是否都已安裝。
//      把已安裝套件清單與依賴清單排序後,includes 一行判定。
void practical_feature_dependency_check() {
    std::vector<std::string> installed{"libA", "libB", "libC", "libD"};
    std::vector<std::string> deps{"libA", "libC"};
    std::sort(installed.begin(), installed.end());
    std::sort(deps.begin(), deps.end());
    bool ok = std::includes(installed.begin(), installed.end(),
                            deps.begin(), deps.end());
    std::cout << "all deps installed: " << std::boolalpha << ok << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra includes.cpp -o includes

// === 預期輸出 ===
// {1..6} contains {2,4,6}? true
// {1..6} contains {2,7}? false
// {1,2,2,3} contains {2,2,2}? false
// any contains empty? true
// subset check: true
// LC383: true
// permission granted? true
// LC1002 base covers all: true
// all deps installed: true
