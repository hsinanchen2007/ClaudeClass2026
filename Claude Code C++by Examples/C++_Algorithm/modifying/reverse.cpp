// ============================================================
// std::reverse / std::reverse_copy
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/reverse
//   * https://en.cppreference.com/w/cpp/algorithm/reverse_copy
//   * https://cplusplus.com/reference/algorithm/reverse/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::reverse 的目的就一句話:「把這段元素的順序反過來」。
//
// 原理:雙端指針 — 一個從前、一個從後,兩兩 swap,直到相遇。
// 因此恰好需要 N/2 次 swap,O(N) 時間、O(1) 空間。
//
// 兩個版本:
//
//   ┌──────────────┬─────────────────────────────────────────┐
//   │ reverse      │ 就地反轉 (in-place),原容器被修改        │
//   │ reverse_copy │ 反轉結果寫到輸出端,原容器不動           │
//   └──────────────┴─────────────────────────────────────────┘
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、什麼時候不需要呼叫 reverse?                           │
// └────────────────────────────────────────────────────────────┘
//
// 如果你只是想「反向走訪」(讀取),根本不需要修改容器 —
// 改用 rbegin() / rend() 反向迭代器就好:
//
//   for (auto it = v.rbegin(); it != v.rend(); ++it) ...
//
// 真的有「反轉後資料要保留下來」才需要呼叫 std::reverse。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、reverse 在更大演算法中的角色                           │
// └────────────────────────────────────────────────────────────┘
//
// reverse 不只是「把陣列倒過來」 — 它有些經典的延伸用法:
//
//   * 「三次反轉法」做陣列旋轉:
//       reverse(整個);  reverse(前 k);  reverse(後 n-k);
//       → 實作 LeetCode 189 旋轉陣列,O(n) 時間 O(1) 空間。
//
//   * 「下一個排列」(next_permutation) 的最後一步是 reverse 一段尾巴。
//
//   * 「反轉子字串」「反轉鏈表的某段」等,都是雙端指針 + reverse 思路。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class BidirIt>
//   void reverse(BidirIt first, BidirIt last);
//
//   template <class BidirIt, class OutputIt>
//   OutputIt reverse_copy(BidirIt first, BidirIt last, OutputIt d_first);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、需求 (Requirements)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   * reverse:      BidirectionalIterator (要能往回走,所以 list 也行)
//   * reverse_copy: 來源 BidirectionalIterator,目的 OutputIterator
//
//   注意:對 std::list 雖能用,但 list 自帶 list::reverse() 成員函式,
//   它不必 swap 元素,只要重接節點指標,O(N) 但常數更小,優先使用。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: reverse N/2 次 swap;reverse_copy N 次指派
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 只想反向「走訪」就用 rbegin/rend,不要無謂呼叫 reverse。
//   2. 對 std::list,優先用 list::reverse() 成員函式。
//   3. 對 string 也適用 (它也是字元的 sequence)。
//   4. reverse 是 O(N),不是 O(1) — 別在熱路徑反覆呼叫。
//
// ============================================================

/*
補充筆記：std::reverse
  - reverse 原地反轉範圍順序，需要 bidirectional iterator。
  - 它改變元素位置但不改變元素值本身。
  - 若外部保存索引或 iterator，反轉後語意位置已經不同。
  - std::reverse 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ===========================================================================
// 【面試題】std::reverse / reverse_copy
// ---------------------------------------------------------------------------
// 🔥 Q1. std::reverse 的迭代器需求與複雜度?
//     答:需要 bidirectional iterator(頭尾往中間夾,逐對 swap),恰好 N/2 次 swap,
//         時間 O(n)、空間 O(1)、in-place。它只改變元素的「位置」,不改變元素的值。
//         因為只要 bidirectional,所以 std::list 也可以用——但 list 有更好的成員版。
//
// 🔥 Q2. 只是想「反向走訪」,該用 reverse 嗎?
//     答:不該。要反向讀就用 rbegin()/rend()(或 C++20 的 std::views::reverse),
//         那是 O(1) 建立、完全不動資料。std::reverse 是 O(n) 而且會真的改寫容器內容,
//         在熱路徑上反覆呼叫是常見的效能地雷。
//
// Q3. reverse 和 rotate 的關係?
//     答:rotate 可用三次 reverse 實作——reverse 前段、reverse 後段、再 reverse 全體,
//         這是「陣列循環移位」的經典解法。反過來說,reverse 是 rotate 的組成零件。
//
// Q4. 為什麼 std::list 要用成員 list::reverse() 而不是 std::reverse?
//     答:成員版只重接節點的前後指標,不搬移元素本身,對重型元素更快,而且 iterator
//         仍跟著原本的元素走。通用的 std::reverse 是逐對 swap 元素值。
//         同樣的「成員版優先」原則也適用於 list::sort、list::unique、list::remove。
// ===========================================================================

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 反轉整個 vector ---
    std::vector<int> v{1, 2, 3, 4, 5};
    std::reverse(v.begin(), v.end());
    std::cout << "reverse: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 反轉部分範圍 (子範圍 reverse) ---
    std::vector<int> w{1, 2, 3, 4, 5, 6};
    std::reverse(w.begin() + 1, w.begin() + 5);
    std::cout << "partial reverse: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: reverse_copy (原容器不變) ---
    std::vector<int> src{1, 2, 3, 4, 5};
    std::vector<int> dst;
    std::reverse_copy(src.begin(), src.end(), std::back_inserter(dst));
    std::cout << "src: ";
    for (int x : src) std::cout << x << ' ';
    std::cout << '\n';
    std::cout << "dst: ";
    for (int x : dst) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 4: 反轉字串 ---
    std::string s = "Hello!";
    std::reverse(s.begin(), s.end());
    std::cout << "reversed string: " << s << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_344_reverse_string();
    void leetcode_189_rotate_array_via_reverse();
    void practical_recent_events_view();
    void leetcode_151_reverse_words();
    void practical_undo_stack_replay();
    leetcode_344_reverse_string();
    leetcode_189_rotate_array_via_reverse();
    practical_recent_events_view();
    leetcode_151_reverse_words();
    practical_undo_stack_replay();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 344: 反轉字串 (Reverse String)
// ----------------------------------------------------------------
// 題目:給字元陣列 s,原地反轉 (空間 O(1))。
//
// 為什麼用 std::reverse:
//   題目 100% 對應「反轉序列」 — 一行就解決。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_344_reverse_string() {
    std::vector<char> s{'h','e','l','l','o'};
    std::reverse(s.begin(), s.end());
    std::cout << "LC344: ";
    for (char c : s) std::cout << c;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 189: 旋轉陣列 (Rotate Array) — 三次反轉法
// ----------------------------------------------------------------
// 題目:給陣列 nums 與 k,將 nums 向右旋轉 k 步 (in-place,O(1) 額外空間)。
//      例如 [1,2,3,4,5,6,7], k=3 → [5,6,7,1,2,3,4]。
//
// 為什麼用 std::reverse 三次:
//   利用「reverse 兩段拼接 = 整段拼接後的反轉」這個數學性質。
//   不需任何額外陣列,O(1) 空間,程式碼短小漂亮。
//
// 解法步驟:
//   k = k % n  (處理 k > n 的情況)
//   1. reverse 整個 nums          → 末段變到前面 (但順序反了)
//   2. reverse 前 k 個            → 前段恢復正向
//   3. reverse 後 n-k 個          → 後段恢復正向
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_189_rotate_array_via_reverse() {
    std::vector<int> nums{1,2,3,4,5,6,7};
    int k = 3;
    int n = static_cast<int>(nums.size());
    k %= n;
    std::reverse(nums.begin(), nums.end());
    std::reverse(nums.begin(), nums.begin() + k);
    std::reverse(nums.begin() + k, nums.end());
    std::cout << "LC189: ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:把「最舊→最新」的事件清單轉成「最新→最舊」顯示
// ----------------------------------------------------------------
// 場景:事件按時間順序 push_back 進 vector,UI 卻要「最新在最上面」。
//
// 為什麼用 std::reverse_copy:
//   不想動到原來的時序資料 (它可能還有別的 consumer),
//   就把反轉結果寫到「顯示用」的 vector 中。
void practical_recent_events_view() {
    std::vector<std::string> events{"login", "click", "purchase", "logout"};
    std::vector<std::string> view;
    std::reverse_copy(events.begin(), events.end(), std::back_inserter(view));
    std::cout << "Recent first: ";
    for (auto& s : view) std::cout << s << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 151: 反轉字串中的單字 (Reverse Words in a String)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給字串 s,反轉字串中「單字」的順序 (單字之間以空白分隔)。
//      例如 "the sky is blue" → "blue is sky the"。
//
// 為什麼用 std::reverse 兩次:
//   1. 反轉整個字串。
//   2. 對每個「單字段」再反轉一次,還原單字內部順序。
//   這個「兩次反轉」是 LC 189 思想的字串版。
//
// 複雜度:時間 O(n);空間 O(1) (in-place)。
void leetcode_151_reverse_words() {
    std::string s = "the sky is blue";
    std::reverse(s.begin(), s.end());
    auto i = s.begin();
    while (i != s.end()) {
        auto j = std::find(i, s.end(), ' ');
        std::reverse(i, j);
        if (j == s.end()) break;
        i = j + 1;
    }
    std::cout << "LC151: " << s << '\n';
}

// ----------------------------------------------------------------
// 實務範例:Undo Stack 重播 (反向遍歷)
// ----------------------------------------------------------------
// 場景:編輯器的 redo 操作要從 undo stack 末端往前依序重播,
//      若想直接「正向遍歷」,可先 reverse 一份副本。
void practical_undo_stack_replay() {
    std::vector<std::string> undo{"insert(A)", "insert(B)", "delete(A)", "insert(C)"};
    std::vector<std::string> replay = undo;
    std::reverse(replay.begin(), replay.end());
    std::cout << "replay order:";
    for (auto& s : replay) std::cout << " " << s;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// reverse: 5 4 3 2 1
// partial reverse: 1 5 4 3 2 6
// src: 1 2 3 4 5
// dst: 5 4 3 2 1
// reversed string: !olleH
// LC344: olleh
// LC189: 5 6 7 1 2 3 4
// Recent first: logout purchase click login
// LC151: blue is sky the
// replay order: insert(C) delete(A) insert(B) insert(A)
