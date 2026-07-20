// ============================================================
// std::merge
// 分類 (Category): Set operations on sorted ranges (有序範圍合併)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/merge
//   * https://cplusplus.com/reference/algorithm/merge/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::merge 解的問題:
//
//   「兩段『已排序』的資料,合併成一段『仍然排序』的資料,寫到輸出端。」
//
// 它是「合併排序 (Merge Sort)」的核心步驟,也是處理「兩段已排好的時序資料」
// 最自然的工具。
//
// 重要特性:
//   * 「保留所有元素」 — 兩邊有重複都會出現在輸出 (與 set_union 不同)。
//   * 線性時間 — 不必比較所有對 (i, j),只是兩個指針向前推。
//   * 「穩定」 — 兩邊有等價元素時,先出現第一範圍的拷貝。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、merge vs set_union 的差別                              │
// └────────────────────────────────────────────────────────────┘
//
//   * merge:     重複元素「全部保留」 (m + n 份)
//   * set_union: 重複元素只保留 max(m, n) 份 (集合語意,去除多餘)
//
// 範例:a = {1, 2, 2, 3},  b = {2, 4}
//   merge      → {1, 2, 2, 2, 3, 4}    (a 的兩個 2 + b 的一個 2)
//   set_union  → {1, 2, 2, 3, 4}       (max(2, 1) = 2 個 2)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 合併兩台機器各自已排序的 log → 單一時間軸
//   * 合併排序的內部步驟
//   * LC 88 Merge Sorted Array、LC 21 Merge Two Sorted Lists 概念
//   * 兩段已排序資料合成 (例如歷史快照 + 新進資料)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt1, class InputIt2, class OutputIt>
//   OutputIt merge(InputIt1 first1, InputIt1 last1,
//                  InputIt2 first2, InputIt2 last2,
//                  OutputIt d_first);
//
//   + 帶 Compare 的版本
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 N1 + N2 - 1 次比較 — O(N1 + N2)
//   空間: O(1) (除了輸出)
//   需求:
//     * 兩個輸入範圍已依 comp 排序 (同方向,否則結果亂)。
//     * 輸出範圍不得與兩個輸入範圍重疊 — 重疊請改 std::inplace_merge。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 是「合併」不是「聯集」 — 重複元素全保留。
//   2. 兩邊的排序方向必須一致 (都升或都降),否則結果未排序。
//   3. 同一容器內的兩段子範圍合併用 std::inplace_merge。
//   4. 對更高層的 N 路合併 (priority_queue 為基礎),需要自己組合。
//
// ============================================================

/*
補充筆記：std::merge
  - std::merge 是排序範圍上的集合運算；輸入必須已依同一比較器排序，否則交集、聯集、差集結果會錯。
  - 這些演算法處理的是 sorted range，不是 std::set 專用；vector 排序後也能使用。
  - set_union/set_intersection/set_difference 對重複元素有明確規則，結果數量不是單純去重後集合概念。
  - 輸出範圍要有足夠空間，或使用 back_inserter 讓容器自動追加。
  - merge/inplace_merge 合併的是兩段已排序資料；它們不會替未排序資料做完整排序。
  - 比較器必須與排序時一致；若輸入用小寫排序、運算時用大小寫不敏感比較，結果可能不符合前置條件。
  - std::merge 的輸入是 sorted range；這裡的 set operation 不是只能用 std::set，而是依排序結果做線性合併。
  - std::merge 對重複元素有 multiset 語意，例如交集會取兩邊出現次數的較小值，聯集會取較大值。
  - std::merge 的輸出 iterator 代表寫入結果的結尾；若使用預先配置容器，結束後通常要 erase 多餘尾端。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::merge
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::merge 和 std::set_union 差在哪?
//     答：merge 是「合併」— 兩邊的元素全部保留,某元素在 A 有 m 份、B 有 n 份,
//         輸出就有 m + n 份。set_union 是集合語意,只留 max(m, n) 份。
//         例:A = {1,2,2,3}、B = {2,4} → merge 得 {1,2,2,2,3,4};
//         set_union 得 {1,2,2,3,4}。要「串接兩段時序資料」用 merge,要「聯集」用 union。
//     追問：兩者複雜度一樣嗎?(答：都是 O(N1 + N2) 的線性歸併)
//
// 🔥 Q2. 前置條件是什麼?merge 穩定嗎?
//     答：兩個輸入都必須已依「同一個」comp 排序、方向一致(不必是 std::set,排序過
//         的 vector 就行)。merge 是穩定的 — 兩邊有等價元素時,第一個範圍的拷貝
//         一定排在第二個範圍的前面,所以「哪邊優先」是可預測的。
//     追問：未排序輸入會怎樣?(答：違反前置條件的 UB,不會報錯,只會安靜給出錯的結果)
//
// Q3. 輸出範圍可以和輸入重疊嗎?
//     答：不行,那是 UB。std::merge 要求目的端與兩個輸入範圍都不重疊。若你的兩段
//         有序資料本來就在「同一個容器裡相鄰」,要用的是 std::inplace_merge —
//         它就是為了就地合併而設計的。
//     追問：constexpr 呢?(答：std::merge 自 C++20 起是 constexpr;
//           std::inplace_merge 因為可能配置暫存記憶體,C++20 並未一併加上)
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> a{1, 3, 5, 7};
    std::vector<int> b{2, 3, 4, 8};

    // --- 範例 1: 合併兩個排序好的容器 ---
    std::vector<int> out;
    std::merge(a.begin(), a.end(),
               b.begin(), b.end(),
               std::back_inserter(out));
    std::cout << "merge: ";
    for (int x : out) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 對降序資料,改用 greater ---
    std::vector<int> da{9, 7, 5};
    std::vector<int> db{8, 6, 1};
    std::vector<int> dout;
    std::merge(da.begin(), da.end(), db.begin(), db.end(),
               std::back_inserter(dout), std::greater<int>{});
    std::cout << "merge desc: ";
    for (int x : dout) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 重複元素全部保留 (對比 set_union) ---
    std::vector<int> p{1, 2, 2, 3};
    std::vector<int> q{2, 2, 4};
    std::vector<int> mout;
    std::merge(p.begin(), p.end(), q.begin(), q.end(),
               std::back_inserter(mout));
    std::cout << "merge with duplicates: ";
    for (int x : mout) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_88_merge_sorted_array_extra();
    void leetcode_21_merge_two_sorted_lists_concept();
    void practical_merge_sorted_logs();
    void leetcode_1768_merge_alternately();
    void practical_merge_two_event_streams();
    leetcode_88_merge_sorted_array_extra();
    leetcode_21_merge_two_sorted_lists_concept();
    practical_merge_sorted_logs();
    leetcode_1768_merge_alternately();
    practical_merge_two_event_streams();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 88: 合併兩個有序陣列 (用額外空間版)
// ----------------------------------------------------------------
// 題目:nums1 (大小 m+n,前 m 為有效) + nums2 (大小 n),合併進 nums1 並排序。
//
// 為什麼用 std::merge:
//   兩段已排序 → std::merge 一次完成,寫到 buffer 再 copy 回 nums1。
//
// 複雜度:時間 O(m + n);空間 O(m + n)。
void leetcode_88_merge_sorted_array_extra() {
    std::vector<int> nums1{1, 2, 3, 0, 0, 0};
    int m = 3;
    std::vector<int> nums2{2, 5, 6};
    int n = 3;
    std::vector<int> buf;
    buf.reserve(m + n);
    std::merge(nums1.begin(), nums1.begin() + m,
               nums2.begin(), nums2.begin() + n,
               std::back_inserter(buf));
    std::copy(buf.begin(), buf.end(), nums1.begin());
    std::cout << "LC88: ";
    for (int x : nums1) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 21: 合併兩個排序鏈結串列 (概念示範)
// ----------------------------------------------------------------
// 題目:給兩個排序好的鏈結串列,合併成一個新的排序串列。
//
// 為什麼用 std::merge (概念):
//   鏈結串列版需用雙指針接點,但「概念」與 std::merge 完全一致 —
//   兩個已排序輸入合成一個。這裡用 vector 模擬其等價邏輯。
void leetcode_21_merge_two_sorted_lists_concept() {
    std::vector<int> l1{1, 2, 4};
    std::vector<int> l2{1, 3, 4};
    std::vector<int> out;
    std::merge(l1.begin(), l1.end(), l2.begin(), l2.end(),
               std::back_inserter(out));
    std::cout << "LC21: ";
    for (int x : out) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:合併兩台機器的時序 log
// ----------------------------------------------------------------
// 場景:兩台 server 各自輸出已按時戳排序的 log,要合併成單一時間軸供分析。
//
// 為什麼用 std::merge:
//   兩份輸入皆已排序 → 線性 O(N+M) 一次合併,不需重排。
void practical_merge_sorted_logs() {
    std::vector<long> serverA{1001, 1003, 1007, 1010};
    std::vector<long> serverB{1002, 1003, 1005, 1011};
    std::vector<long> timeline;
    std::merge(serverA.begin(), serverA.end(),
               serverB.begin(), serverB.end(),
               std::back_inserter(timeline));
    std::cout << "merge logs: ";
    for (long t : timeline) std::cout << t << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1768: 交替合併字串 (Merge Strings Alternately)
// ----------------------------------------------------------------
// 題目:給兩字串 word1 與 word2,輪流取字元,任一耗盡後接上剩餘。
//
// 為什麼順便對比 std::merge:
//   雖然這題不適合直接用 std::merge (它做的是「比較大小」合併,不是「輪流」),
//   但這題是「合併兩序列」的概念變體 — 兩種「合併」模式對比有助學習。
//   這裡示範手寫的輪流合併。
//
// 複雜度:時間 O(n + m);空間 O(n + m)。
void leetcode_1768_merge_alternately() {
    std::string w1 = "abc", w2 = "pqr";
    std::string out;
    size_t i = 0, j = 0;
    while (i < w1.size() && j < w2.size()) { out += w1[i++]; out += w2[j++]; }
    out += w1.substr(i);
    out += w2.substr(j);
    std::cout << "LC1768: " << out << '\n';
}

// ----------------------------------------------------------------
// 實務範例:合併兩個事件流 (按時戳)
// ----------------------------------------------------------------
// 場景:兩個獨立事件源 (例如鍵盤、滑鼠) 各自輸出帶時戳的事件,
//      要合併成一條時間軸傳給處理器。兩流皆按時戳排序 → std::merge。
void practical_merge_two_event_streams() {
    struct Ev { long ts; std::string src; };
    auto cmp = [](const Ev& a, const Ev& b){ return a.ts < b.ts; };
    std::vector<Ev> kb{{100, "KB:A"}, {300, "KB:B"}, {500, "KB:C"}};
    std::vector<Ev> mouse{{150, "M:click"}, {350, "M:move"}, {600, "M:drag"}};
    std::vector<Ev> merged;
    std::merge(kb.begin(), kb.end(), mouse.begin(), mouse.end(),
               std::back_inserter(merged), cmp);
    std::cout << "merged timeline:";
    for (auto& e : merged) std::cout << " " << e.ts << "(" << e.src << ")";
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// merge: 1 2 3 3 4 5 7 8
// merge desc: 9 8 7 6 5 1
// merge with duplicates: 1 2 2 2 2 3 4
// LC88: 1 2 2 3 5 6
// LC21: 1 1 2 3 4 4
// merge logs: 1001 1002 1003 1003 1005 1007 1010 1011
// LC1768: apbqcr
// merged timeline: 100(KB:A) 150(M:click) 300(KB:B) 350(M:move) 500(KB:C) 600(M:drag)
