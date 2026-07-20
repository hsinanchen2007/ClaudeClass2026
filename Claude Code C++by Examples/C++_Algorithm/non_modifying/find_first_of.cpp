// ============================================================
// std::find_first_of
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/find_first_of
//   * https://cplusplus.com/reference/algorithm/find_first_of/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::find_first_of 解的問題:
//
//   「在主序列裡,找第一個『等於候選集合中任一元素』的位置。」
//
// 想像情境:
//   * 在字串裡找「第一個母音」 — 候選集合 = {a, e, i, o, u}
//   * 在 URL 裡找「第一個分隔符」 — 候選集合 = {?, &, #}
//   * 在 token 串裡找「第一個關鍵字」 — 候選集合 = {if, else, return ...}
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、和其他 find 系列的差別                                 │
// └────────────────────────────────────────────────────────────┘
//
//   ┌──────────────────────┬───────────────────────────────────┐
//   │ 函式                 │ 找什麼                            │
//   ├──────────────────────┼───────────────────────────────────┤
//   │ find                 │ 等於『單一』value 的元素           │
//   │ find_if              │ 述詞 p 為 true 的元素              │
//   │ find_first_of        │ 等於『集合中任一』元素的元素        │
//   │ search               │ 整個子序列『按順序』出現           │
//   │ find_end             │ 整個子序列最後一次『按順序』出現    │
//   └──────────────────────┴───────────────────────────────────┘
//
// 「集合中任一」 vs 「整個子序列按順序」 是 find_first_of 與 search 最大的區別。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class FwdIt>
//   InputIt find_first_of(InputIt first, InputIt last,
//                         FwdIt s_first, FwdIt s_last);
//
//   template <class InputIt, class FwdIt, class BinaryPred>
//   InputIt find_first_of(InputIt first, InputIt last,
//                         FwdIt s_first, FwdIt s_last,
//                         BinaryPred p);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//   * C++20 引入 std::ranges::find_first_of。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   * 找到 → 主序列中第一個符合的元素的迭代器
//   * 找不到 (含候選集合為空) → last
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 (N) × (S) 次比較,N = 主序列長, S = 候選集合長
//   空間: O(1)
//
//   候選集合若是大量資料且事先已排序,可考慮自己改用 binary_search 加速
//   (find_first_of 內部不會做這個最佳化)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、需求 (Requirements)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   * InputIt: LegacyInputIterator
//   * FwdIt:   LegacyForwardIterator (因為候選集合可能要被多次掃描)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 與 search 不同 — search 找「整個子序列出現」,
//      find_first_of 是「集合中任一元素出現」。容易混淆。
//   2. 候選集合為空 → 回傳 last。
//   3. 對大集合需高效查詢,自己用 unordered_set 配 find_if 通常更快。
//
// ============================================================

/*
補充筆記：std::find_first_of
  - find_first_of 找第一個屬於另一組候選元素的值。
  - 第二段範圍是候選集合；若候選很多，unordered_set 可能比線性比對更合適。
  - 自訂 predicate 可以把大小寫不敏感等規則放進比對。
  - std::find_first_of 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::find_first_of
// ───────────────────────────────────────────────────────────────────────────
// ⚠️ 陷阱 Q1. find_first_of 和 std::search 都接受兩段區間,差別在哪?
//     答:語意完全不同。
//         search 要求第二段「整個子序列、按順序」出現(子字串模式);
//         find_first_of 只要主序列中某個元素「等於第二段裡的任何一個」就命中
//         (字元集模式,像找第一個母音、第一個分隔符)。
//     為什麼會錯:兩者簽章長得幾乎一樣(都是四個 iterator),
//         很容易以為第二段的角色也一樣;
//         實際上一個是 pattern,另一個是候選集合。
//
// 🔥 Q2. 為什麼第二段區間要求 ForwardIterator,第一段卻只要 InputIterator?
//     答:因為主序列只需要從頭走一次(InputIterator 就夠),
//         但候選集合對主序列的「每一個」元素都要重新掃一遍,
//         必須支援多次走訪 — 這正是 ForwardIterator 相對 InputIterator 的關鍵能力。
//
// Q3. 複雜度是多少?候選集合很大時該怎麼辦?
//     答:最多 N × S 次比較(N 為主序列長、S 為候選集合長),空間 O(1) —
//         它內部就是對每個元素做線性掃描,不會自動改用二分搜尋或雜湊。
//         候選集合大時,自己把它放進 std::unordered_set 再配 find_if 查表,
//         通常快得多(平均 O(N))。
//     追問:候選集合為空會怎樣?(回傳 last,視為找不到)
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 找第一個母音 ---
    std::string s = "rhythm and blues";
    std::string vowels = "aeiou";
    auto it = std::find_first_of(s.begin(), s.end(),
                                 vowels.begin(), vowels.end());
    if (it != s.end())
        std::cout << "first vowel '" << *it
                  << "' at index " << (it - s.begin()) << '\n';
    else
        std::cout << "no vowel found\n";

    // --- 範例 2: 找第一個出現的「分隔字元」 ---
    std::string line = "abc,def;ghi";
    std::string delims = ",;|";
    auto d = std::find_first_of(line.begin(), line.end(),
                                delims.begin(), delims.end());
    std::cout << "first delim at index " << (d - line.begin())
              << " char='" << *d << "'\n";

    // --- 範例 3: 自訂比較 — 整數的「絕對值是否屬於候選集合」 ---
    std::vector<int> nums{ 5, -3, 7, 2, -8 };
    std::vector<int> targets{ 3, 8 };
    auto m = std::find_first_of(nums.begin(), nums.end(),
                                targets.begin(), targets.end(),
                                [](int a, int b){ return std::abs(a) == b; });
    if (m != nums.end())
        std::cout << "first match (by abs) = " << *m << '\n';

    // --- 範例 4: 候選集合為空 → 回傳 last ---
    std::vector<int> none;
    auto e = std::find_first_of(nums.begin(), nums.end(),
                                none.begin(), none.end());
    std::cout << "empty candidates: " << (e == nums.end() ? "end" : "found") << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_345_first_vowel_index();
    void practical_first_separator();
    void leetcode_first_uppercase_letter();
    void practical_first_command_keyword();
    leetcode_345_first_vowel_index();
    practical_first_separator();
    leetcode_first_uppercase_letter();
    practical_first_command_keyword();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 345 子題:字串中第一個母音的索引
// ----------------------------------------------------------------
// 題目:給字串 s,回傳第一個母音 (a, e, i, o, u, 不分大小寫) 的索引;
//      若無則回傳 -1。LC 345 本身是「反轉字串中的母音」,核心子問題
//      就是「找母音」 — find_first_of 是最直接的工具。
//
// 為什麼用 std::find_first_of:
//   候選集合 = {a, e, i, o, u, A, E, I, O, U},正是「集合中任一」的場景。
//
// 解法步驟:
//   1. 把 10 個母音字元放在一個字串 vowels 中當候選集合。
//   2. find_first_of 一次掃描即可找到第一個屬於該集合的字元。
//
// 複雜度:時間 O(N × K),K 是候選集合大小 (此處常數 10);空間 O(1)。
void leetcode_345_first_vowel_index() {
    std::string s = "TryHElloWorld";
    std::string vowels = "aeiouAEIOU";
    auto it = std::find_first_of(s.begin(), s.end(),
                                 vowels.begin(), vowels.end());
    int idx = (it == s.end()) ? -1 : static_cast<int>(it - s.begin());
    std::cout << "LC345-variant: first vowel index = " << idx << '\n';
}

// ----------------------------------------------------------------
// 實務範例:解析 URL 的第一段 (找第一個分隔符)
// ----------------------------------------------------------------
// 場景:後端拿到 "user=alice&token=xyz#frag",要切出第一段 "user=alice"。
//      分隔符可能是 '&'、'#'、'?',我們不在乎是哪一個 — 只要遇到任一個就停。
//
// 為什麼用 std::find_first_of:
//   分隔符集合 = {&, #, ?},「集合中任一」的場景再典型不過。
//   單字元也能做、多字元集合也能做,通用性很好。
void practical_first_separator() {
    std::string url = "user=alice&token=xyz#frag";
    std::string seps = "&#?";
    auto it = std::find_first_of(url.begin(), url.end(),
                                 seps.begin(), seps.end());
    std::string head = std::string(url.begin(), it);
    std::cout << "first segment = " << head << '\n';
}

// ----------------------------------------------------------------
// LeetCode 概念題:找第一個大寫字母的位置 (Capitalize Detection)
// ----------------------------------------------------------------
// 題目:給字串 s,找第一個大寫字母 (A..Z) 出現的位置;找不到回 -1。
//
// 為什麼用 std::find_first_of:
//   候選集合是「A..Z」(26 字元) — 完美對應 find_first_of 的「集合任一」。
//   不必手寫迴圈或預先建表。
//
// 複雜度:時間 O(N × 26) = O(N);空間 O(1)。
void leetcode_first_uppercase_letter() {
    std::string s = "hello World";
    std::string uppers;
    for (char c = 'A'; c <= 'Z'; ++c) uppers.push_back(c);
    auto it = std::find_first_of(s.begin(), s.end(),
                                 uppers.begin(), uppers.end());
    int idx = (it == s.end()) ? -1 : static_cast<int>(it - s.begin());
    std::cout << "first upper idx: " << idx << '\n';
}

// ----------------------------------------------------------------
// 實務範例:找命令字串中第一個關鍵字 (例如 SQL 子句邊界)
// ----------------------------------------------------------------
// 場景:解析 "SELECT * FROM t WHERE a > 0 ORDER BY a" 時,要從 WHERE 起點
//      找下一個邊界關鍵字 (ORDER, GROUP, LIMIT...)。
//      用 find_first_of 的字元版只能找單字元;這裡示範用普通迭代 + 多次搜尋。
void practical_first_command_keyword() {
    std::string cmd = "SELECT * FROM t WHERE a > 0 ORDER BY a";
    // 用字元版示範 "找空格之後接 O/G/L 的位置" — 簡化:找 "OGL" 任一字元
    std::string starts = "OGL";
    auto it = std::find_first_of(cmd.begin() + 20, cmd.end(),
                                 starts.begin(), starts.end());
    int pos = (it == cmd.end()) ? -1 : static_cast<int>(it - cmd.begin());
    std::cout << "next clause-start at: " << pos << '\n';
}

// === 預期輸出 (Expected output) ===
// first vowel 'a' at index 7
// first delim at index 3 char=','
// first match (by abs) = -3
// empty candidates: end
// LC345-variant: first vowel index = 4
// first segment = user=alice
// first upper idx: 6
// next clause-start at: 28
