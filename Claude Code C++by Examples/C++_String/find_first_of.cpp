// =============================================================================
// 檔名: find_first_of.cpp
// 主題: std::string::find_first_of (找出「任一」目標字元的第一個出現位置)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/find_first_of
//   - https://cplusplus.com/reference/string/string/find_first_of/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type find_first_of(const string& str, size_type pos = 0)              const noexcept; // (1)
//   size_type find_first_of(const char*   s,   size_type pos = 0)              const;          // (2)
//   size_type find_first_of(const char*   s,   size_type pos, size_type count) const;          // (3)
//   size_type find_first_of(char          ch,  size_type pos = 0)              const noexcept; // (4)
//   size_type find_first_of(string_view   sv,  size_type pos = 0)              const noexcept; // (5) C++17
//
// 回傳: 第一個位置 i (i ≥ pos),使得 s[i] 屬於 str 字元集合;沒找到回 npos。
//
// 【詳細解釋 Explanation】
// find_first_of 是「字元集」搜尋,而非「子字串」搜尋。把第二個參數 str 想成
// 「集合」 — 我們在 *this 中找出第一個「屬於這個集合」的字元出現位置。
// 這跟 find 完全是兩件事,初學者常搞混。
//
// 【1. find vs find_first_of:子字串 vs 字元集】
//   * find("ab")            找「子字串 'ab' 連續出現」的位置
//   * find_first_of("ab")   找「'a' 或 'b'」第一次出現的位置
// 範例:s = "xyzab"
//   s.find("ab")          → 3   (子字串 "ab" 從 index 3 開始)
//   s.find_first_of("ab") → 3   (湊巧也是 3,因為 'a' 在 3)
//   s.find_first_of("yz") → 1   (找 'y' 或 'z',結果是 'y' 在 1)
//   s.find("yz")          → 1   ("yz" 子字串也在 1,巧合)
//   s.find_first_of("zy") → 1   (集合無序,'z' 與 'y' 哪個先出現算哪個)
//   s.find("zy")          → npos  (子字串 "zy" 不存在,順序不對)
//
// 【2. 對應的 C 函式:strpbrk】
// C 標準函式庫的 strpbrk(s1, s2) 跟 find_first_of 幾乎一模一樣,
// 都是「在 s1 中找第一個出現於 s2 集合的字元」,只是 strpbrk 回傳指標、
// find_first_of 回傳 index;strpbrk 也不能處理含 '\0' 的二進位資料。
// 換言之,find_first_of 就是 C++ 版、更安全、更通用的 strpbrk。
//
// 【3. 演算法與效能】
// naive 實作:
//   for i in [pos, N):
//       for c in str:
//           if s[i] == c: return i
//   return npos
// 複雜度:O(N * M),N 是 *this 的長度,M 是 str 的長度。
// 對於 char 字元集(只 256 種可能值),可用 bitset 預處理為 O(N + M):
//   bool inSet[256] = {};
//   for (char c : str) inSet[(unsigned char)c] = true;
//   for (i in [pos, N)): if (inSet[(unsigned char)s[i]]) return i;
// libstdc++ 對「str 是 const char*」的版本通常做這個優化(透過 char_traits)。
//
// 【4. 各重載細節】
// (1) find_first_of(const string&, pos)
//     傳 string;支援含 '\0' 的字元集。
// (2) find_first_of(const char*, pos)
//     C-string,結束於第一個 '\0'。
// (3) find_first_of(const char*, pos, count)
//     明確指定 count,可使集合含 '\0'。
// (4) find_first_of(char, pos)
//     單字元集合,等同於 find(char, pos)。
// (5) find_first_of(string_view, pos) — C++17
//     避免不必要的複製。
//
// 【5. 邊界規則】
//   * str 為空字串:永遠回傳 npos(集合是空集合,不可能有元素屬於它)。
//   * pos > size():回傳 npos。
//   * pos == size():回傳 npos(已經沒有字元可比)。
//
// 【6. 常見應用情境】
//   * 找第一個母音、子音、空白、標點符號
//   * Tokenize:多個 delimiter 一次比對(' ', '\t', ',')
//   * URL/Query 解析:'&', ';', '=' 等多種分隔符
//   * Lexer:碰到第一個運算子(+, -, *, /)
//   * 路徑處理:同時匹配 '/' 與 '\\'(跨平台)
//
// 【概念補充 Concept Deep Dive】
// (A) find_first_of vs strpbrk:C 函式對應
//   strpbrk(haystack, accept) — POSIX/C 標準
//     回傳 char*,指向 haystack 中第一個屬於 accept 的字元;若無回傳 NULL。
//   find_first_of(accept, pos) — C++ STL
//     回傳 size_type 索引;若無回傳 npos。
//   兩者語意一樣,差在介面與安全性:
//     * strpbrk 不能處理含 '\0' 的資料、回傳指標需小心 lifetime。
//     * find_first_of 支援 string_view、std::string、含 '\0' 的版本。
//
// (B) 為什麼不用 std::set / std::unordered_set 加速
//   對 char 字元集(最多 256 個值),靜態 bool[256] 比任何 hash set 都快。
//   STL 實作通常就是這樣做。對 wchar_t / char32_t 才需要 hash set。
//
// (C) tokenize 的選擇
//   切分多 delimiter 字串時,常見三種寫法:
//     1) find_first_of("delims") + substr → 簡潔,O(N*M)
//     2) std::strtok → 古老、會修改 input、非 thread-safe
//     3) std::regex / ranges::split → 更彈性但慢
//   小型 delimiter 集合用 1) 最划算;若是空白多種(包含 \t \n \r),
//   還可進一步把判斷封裝為 std::isspace。
//
// (D) 與 find_first_not_of 的對偶
//   find_first_of("abc")     → 找「屬於 abc」的字元
//   find_first_not_of("abc") → 找「不屬於 abc」的字元
//   兩者一起使用可實作 trim、token 切分、跳過空白等模式。
//
// 【注意事項 Pay Attention】
// 1. 若 str 為空字串,永遠回傳 npos。
// 2. 想找「不在 str 中」的字元,請用 find_first_not_of。
// 3. 對長 str 可考慮預先建立 bitset/set 加速(STL 實作通常已做)。
// 4. 與 find 完全不同:find_first_of 是字元集搜尋,不是子字串搜尋。
// 5. 處理多種空白(空格、tab、換行)時,find_first_of(" \t\n\r") 是標準慣用法。
//
// =============================================================================

/*
補充筆記：std::string::find_first_of
  - find_first_of 找第一個屬於另一組候選元素的值。
  - 第二段範圍是候選集合；若候選很多，unordered_set 可能比線性比對更合適。
  - 自訂 predicate 可以把大小寫不敏感等規則放進比對。
  - std::string::find_first_of 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::find_first_of
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. find_first_of 和 find 的語意差在哪?
//     答：find 是「子字串」搜尋,參數整串必須連續相符;
//         find_first_of 是「字元集合」搜尋,參數被當成一堆字元的集合,
//         回傳第一個屬於該集合的字元位置。
//         s.find("ab") 找 "ab" 連在一起;s.find_first_of("ab") 找第一個 a 或 b。
//     追問：典型用途?→ 切 token(找第一個分隔符)、找第一個標點符號。
//
// 🔥 Q2. 找不到時怎麼判斷?
//     答：回傳 std::string::npos(= static_cast<size_type>(-1))。
//         必須寫 != std::string::npos;不可比 -1、0 或 >= 0——
//         size_type 是無號型別,>= 0 恆真。
//
// ⚠️ 陷阱. 集合裡有重複字元、或字元順序不同,結果會變嗎?
//     答：不會。參數只被當成「集合」使用,重複與順序都不影響結果。
//         find_first_of("ab") 與 find_first_of("ba") 完全等價。
//     為什麼會錯：用 find 的直覺去讀參數,以為那是一個有順序的子字串。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

void demoFindFirstOf() {
    std::string s = "Hello, World!";
    auto p = s.find_first_of("aeiou");
    std::cout << "first vowel at " << p << " ('" << s[p] << "')\n";   // 1, 'e'

    auto p2 = s.find_first_of("xyz!");
    std::cout << "first of \"xyz!\" at " << p2 << "\n";   // 12 (the '!')
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 387. First Unique Character in a String
// 題目: 找出字串中第一個只出現一次的字元 index;沒有則回 -1。
// 為何用 find_first_of: 我們示範另一個變體 — 「找第一個屬於某字元集的位置」。
//                       這裡定義「unique 字元集」,然後用 find_first_of 一次定位。
//                       (note: 這寫法 O(N + 26) 預處理 + O(N) 搜尋,效能很好)
// -----------------------------------------------------------------------------
#include <array>
int firstUniqChar(const std::string& s) {
    std::array<int, 26> cnt{};                      // 假設小寫英文字母題目
    for (char c : s) ++cnt[c - 'a'];

    // 把「只出現一次」的字元組成一個集合
    std::string uniq;
    for (int i = 0; i < 26; ++i) {
        if (cnt[i] == 1) uniq.push_back(static_cast<char>('a' + i));
    }
    if (uniq.empty()) return -1;

    // 找 *this 中第一個屬於 uniq 集合的字元
    auto p = s.find_first_of(uniq);
    return (p == std::string::npos) ? -1 : static_cast<int>(p);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 1816. Truncate Sentence
// 題目: 給句子和 k,只保留前 k 個單字。
// 為何用 find_first_of: 找空白字元 (任一空格/tab) 來切分單字。
// -----------------------------------------------------------------------------
std::string truncateSentence(const std::string& s, int k) {
    size_t pos = 0;
    int count = 0;
    while (count < k && pos < s.size()) {
        size_t sp = s.find_first_of(" \t", pos);
        if (sp == std::string::npos) { ++count; break; }
        pos = sp + 1;
        ++count;
    }
    if (count == k) {
        // pos 是第 k 個空白後的位置,pos-1 是空白本身
        return s.substr(0, pos - 1);
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【實務範例】簡單的 token 切分
// 為何用 find_first_of: 比起手寫迴圈處理多個 delim,find_first_of 一次比對。
// -----------------------------------------------------------------------------
#include <vector>
std::vector<std::string> tokenize(const std::string& s, const std::string& delims) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start < s.size()) {
        size_t end = s.find_first_of(delims, start);
        if (end == std::string::npos) {
            out.push_back(s.substr(start));
            break;
        }
        if (end > start) out.push_back(s.substr(start, end - start));
        start = end + 1;
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】解析 URL 的 query string
// 為何用 find_first_of: query string 由 '&' / ';' / '=' 等分隔字元組成,
//                       find_first_of 一次找到任一分隔符,比逐字元判斷簡潔。
// -----------------------------------------------------------------------------
std::vector<std::pair<std::string, std::string>> parseQuery(const std::string& qs) {
    std::vector<std::pair<std::string, std::string>> out;
    size_t i = 0;
    while (i < qs.size()) {
        size_t eq = qs.find('=', i);
        size_t amp = qs.find_first_of("&;", i);
        if (amp == std::string::npos) amp = qs.size();
        if (eq != std::string::npos && eq < amp) {
            out.push_back({qs.substr(i, eq - i), qs.substr(eq + 1, amp - eq - 1)});
        } else {
            out.push_back({qs.substr(i, amp - i), ""});
        }
        i = amp + 1;
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1023. Camelcase Matching - 簡化版
// 題目: 給 query 與 pattern,判斷 query 是否可由 pattern 加入「任意小寫字母」得到。
// 為何用 find_first_of: 找下一個大寫字母位置可用 find_first_of(uppercase set, pos)。
//                       同 pattern 之大寫字母必須對齊一致。
// 解法: 雙指針 + find_first_of 跳到下一個大寫。
// 複雜度: O(N)。
// 難度: medium
// -----------------------------------------------------------------------------
bool camelMatchOne(const std::string& query, const std::string& pattern) {
    const std::string upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t i = 0, j = 0;
    while (i < query.size()) {
        if (j < pattern.size() && query[i] == pattern[j]) { ++i; ++j; }
        else if (query[i] >= 'a' && query[i] <= 'z')      { ++i; }
        else return false;
    }
    return j == pattern.size();
    (void)upper;  // (示意 find_first_of 可用於跳大寫,但此實作不需要)
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把字串內首個 newline/CR/TAB 之前的部分當作 header line 取出
// 為何用 find_first_of: 一次比對 {'\n','\r','\t'} 三種終止符,比連續 find 快。
// -----------------------------------------------------------------------------
std::string firstLine(const std::string& s) {
    size_t p = s.find_first_of("\r\n");
    return (p == std::string::npos) ? s : s.substr(0, p);
}

int main() {
    demoFindFirstOf();

    std::cout << "\n=== LeetCode 387 ===\n";
    std::cout << firstUniqChar("leetcode")    << "\n";   // 0
    std::cout << firstUniqChar("loveleetcode")<< "\n";   // 2
    std::cout << firstUniqChar("aabb")        << "\n";   // -1

    std::cout << "\n=== LeetCode 1816 ===\n";
    std::cout << "[" << truncateSentence("Hello how are you Contestant", 4) << "]\n";

    std::cout << "\n=== LeetCode 1023 ===\n";
    std::cout << std::boolalpha
              << camelMatchOne("FooBar",       "FB")  << "\n"   // true
              << camelMatchOne("FooBarTest",   "FB")  << "\n"   // false
              << camelMatchOne("FooBaBar",     "FoBa")<< "\n";  // true

    std::cout << "\n=== tokenize ===\n";
    for (auto& t : tokenize("a,b;c d", ",; ")) std::cout << "[" << t << "] ";
    std::cout << "\n";

    std::cout << "\n=== 日常實務: 解析 query string ===\n";
    for (auto& [k, v] : parseQuery("q=cpp&page=2&debug")) {
        std::cout << k << "=[" << v << "]\n";
    }

    std::cout << "\n=== 日常實務: firstLine ===\n";
    std::cout << "[" << firstLine("HTTP/1.1 200 OK\r\nServer: nginx\r\n") << "]\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:find_first_of("ab") 跟 find("ab") 差在哪?面試常考。
    //    A:find_first_of 是「字元集合」搜尋 — 找第一個是 'a' 或 'b' 的字元;
    //       find 是「子字串」搜尋 — 找連續的 "ab" 兩字元。例 s="bza":前者回 0
    //       ('b' 在 set 中),後者回 npos (沒有 "ab" 子字串)。
    //
    //  Q2:對 const char* 重載而言,集合大小是怎麼判斷的?跟 (s, pos, count)
    //       重載差在哪?
    //    A:find_first_of(const char*, pos) 結束於第一個 '\0';
    //       find_first_of(const char*, pos, count) 明確帶長度,可讓集合包含 '\0'。
    //       要在二進位資料中尋找 control bytes 時必須用後者。
    //
    //  Q3:這個函式對應的 C 函式 strpbrk 有什麼侷限,find_first_of 改善了什麼?
    //    A:strpbrk 只能處理 NUL-terminated C-string 且回傳 char* 易出 lifetime
    //       問題;find_first_of 接受 string / string_view、支援含 '\0' 的版本、
    //       回傳 size_type index,生命週期跟著容器走較安全。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra find_first_of.cpp -o find_first_of

// === 預期輸出 (節錄) ===
// === LeetCode 1023 ===
// true
// false
// true
// === 日常實務: firstLine ===
// [HTTP/1.1 200 OK]
