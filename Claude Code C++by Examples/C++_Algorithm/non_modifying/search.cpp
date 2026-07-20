// ============================================================
// std::search
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/search
//   * https://cplusplus.com/reference/algorithm/search/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::search 解決的問題:
//
//   「在主序列裡,某個子序列『第一次』出現的位置在哪?」
//
// 也就是 C 語言裡 strstr / Java 裡 indexOf 那種「子字串搜尋」概念,
// 但 std::search 推廣到「任何序列」 — 不只是字串,任何容器都行。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、search vs find_end vs find_first_of                    │
// └────────────────────────────────────────────────────────────┘
//
//   ┌──────────────────┬─────────────────────────────────────┐
//   │ search           │ 子序列『按順序』第一次出現           │
//   │ find_end         │ 子序列『按順序』最後一次出現         │
//   │ find_first_of    │ 集合中『任一元素』第一次出現         │
//   │ find             │ 單一 value 第一次出現                │
//   └──────────────────┴─────────────────────────────────────┘
//
// 「按順序」是 search 與 find_first_of 最關鍵的差異:
//   * search   要 sub 中每個元素都按順序出現 (像找子字串)
//   * find_first_of 只要主序列遇到「集合中任一」就停 (像找母音)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、C++17 引入的 Searcher 物件 (高階加速)                  │
// └────────────────────────────────────────────────────────────┘
//
// 標準的 std::search 預設用「樸素演算法」(O(N×M))。
// 對長 pattern 或大量重複搜尋時效率不夠好。
// C++17 起加入了「Searcher 物件」版本,可選用更快的搜尋算法:
//
//   * std::default_searcher           — 等同樸素 search
//   * std::boyer_moore_searcher       — Boyer-Moore (平均 O(N+M))
//   * std::boyer_moore_horspool_searcher — Boyer-Moore-Horspool 變體
//
// 用法:
//   auto bm = std::boyer_moore_searcher(pat.begin(), pat.end());
//   auto it = std::search(text.begin(), text.end(), bm);
//
// 對「同一個 pattern 多次搜尋」效益最大 — 因為 Boyer-Moore 的
// 預處理表只算一次。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   // (1) 經典版
//   template <class FwdIt1, class FwdIt2>
//   FwdIt1 search(FwdIt1 first, FwdIt1 last,
//                 FwdIt2 s_first, FwdIt2 s_last);
//
//   // (2) 經典版 + 自訂比較
//   template <class FwdIt1, class FwdIt2, class BinaryPred>
//   FwdIt1 search(FwdIt1 first, FwdIt1 last,
//                 FwdIt2 s_first, FwdIt2 s_last,
//                 BinaryPred p);
//
//   // (3) C++17 起 — 使用 Searcher 物件
//   template <class FwdIt, class Searcher>
//   FwdIt search(FwdIt first, FwdIt last, const Searcher& searcher);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   * 找到 → 子序列在主序列中的「起始位置」
//   * 找不到 → last
//   * 子序列為空 → first  ★ 注意:不是 last,是 first!
//
// 與 find_end 相反 — find_end 對空子序列回 last。要記清楚這個差異。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   經典版:    O(N × S) — 樸素逐字元比對
//   Boyer-Moore: 預處理 O(S),搜尋平均 O(N + S)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 空子序列回 first (與 find_end 不同)。
//   2. 大量重複搜尋同一 pattern → 用 Boyer-Moore searcher。
//   3. 不支援萬用字元/正規表達式;要 regex 請用 <regex>。
//   4. searcher 物件需 #include <functional>。
//
// ============================================================


/*
補充筆記：std::search
  - std::search 用來尋找「一段子序列」第一次出現在主序列的位置；這和 std::find 只找單一元素不同。
  - 如果 pattern 為空，std::search 會回傳 first，代表空序列可視為在開頭被找到；這個邊界和 find_end 不同。
  - 回傳 last 代表找不到，解參考前必須先檢查；若要取得索引，vector/string 可用 it - begin()，list 則不支援常數時間相減。
  - 經典 overload 需要兩個 range：[first,last) 是主序列，[s_first,s_last) 是 pattern；兩段 range 都用半開區間表示。
  - 自訂 BinaryPred 可用來做大小寫不敏感或欄位比較，但 predicate 必須對同一組輸入保持一致，不要在比較時修改元素。
  - C++17 searcher 物件如 boyer_moore_searcher 適合同一 pattern 重複搜尋很多次，因為 pattern 的預處理表可以重用。
  - std::search 不是 regex，也不支援萬用字元；需要正規表示式時應使用 <regex> 或專門字串搜尋工具。
  - 對 std::string 找子字串時，string::find 通常更直接；std::search 的價值在於它能套用到任何 iterator range。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::search
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. search / find_end / find_first_of / find 四者怎麼區分?
//     答:search = 子序列「按順序」第一次出現;find_end = 同樣的子序列最後一次出現;
//         find_first_of = 候選集合中「任一元素」第一次出現(字元集模式);
//         find = 單一 value 第一次出現。
//         關鍵字是「按順序」— 這是 search 與 find_first_of 最大的差異。
//     追問:回傳值語意一致嗎?(search 與 find_end 回傳的都是該次出現的「起始位置」)
//
// 🔥 Q2. C++17 為 search 加了什麼?什麼時候值得用?
//     答:加了「searcher 物件」多載 search(first, last, searcher),
//         可選 std::default_searcher、std::boyer_moore_searcher、
//         std::boyer_moore_horspool_searcher(需 #include <functional>)。
//         Boyer-Moore 系列會先對 pattern 做預處理表,再據以一次跳過多個位置;
//         因為預處理只做一次,對「同一個 pattern 反覆搜尋大量文字」效益最大。
//     追問:那 find_end 也有 searcher 版嗎?(沒有 — 只有 search 有)
//
// ⚠️ 陷阱 Q3. 子序列(pattern)為空時,std::search 回傳什麼?
//     答:回傳 first,不是 last —— 空 pattern 被視為「在開頭就匹配成功」。
//         而 std::find_end 對空子序列回傳的是 last。兩者剛好相反,要分開記。
//     為什麼會錯:習慣把「找不到就回 last」當成通則,
//         於是把空 pattern 也歸到「找不到」那一類;
//         但空序列在語意上是「處處都匹配」,search 取最前面那個位置。
//
// Q4. 經典版(非 searcher)的複雜度是多少?
//     答:最壞 O(N × S) — 樸素逐一對齊比對,N 為主序列長、S 為子序列長。
//         對 std::string 找子字串,std::string::find 通常更直接;
//         std::search 的價值在於它能套用到任何 iterator 區間,不限字串。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <functional>   // searchers
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{4, 1, 2, 3, 9, 1, 2, 3, 5};
    std::vector<int> sub{1, 2, 3};

    // --- 範例 1: 找子序列首次出現 ---
    auto it = std::search(v.begin(), v.end(), sub.begin(), sub.end());
    if (it != v.end())
        std::cout << "first occurrence at index " << (it - v.begin()) << '\n';

    // --- 範例 2: 子序列為空 → 回傳 first (注意:與 find_end 相反) ---
    std::vector<int> empty;
    auto e = std::search(v.begin(), v.end(), empty.begin(), empty.end());
    std::cout << "empty pattern: index = " << (e - v.begin()) << '\n';

    // --- 範例 3: 字串子字串搜尋 ---
    std::string text = "the quick brown fox jumps over the lazy dog";
    std::string pat  = "fox";
    auto sit = std::search(text.begin(), text.end(), pat.begin(), pat.end());
    std::cout << "'fox' at index " << (sit - text.begin()) << '\n';

    // --- 範例 4: C++17 Boyer-Moore searcher (適合長 pattern + 多次搜尋) ---
    auto bm = std::search(text.begin(), text.end(),
                          std::boyer_moore_searcher(pat.begin(), pat.end()));
    std::cout << "BM 'fox' at index " << (bm - text.begin()) << '\n';

    // --- 範例 5: 找不到 ---
    std::vector<int> none{99};
    auto nf = std::search(v.begin(), v.end(), none.begin(), none.end());
    std::cout << "{99} not found: " << (nf == v.end() ? "yes" : "no") << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_28_strstr();
    void practical_log_pattern_search();
    void leetcode_2278_percentage_substring();
    void practical_binary_pattern_match();
    leetcode_28_strstr();
    practical_log_pattern_search();
    leetcode_2278_percentage_substring();
    practical_binary_pattern_match();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 28: 找出字串中第一個匹配的下標
//              (Find the Index of the First Occurrence in a String)
// ----------------------------------------------------------------
// 題目:給字串 haystack 與 needle,回傳 needle 在 haystack 中
//      第一次出現的索引;若不存在則回傳 -1。等同 strStr() / indexOf。
//
// 為什麼用 std::search:
//   題意完全等同「子序列第一次出現」 — 這就是 search 的職責。
//   題目規定 needle 為空時回傳 0,正好對應「空子序列回 first」。
//
// 解法步驟:
//   1. std::search(haystack, needle)。
//   2. 回傳 == end() → -1;否則回傳 distance(begin, it)。
//
// 複雜度:時間 O(N × M) (預設) / O(N + M) (Boyer-Moore);空間 O(1)。
void leetcode_28_strstr() {
    std::string haystack = "sadbutsad";
    std::string needle   = "sad";
    auto it = std::search(haystack.begin(), haystack.end(),
                          needle.begin(), needle.end());
    int idx = (it == haystack.end()) ? -1
                                     : static_cast<int>(it - haystack.begin());
    std::cout << "LC28: index = " << idx << '\n';
}

// ----------------------------------------------------------------
// 實務範例:在大段 log 中尋找特定錯誤序列
// ----------------------------------------------------------------
// 場景:監控系統需要在連續多行 log 中找特定的「錯誤序列」 —
//      例如「TIMEOUT 後緊接 RECONNECT_FAIL 再接 ABORT」這種模式。
//
// 為什麼用 std::search:
//   把 log 看成 vector<string>,把錯誤模式也是 vector<string>,
//   用 std::search 就能找到「按順序出現」的位置。
//   字串只是元素;search 對任何能比較的型別都適用。
void practical_log_pattern_search() {
    std::vector<std::string> log{
        "OK", "OK", "TIMEOUT", "RECONNECT_FAIL", "ABORT", "OK", "OK"
    };
    std::vector<std::string> pat{
        "TIMEOUT", "RECONNECT_FAIL", "ABORT"
    };
    auto it = std::search(log.begin(), log.end(),
                          pat.begin(), pat.end());
    if (it != log.end())
        std::cout << "log pattern found at line " << (it - log.begin()) << '\n';
    else
        std::cout << "log pattern not found\n";
}

// ----------------------------------------------------------------
// LeetCode 2278: 字串中字元出現的百分比 (用 search 變體計次)
// ----------------------------------------------------------------
// 題目:給字串 s 與字元 letter,回傳 letter 占 s 長度的百分比 (向下取整)。
//      雖然這題用 std::count 最直接,我們示範用 search 找子字串
//      ((letter)) 的子串匹配版本作為「子序列搜尋」教學。
//
// 為什麼用 std::search (作為 demo):
//   把單字元 letter 視為長度為 1 的 pattern,search 走訪一次,
//   每次找到就 ++count、再從 hit + 1 繼續。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2278_percentage_substring() {
    std::string s = "foobar", pattern(1, 'o');
    int count = 0;
    auto it = s.begin();
    while (true) {
        auto hit = std::search(it, s.end(), pattern.begin(), pattern.end());
        if (hit == s.end()) break;
        ++count;
        it = hit + 1;
    }
    int percent = static_cast<int>(100LL * count / s.size());
    std::cout << "LC2278: count=" << count << " pct=" << percent << '\n';
}

// ----------------------------------------------------------------
// 實務範例:在 binary stream 中尋找已知 packet header
// ----------------------------------------------------------------
// 場景:解析自訂二進位協定時,要從 buffer 中找特定 magic bytes
//      作為 packet 起點;std::search 即可,內部會做 byte-by-byte 比對。
void practical_binary_pattern_match() {
    std::vector<unsigned char> stream{
        0x01, 0x02, 0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x11, 0xCA, 0xFE
    };
    std::vector<unsigned char> magic{0xCA, 0xFE, 0xBA, 0xBE};
    auto it = std::search(stream.begin(), stream.end(),
                          magic.begin(), magic.end());
    if (it != stream.end())
        std::cout << "magic found at offset: " << (it - stream.begin()) << '\n';
    else
        std::cout << "no magic\n";
}

// 編譯: g++ -std=c++20 -Wall -Wextra search.cpp -o search

// === 預期輸出 ===
// first occurrence at index 1
// empty pattern: index = 0
// 'fox' at index 16
// BM 'fox' at index 16
// {99} not found: yes
// LC28: index = 0
// log pattern found at line 2
// LC2278: count=2 pct=33
// magic found at offset: 2
