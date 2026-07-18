// ============================================================
// std::find_end
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/find_end
//   * https://cplusplus.com/reference/algorithm/find_end/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::find_end 解決的問題:
//
//   「在主序列裡,某個子序列『最後一次』出現的位置在哪?」
//
// 注意名稱裡的 "end" — 它不是指「子序列的尾端」,而是
// 「找到的子序列在主序列中『最末』的那次出現」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、find_end vs search 對照                                │
// └────────────────────────────────────────────────────────────┘
//
//   * std::search   找「第一次」出現
//   * std::find_end 找「最後一次」出現
//
// 兩者用法、回傳值、複雜度幾乎一致,差別只在「找哪一個出現」。
// 很常見的用法:
//   * 取出 path 的最後一個 '/' 之後 → 取檔名
//   * 從一段 log 裡找最後一次某種錯誤的位置 (做事故分析)
//   * 字串解析時找最後一次的分隔符
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、它是怎麼運作的?                                       │
// └────────────────────────────────────────────────────────────┘
//
// 概念上像是「從右往左 scan」: 尋找最後一個對齊位置 i,
// 使得 main[i .. i+M) == sub[0 .. M)。
//
// 實作上一般是「從左往右掃描,記錄目前找到的最右匹配位置」,
// 但邏輯結果等價。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt1, class FwdIt2>
//   FwdIt1 find_end(FwdIt1 first, FwdIt1 last,
//                   FwdIt2 s_first, FwdIt2 s_last);
//
//   template <class FwdIt1, class FwdIt2, class BinaryPred>
//   FwdIt1 find_end(FwdIt1 first, FwdIt1 last,
//                   FwdIt2 s_first, FwdIt2 s_last,
//                   BinaryPred p);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//   * C++20 引入 std::ranges::find_end。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   * 找到 → 指向最後一次出現中「子序列首字元」的迭代器
//   * 找不到 → last
//   * 子序列為空 → last (注意! 不是回傳 last 表示「在尾端找到空字串」)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 S × (N - S + 1) 次比較,N = 主序列長, S = 子序列長
//   空間: O(1)
//
//   若需要更高效的字串搜尋 (如 KMP / Boyer-Moore),
//   一般要自己實作或用第三方庫;標準庫的 find_end 採通用簡單演算法。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 別跟 search 混淆:search 是「第一次」,find_end 是「最後一次」。
//   2. 子序列為空時回傳 last,不會給你「end()」當作技術性命中。
//   3. 它是逐字比對,不是正規表達式。要 regex 請用 <regex>。
//   4. 想取「最後一個分隔符後的子字串」,std::string::rfind 通常更直接。
//
// ============================================================

/*
補充筆記：std::find_end
  - find_end 找最後一次出現的子序列，不是從尾端找單一元素。
  - 它適合處理「最後一段 pattern」這類需求，例如最後一個分隔片段。
  - 找不到時回傳 last；找到時回傳子序列開頭。
  - std::find_end 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 4, 1, 2, 3, 5, 1, 2, 3};
    std::vector<int> sub{1, 2, 3};

    // --- 範例 1: 找子序列 {1,2,3} 最後一次出現 ---
    // v 中 {1,2,3} 出現三次 (index 0, 4, 8),find_end 回傳 index 8。
    auto it = std::find_end(v.begin(), v.end(), sub.begin(), sub.end());
    if (it != v.end())
        std::cout << "last occurrence at index " << (it - v.begin()) << '\n';

    // --- 範例 2: 找不到的情況 ---
    std::vector<int> sub2{9, 9};
    auto it2 = std::find_end(v.begin(), v.end(), sub2.begin(), sub2.end());
    std::cout << "find {9,9}: " << (it2 == v.end() ? "not found" : "found") << '\n';

    // --- 範例 3: 自訂比較器 — 大小寫不敏感找最後一個 "world" ---
    std::string text  = "Hello WORLD, hello world!";
    std::string pat   = "world";
    auto wit = std::find_end(text.begin(), text.end(), pat.begin(), pat.end(),
                             [](char a, char b){
                                 return std::tolower(a) == std::tolower(b);
                             });
    std::cout << "case-insensitive last 'world' at index "
              << (wit - text.begin()) << '\n';

    // --- 範例 4: 子序列為空 → 回傳 last ---
    std::vector<int> empty;
    auto it3 = std::find_end(v.begin(), v.end(), empty.begin(), empty.end());
    std::cout << "empty pattern returns last: "
              << (it3 == v.end() ? "yes" : "no") << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_last_substring_index();
    void practical_basename_from_path();
    void leetcode_last_word_position();
    void practical_find_last_log_marker();
    leetcode_last_substring_index();
    practical_basename_from_path();
    leetcode_last_word_position();
    practical_find_last_log_marker();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 子題:回傳子字串「最後一次」出現的索引
// ----------------------------------------------------------------
// 題目:給 text 與 pattern,回傳 pattern 在 text 中最後一次出現的起始
//      索引;若不存在則回傳 -1。(strStr 的「rstrStr」版本)
//
// 為什麼用 std::find_end:
//   題意要的就是「最後一次出現」 — 這就是 find_end 唯一的用途。
//   不必自己手寫從右掃描的雙重迴圈。
//
// 解法步驟:
//   1. std::find_end(text.begin/end, pattern.begin/end)
//   2. 回傳值 == text.end() → -1;否則回傳 distance(begin, it)。
//
// 複雜度:時間 O(N × M),空間 O(1)。
void leetcode_last_substring_index() {
    std::string text    = "abcXYZdefXYZghi";
    std::string pattern = "XYZ";
    auto it = std::find_end(text.begin(), text.end(),
                            pattern.begin(), pattern.end());
    int idx = (it == text.end()) ? -1 : static_cast<int>(it - text.begin());
    std::cout << "LC(last-substring): index = " << idx << '\n';
}

// ----------------------------------------------------------------
// 實務範例:從檔案完整路徑取出 basename
// ----------------------------------------------------------------
// 場景:給 "/home/user/projects/main.cpp",取出最後一個 '/' 之後的內容。
//      這就是「最後一個分隔符之後的子字串」。
//
// 為什麼用 std::find_end:
//   * 把單一字元 "/" 當成長度為 1 的子序列,find_end 找最後一次出現。
//   * 也可以用 std::string::rfind 達到同樣目的;這裡示範 STL 通用做法,
//     可推廣到「分隔符是多個字元」(例如 "::" 或 "->" 之類)的情境。
void practical_basename_from_path() {
    std::string path = "/home/user/projects/main.cpp";
    std::string sep  = "/";
    auto it = std::find_end(path.begin(), path.end(), sep.begin(), sep.end());
    std::string base = (it == path.end()) ? path
                                          : std::string(it + 1, path.end());
    std::cout << "basename = " << base << '\n';
}

// ----------------------------------------------------------------
// LeetCode 58 概念:最後一個單字的長度 (Length of Last Word) — find_end 變體
// ----------------------------------------------------------------
// 題目:給字串 s,計算其中最後一個單字的長度 (單字以空格分隔)。
//
// 為什麼用 std::find_end:
//   找「最後一個空格」即可:其後到尾就是最後一個單字。
//   先 trim 尾端空格,然後 find_end(" ")。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_last_word_position() {
    std::string s = "Hello World  ";
    // 去掉尾端空格
    auto end = s.end();
    while (end != s.begin() && *(end - 1) == ' ') --end;
    std::string sep = " ";
    auto it = std::find_end(s.begin(), end, sep.begin(), sep.end());
    int len = (it == end) ? static_cast<int>(end - s.begin())
                          : static_cast<int>(end - (it + 1));
    std::cout << "LC58 last word len: " << len << '\n';
}

// ----------------------------------------------------------------
// 實務範例:在 log 檔中找「最後一筆錯誤」的位置
// ----------------------------------------------------------------
// 場景:大型 log 檔中,要快速跳到「最後一次出現 ERROR」的位置,
//      用 find_end 對序列做反向搜尋。
void practical_find_last_log_marker() {
    std::string log = "INFO: start\nINFO: ok\nERROR: x\nINFO: ok\nERROR: y\nINFO: end\n";
    std::string mark = "ERROR";
    auto it = std::find_end(log.begin(), log.end(), mark.begin(), mark.end());
    int pos = (it == log.end()) ? -1 : static_cast<int>(it - log.begin());
    std::cout << "last ERROR at offset: " << pos << '\n';
}

// === 預期輸出 (Expected output) ===
// last occurrence at index 8
// find {9,9}: not found
// case-insensitive last 'world' at index 19
// empty pattern returns last: yes
// LC(last-substring): index = 9
// basename = main.cpp
// LC58 last word len: 5
// last ERROR at offset: 39
