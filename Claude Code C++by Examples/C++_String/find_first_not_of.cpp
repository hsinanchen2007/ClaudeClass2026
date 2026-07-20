// =============================================================================
// 檔名: find_first_not_of.cpp
// 主題: std::string::find_first_not_of (找出「不在目標字元集」的第一個位置)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/find_first_not_of
//   - https://cplusplus.com/reference/string/string/find_first_not_of/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type find_first_not_of(const string& str, size_type pos = 0)              const noexcept; // (1)
//   size_type find_first_not_of(const char*   s,   size_type pos = 0)              const;          // (2)
//   size_type find_first_not_of(const char*   s,   size_type pos, size_type count) const;          // (3)
//   size_type find_first_not_of(char          ch,  size_type pos = 0)              const noexcept; // (4)
//   size_type find_first_not_of(string_view   sv,  size_type pos = 0)              const noexcept; // (5) C++17
//
// 回傳: 第一個位置 i (i ≥ pos),使得 s[i] 不屬於 str 字元集合;若全部屬於回 npos。
//
// 【詳細解釋 Explanation】
// find_first_not_of 是 find_first_of 的「補集版本」 — 我們找的是「不在 str
// 集合中」的第一個字元位置。直觀理解:「跳過所有屬於 str 的字元,找第一個不
// 是的位置」。這是字串解析中最有用的工具之一,因為「跳過空白」、「找 token
// 邊界」、「strip 前綴/後綴」等需求,本質上都是「找第一個不屬於某集合的字元」。
//
// 【1. 與 find_first_of 的對偶】
//   find_first_of("aeiou")        找第一個母音
//   find_first_not_of("aeiou")    找第一個非母音(子音 / 標點 / 空白等)
// 範例:s = "aeIO!xyz"
//   s.find_first_of("aeiou")      → 0   ('a' 是母音)
//   s.find_first_not_of("aeiou")  → 2   ('I' 大寫不在集合裡)
// 例:s = "   hello"
//   s.find_first_of(" \t")        → 0   (第一個是空格)
//   s.find_first_not_of(" \t")    → 3   (跳過所有空格,第一個非空白)
//
// 【2. trim 的標準寫法】
// C++ STL 沒有內建 trim 函式,但 find_first_not_of + find_last_not_of 就是
// 答案,寫起來只要 3 行:
//   std::string trim(const std::string& s) {
//       const char* ws = " \t\n\r\f\v";
//       size_t a = s.find_first_not_of(ws);
//       if (a == std::string::npos) return "";  // 整串都是空白
//       size_t b = s.find_last_not_of(ws);
//       return s.substr(a, b - a + 1);
//   }
// 這個寫法的優點:
//   * 一次掃描兩端,O(N) 完成
//   * 不需要寫 isspace 迴圈,意圖明確
//   * 集合可任意自訂,要去除引號 / 0 / 標點都很容易
//
// 【3. 演算法與效能】
// naive 實作:
//   for i in [pos, N):
//       found = false
//       for c in str:
//           if s[i] == c: found = true; break
//       if not found: return i
//   return npos
// 複雜度 O(N * M)。對 char 集合可用 bool[256] 預處理為 O(N + M)。
// libstdc++ / libc++ 對 const char* 重載通常都做這個優化。
//
// 【4. 各重載細節】
// 與 find_first_of 完全對應,只是判斷條件相反。
//
// 【5. 邊界規則(常被遺忘)】
//   * str 為空字串:永遠回傳 pos(「集合是空集合,任何字元都不屬於它,所以
//     第一個位置就是 pos 自己」)。注意這跟 find_first_of("") → npos 相反。
//   * pos > size():回傳 npos。
//   * pos == size():回傳 npos(沒有字元可比)。
//   * 在空字串上:回傳 npos。
//
// 【6. 常見應用模式】
//   * trim_left:  s.substr(s.find_first_not_of(" \t"))
//   * trim_right: s.substr(0, s.find_last_not_of(" \t") + 1)
//   * 跳過某些前綴字元(如 BOM、引號、零)
//   * 找 token 邊界:跳過分隔符後找下一個內容
//   * 數字解析:找第一個非數字位置作為「整數結束」
//   * 跳過註解標記:s.find_first_not_of("/*# ")
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼 trim 要用 find_*_not_of 而不是手寫 while
//   手寫:
//     while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
//     ⚠️ 那個 static_cast<unsigned char> 不是可有可無的裝飾:cctype 系列的前置條件是
//        「引數必須可表示為 unsigned char 或等於 EOF」。plain char 在 x86-64 Linux 上
//        是有號的,傳入 0x80~0xFF(例如 UTF-8 中文的任何一個 byte)會變成負值 → UB。
//   STL:
//     i = s.find_first_not_of(" \t\n\r");
//   STL 版本:
//     * 一行表達意圖,無需迴圈變數
//     * 集合可隨意調整(只要去除空格?去除空格與 tab?去除全部空白字元?)
//     * 內部有 char_traits 優化、可能 SIMD 加速
//   手寫版本只有在「條件需要動態判斷」時才有優勢(例如要呼叫 isalpha + isdigit)。
//
// (B) trim 的標準寫法(完整版,C++17 string_view 版本)
//   std::string_view trim(std::string_view sv,
//                         std::string_view ws = " \t\n\r\f\v") {
//       size_t a = sv.find_first_not_of(ws);
//       if (a == std::string_view::npos) return {};
//       size_t b = sv.find_last_not_of(ws);
//       return sv.substr(a, b - a + 1);
//   }
//   string_view 版本不需任何複製,效能極佳。
//
// (C) 與 std::isspace / std::isalnum 的關係
//   這些 cctype 函式是「謂詞」(predicate),適合一次只判斷一個字元。
//   find_first_not_of 是「索引搜尋」,直接給你下一個感興趣位置。
//   兩者互補:
//     find_first_not_of(" \t") + 後續 isalpha 判斷 → 高效且清晰
//
// (D) 與 ranges (C++20) 的對照
//   C++20 後可用 ranges::find_if + lambda:
//     auto it = std::ranges::find_if(s, [](char c){
//         return !std::isspace(static_cast<unsigned char>(c)); });   // 同樣要轉 unsigned char
//   但對「字元集合」這個用例,find_first_not_of 仍然簡潔得多。
//
// 【注意事項 Pay Attention】
// 1. 與 find_first_of 一樣,空 str 行為不同:
//      find_first_of("")     → npos
//      find_first_not_of("") → pos(因為「集合空,任何字元都不屬於它」)
// 2. 配合 find_last_not_of 是最簡潔的 trim 工具。
// 3. trim 後別忘記檢查「全部是空白」的情況(start == npos)。
// 4. 解析數字 / 標識字時,find_first_not_of("0-9...") 找邊界比手寫更安全。
// 5. 對 UTF-8 多 byte 字元無效 — 它是 byte-level 比對,「中文字元」會被拆開。
//
// =============================================================================

/*
補充筆記：std::string::find_first_not_of
  - std::string::find_first_not_of 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::find_first_not_of 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::find_first_not_of
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. find_first_not_of 的典型用途是什麼?
//     答：trim 前導空白。s.find_first_not_of(" \t\n") 回傳第一個非空白字元的位置,
//         再 s.substr(pos) 就得到去掉前導空白的字串。
//     追問：整串都是空白會怎樣?→ 回傳 npos,所以一定要先判斷再 substr。
//
// 🔥 Q2. 為什麼一定要先檢查 npos?
//     答：當每個字元都屬於該集合(或字串為空)時回傳 std::string::npos
//         (= static_cast<size_type>(-1),64-bit 上為 18446744073709551615)。
//         直接把它拿去 s.substr(npos) 會拋 std::out_of_range(pos > size())。
//
// ⚠️ 陷阱. 它是「補集」搜尋,不是「找不到某個子字串」。
//     答：find_first_not_of("abc") 找的是第一個「不是 a、也不是 b、也不是 c」的字元,
//         而不是「第一個不等於子字串 abc 的位置」。
//     為什麼會錯：把參數讀成子字串(像 find 那樣),它其實是字元集合。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

void demoFFNO() {
    std::string s = "   Hello, World!";
    auto p = s.find_first_not_of(' ');
    std::cout << "first non-space at " << p << "\n";   // 3

    auto p2 = s.find_first_not_of(" Heo,Wrld!");
    std::cout << "first not-in-set at " << p2 << "\n"; // npos
}

// -----------------------------------------------------------------------------
// 【實務範例】最簡潔的 trim
// 為何用 find_first/last_not_of: 兩呼叫 + substr 即完成。
// -----------------------------------------------------------------------------
std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 14. Longest Common Prefix
// 題目: 給字串陣列,回傳所有字串的最長公共前綴。
// 為何用 find_first_not_of: 雖然官方解法常用「逐字元比對」,我們示範另一種
//                            視角 — 把第一個字串當基準,依序檢查其他字串是否以
//                            該前綴開頭(用 find != 0 判斷),不符就縮短前綴。
//                            這個解法清楚展示了 find 與 trim 思維的搭配。
// -----------------------------------------------------------------------------
#include <vector>
std::string longestCommonPrefix(const std::vector<std::string>& strs) {
    if (strs.empty()) return "";
    std::string prefix = strs[0];
    for (size_t i = 1; i < strs.size(); ++i) {
        // 把 prefix 一直縮短,直到 strs[i] 以 prefix 開頭(find == 0)
        while (!prefix.empty() && strs[i].find(prefix) != 0) {
            prefix.pop_back();
        }
        if (prefix.empty()) return "";
    }
    return prefix;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】字串 trim (LeetCode 1816 / 1119 等都會用到的工具函式)
// 題目: 移除字串前後空白字元。
// 為何用 find_first_not_of + find_last_not_of: 這是 STL 中 trim 的標準寫法,
//                                                 兩次 O(N) 掃描即完成。
// -----------------------------------------------------------------------------
std::string trimStandard(const std::string& s) {
    static const char* ws = " \t\n\r\f\v";
    size_t a = s.find_first_not_of(ws);
    if (a == std::string::npos) return "";   // 整串都是空白
    size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 3】LeetCode 290 風格 — 用 find_first_not_of 切分單字
// 雖然完整題還需 hash map,我們示範用 find_first_not_of 切分單字。
// -----------------------------------------------------------------------------
#include <vector>
std::vector<std::string> splitWords(const std::string& s) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size()) {
        i = s.find_first_not_of(' ', i);
        if (i == std::string::npos) break;
        size_t j = s.find(' ', i);
        if (j == std::string::npos) j = s.size();
        out.push_back(s.substr(i, j - i));
        i = j;
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】解析 INI / .env 設定檔: KEY = value
// 為何用 find_first_not_of: 跳過 KEY 之後 / value 之前的空白,
//                            比手寫 while 迴圈簡潔很多。
// -----------------------------------------------------------------------------
std::pair<std::string, std::string> parseIniLine(const std::string& line) {
    size_t eq = line.find('=');
    if (eq == std::string::npos) return {"", ""};
    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);

    // trim key
    size_t kStart = key.find_first_not_of(" \t");
    size_t kEnd   = key.find_last_not_of(" \t");
    key = (kStart == std::string::npos) ? "" : key.substr(kStart, kEnd - kStart + 1);

    // trim value (還可處理引號)
    size_t vStart = val.find_first_not_of(" \t");
    size_t vEnd   = val.find_last_not_of(" \t");
    val = (vStart == std::string::npos) ? "" : val.substr(vStart, vEnd - vStart + 1);

    return {key, val};
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1455. Check If a Word Occurs As a Prefix of Any Word
// 題目: 給一句話與 searchWord,回傳第一個以 searchWord 為前綴的單字的 1-based index,
//       不存在回 -1。
// 為何用 find_first_not_of: 跳過開頭空白,再用 " " 切詞時找下一個非空白位置。
//                            這是「token 開頭定位」的典型應用。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int isPrefixOfWord(const std::string& sentence, const std::string& searchWord) {
    int idx = 1;
    size_t i = sentence.find_first_not_of(' ', 0);
    while (i != std::string::npos) {
        if (sentence.compare(i, searchWord.size(), searchWord) == 0) return idx;
        size_t j = sentence.find(' ', i);
        if (j == std::string::npos) break;
        i = sentence.find_first_not_of(' ', j);
        ++idx;
    }
    return -1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】檢查字串是否「全為數字」
// 為何用 find_first_not_of: 若回傳 npos,代表沒有「非數字」字元 → 全是數字。
//                           比逐字 isdigit 簡潔。場景:輸入驗證、port、ID 確認。
// -----------------------------------------------------------------------------
bool isAllDigits(const std::string& s) {
    return !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
}

int main() {
    demoFFNO();
    std::cout << "\n=== trim ===\n";
    std::cout << "[" << trim("   hello   ") << "]\n";

    std::cout << "\n=== LeetCode 14 ===\n";
    std::cout << "[" << longestCommonPrefix({"flower", "flow", "flight"}) << "]\n";  // "fl"
    std::cout << "[" << longestCommonPrefix({"dog", "racecar", "car"})    << "]\n";  // ""

    std::cout << "\n=== LeetCode 1455 ===\n";
    std::cout << isPrefixOfWord("i love eating burger", "burg") << "\n";   // 4
    std::cout << isPrefixOfWord("this problem is great", "pro") << "\n";   // 2

    std::cout << "\n=== trim (標準寫法) ===\n";
    std::cout << "[" << trimStandard("  \t hello world\n ") << "]\n";  // "hello world"
    std::cout << "[" << trimStandard("     ")                << "]\n";  // ""

    std::cout << "\n=== splitWords ===\n";
    for (auto& w : splitWords("  dog  cat  fish  "))
        std::cout << "[" << w << "]\n";

    std::cout << "\n=== 日常實務: 解析 INI ===\n";
    auto [k1, v1] = parseIniLine("  port  =  8080  ");
    auto [k2, v2] = parseIniLine("name=Alice");
    std::cout << "[" << k1 << "]=[" << v1 << "]\n";
    std::cout << "[" << k2 << "]=[" << v2 << "]\n";

    std::cout << "\n=== 日常實務: isAllDigits ===\n";
    std::cout << std::boolalpha
              << isAllDigits("12345") << "\n"    // true
              << isAllDigits("12a45") << "\n"    // false
              << isAllDigits("")      << "\n";   // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:find_first_of("") 與 find_first_not_of("") 對「空集合」的回傳為何相反?
    //    A:find_first_of("") → npos (集合是空,沒有任何字元屬於它);
    //       find_first_not_of("") → pos  (集合是空,任何字元都「不屬於」它,
    //       所以從 pos 開始的第一個字元就符合條件)。是常見的「空集合對偶」陷阱。
    //
    //  Q2:對 UTF-8 中文字串呼叫 find_first_not_of(" ") 安全嗎?
    //    A:技術上沒問題 (只是 byte-level 比對),但若集合含 multi-byte 中文,
    //       它會把中文字「拆開比對」造成誤判 — 例如 "中" 是 3 bytes,集合裡放
    //       "中" 會分別比對其中每個 byte。要 unicode-safe 須用 ICU 或先轉
    //       std::wstring + locale。
    //
    //  Q3:trim 為什麼必須先檢查 find_first_not_of 是否回傳 npos?
    //    A:因為「整串都是空白」時 first 為 npos,後面 substr(npos, ...) 雖不
    //       UB 但會給整串內容回來,結果跟 trim 預期 (回空字串) 相反。標準 trim
    //       寫法都先處理這個 edge case 才繼續算 last。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra find_first_not_of.cpp -o find_first_not_of

// === 預期輸出 (節錄) ===
// === LeetCode 1455 ===
// 4
// 2
// === 日常實務: isAllDigits ===
// true
// false
// false
