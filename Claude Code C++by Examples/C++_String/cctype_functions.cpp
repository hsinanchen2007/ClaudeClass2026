// =============================================================================
// 檔名: cctype_functions.cpp
// 主題: <cctype> 字元分類與大小寫轉換函式
//   isalpha / isdigit / isalnum / isspace / isupper / islower /
//   isxdigit / ispunct / iscntrl / isprint / isgraph /
//   tolower / toupper
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/byte
//   cplusplus.com: https://cplusplus.com/reference/cctype/
// =============================================================================
//
// 【家族資訊 Information】
//   全部位於 <cctype> (C 對應 <ctype.h>);std:: namespace 與 :: 皆可使用。
//
//   分類函式 (回傳非 0 表「是」、0 表「否」):
//     int isalpha(int ch);        // 字母 a-zA-Z
//     int isdigit(int ch);        // 數字 0-9
//     int isalnum(int ch);        // 字母或數字
//     int isspace(int ch);        // 空白 (空格、tab、CR、LF、VT、FF)
//     int isupper(int ch);        // 大寫字母
//     int islower(int ch);        // 小寫字母
//     int isxdigit(int ch);       // 16 進位數字 0-9 a-f A-F
//     int ispunct(int ch);        // 標點符號
//     int iscntrl(int ch);        // 控制字元 (< 0x20 與 0x7F)
//     int isprint(int ch);        // 可列印 (含空格)
//     int isgraph(int ch);        // 可列印且非空格
//
//   轉換函式:
//     int tolower(int ch);        // 大寫 → 小寫;否則原樣回傳
//     int toupper(int ch);        // 小寫 → 大寫;否則原樣回傳
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼參數型別是 int 而非 char?
// ----------------------------------------------------------------------------
// 這是「C 字元函式祖傳設計」最大的雷:函式簽名都是 `int isXxx(int ch)`,
// 而 C/C++ 的 char **可能是 signed**(平台相依!)。如果你直接傳一個
// char 進去:
//
//     char c = '\xC4';            // 非 ASCII (例如中文 Big5、UTF-8 開頭)
//     std::isalpha(c);            // UB! signed char → int 變成負數
//
// 標準合約是:傳入值必須能表示為 unsigned char,**或** EOF (-1)。
// 任何負值且不是 EOF 都是 UB。所以 **永遠** 要這樣寫:
//
//     std::isalpha(static_cast<unsigned char>(c));
//
// 這個 cast 是 C/C++ 字元處理的必修魔咒;沒做的話多數時候在 ASCII
// 文字會「碰巧正確」,但只要遇到非 ASCII 一切爆掉。
//
// (二) Locale 影響
// ----------------------------------------------------------------------------
// 預設 (program 啟動時) 的 locale 是 "C"(等同 ASCII)。如果你呼叫過
// std::setlocale(LC_ALL, "") 或 std::locale::global(...),這些 cctype
// 函式的行為會根據新 locale 變動 —— 例如某些 locale 下,某些字元 (像
// é 在 Latin-1 區段) 會被 isalpha() 視為字母。
//
// 在多執行緒、跨地區的服務上這是「靜默 bug」溫床。安全的做法:
//   - 不要 setlocale,讓 cctype 一直用 "C" locale。
//   - 若需要 locale-aware 字元分類,改用 <locale> 的 std::isalpha 範本
//     版本 (吃一個 std::locale 引數),語意明確。
//
// (三) 對非 ASCII 文字 (UTF-8、Big5、中文) 的限制
// ----------------------------------------------------------------------------
// cctype 只處理「single-byte char」。一個中文字 (UTF-8 占 3 byte) 拆開來
// 沒有任何單一 byte 能被合理地分類。要真正處理 Unicode 字元類型應使用:
//   - ICU (https://icu.unicode.org/)
//   - C++ `<cuchar>` + 自製 codepoint 表
//   - C++23 `<text_encoding>` (有限度)
// 不要用 cctype 對 UTF-8 字元做語意判斷 —— 它只能視為 byte 過濾器。
//
// (四) tolower / toupper 的同樣陷阱
// ----------------------------------------------------------------------------
// tolower / toupper 也吃 int、也只認 ASCII (預設 C locale)。對 'A'..'Z' /
// 'a'..'z' 是正確的,對其他字元原樣回傳。要轉中文、土耳其文 i↔İ、
// 德文 ß ↔ ẞ 等都辦不到 —— 必須走 ICU 或專用函式庫。
//
// (五) 對應的 <cwctype> (寬字元版)
// ----------------------------------------------------------------------------
// <cwctype> 提供 iswalpha、iswdigit、iswspace、towlower、towupper 等對
// wchar_t 的版本。但 wchar_t 在 Windows 是 UTF-16、在 Linux/macOS 是
// UTF-32,跨平台一致性差。現代建議:UTF-8 + ICU,別碰 wchar_t。
//
// (六) 與 std::isalpha (in <locale>) 的差別
// ----------------------------------------------------------------------------
//   #include <cctype>
//   bool b = std::isalpha('A');                  // 走 locale-aware C API
//
//   #include <locale>
//   bool b = std::isalpha('A', std::locale());  // 範本版,明確指定 locale
// 後者語意明確、更難用錯,但代價是 build time / 比較 verbose。
//
// (七) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++03 起 <cctype> 即完整存在 (源自 C89)。
//   C++11 加入 <cuchar> 支援 char16_t / char32_t 編碼轉換。
//   後續標準對 cctype 的內容沒有實質變動。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) `static_cast<unsigned char>(c)` 是字元處理的「結界咒」
//    這個 idiom 在 cctype、char_traits、std::sort 比較器等等都應用上,
//    把 signed char 重新解讀為 unsigned char (0..255),避免 sign extension
//    把 0xC4 (E4) 變成 -60。寫多了就會反射。
//
// 2) 為何不直接 (int)c 不行?
//    (int)c 在 char 是 signed 的平台會 sign-extend,變成負數;靜默 UB。
//    經由 unsigned char 才安全。
//
// 3) tolower / toupper 是「不修改參數」的純函式
//    回傳轉換後的字元;原本參數不變。這跟其他 STL 函式 (例如
//    std::transform) 用法一致 —— 結果要自己接住。
//
// 4) 巨集 vs 函式
//    在 C 標準中,這些「函式」可能由 macro 實作 (用 lookup table 一次定址,
//    更快)。C++ 確認它們也存在「真函式」版本,但實作可能仍是 macro。
//    取它們的位址 (&std::isalpha) 不一定可行 — 用 lambda 包一層更安全:
//      std::transform(s.begin(), s.end(), s.begin(),
//          [](unsigned char c){ return std::tolower(c); });
//
// 5) ASCII 表簡記
//    '0'..'9' = 0x30..0x39,'A'..'Z' = 0x41..0x5A,'a'..'z' = 0x61..0x7A。
//    'A' xor 0x20 = 'a';所以 ASCII 大小寫互換的另一種寫法是
//    c ^= 0x20 (前提是已知是字母)。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. **永遠** 把 char 先 cast 成 unsigned char 再呼叫,避免 UB。
// 2. 預設 "C" locale 等同 ASCII;非 ASCII 字元行為依 locale 而定,
//    最佳實踐是不要切換 locale。
// 3. cctype 只處理 single-byte;UTF-8 多 byte 字元無法用它做語意判斷。
// 4. tolower / toupper 不會就地修改;要自己接回傳值。
// 5. 取地址不一定可行 (可能是 macro);建議包 lambda。
// 6. 對 Unicode 處理需求 → ICU 或專屬函式庫;cctype 不夠。
//
// =============================================================================


/*
補充筆記：std::string::cctype_functions
  - <cctype> 的 isalpha、isdigit、isspace、tolower、toupper 都接受 int，但有效輸入必須是 EOF 或可表示成 unsigned char 的值。
  - 直接把 char 傳給 std::isalpha(c) 在 char 為 signed 且資料 byte 大於 127 時可能形成未定義行為；正確寫法是 static_cast<unsigned char>(c)。
  - 這些函式處理的是 byte/單位元組字元，不理解 UTF-8 code point；中文或 emoji 的多 byte 序列不能用 cctype 判斷字母或大小寫。
  - 預設 "C" locale 下，分類大致以 ASCII 規則為主；若程式呼叫 setlocale，全域 locale 會影響 cctype 結果。
  - tolower/toupper 回傳轉換後的 int，不會就地修改原 char；若要批次轉換字串，通常搭配 std::transform。
  - std::tolower 的回傳值轉回 char 前要確認它不是 EOF；一般處理字串 byte 時可 static_cast<char>(std::tolower(unsignedChar))。
  - <locale> 也有 std::isalpha(ch, locale) 這類 overload，語意和 <cctype> 不同，適合需要明確 locale 的情境。
  - cctype 適合 ASCII command、簡單 token、設定檔 key 等 byte-level 處理；真正自然語言文字應使用 Unicode-aware library。
*/

#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

void demoClassify() {
    std::cout << "=== 字元分類函式 ===\n";
    auto report = [](char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        std::cout << "'" << c << "' (0x"
                  << std::hex << static_cast<int>(uc) << std::dec << ")"
                  << " alpha="  << !!std::isalpha(uc)
                  << " digit="  << !!std::isdigit(uc)
                  << " alnum="  << !!std::isalnum(uc)
                  << " space="  << !!std::isspace(uc)
                  << " upper="  << !!std::isupper(uc)
                  << " punct="  << !!std::ispunct(uc) << "\n";
    };
    report('A'); report('a'); report('5'); report(' '); report(','); report('\n');
}

void demoConvert() {
    std::cout << "\n=== tolower / toupper ===\n";
    std::string s = "Hello, World! 123";
    std::string lower(s.size(), '\0');
    std::string upper(s.size(), '\0');

    std::transform(s.begin(), s.end(), lower.begin(),
        [](char c){ return static_cast<char>(
                       std::tolower(static_cast<unsigned char>(c))); });
    std::transform(s.begin(), s.end(), upper.begin(),
        [](char c){ return static_cast<char>(
                       std::toupper(static_cast<unsigned char>(c))); });

    std::cout << "原:    " << s     << "\n";
    std::cout << "lower: " << lower << "\n";
    std::cout << "upper: " << upper << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 1832. Check if the Sentence Is Pangram (Easy)
//
// 題目敘述:
//   給字串 sentence (僅含小寫字母),判斷是否「pangram」(包含 a..z 全部
//   26 個字母至少一次)。
//   範例: "thequickbrownfoxjumpsoverthelazydog" → true
//        "leetcode" → false
//
// 為何用 cctype:
//   題目雖然假設「僅含小寫」,但實務常會放寬條件「混雜其他字元」。
//   遇到非字母要 skip。用 std::isalpha + std::tolower (一定要先 cast
//   到 unsigned char) 是最直接的「字元分類 + 統一轉小寫」工序鏈。
//
// 解題思路:
//   bitmask 標記出現過的字母;檢查最後是否為 (1<<26)-1。
//
// 複雜度: 時間 O(n),空間 O(1)
// -----------------------------------------------------------------------------
bool checkIfPangram(const std::string& sentence) {
    unsigned mask = 0;
    for (char c : sentence) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (!std::isalpha(uc)) continue;
        mask |= 1u << (std::tolower(uc) - 'a');
    }
    return mask == (1u << 26) - 1u;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】設定檔 / .env / INI 解析:trim + 大小寫無視
//
// 為何用 cctype:
//   .env、INI 檔案中,key/value 周圍常有空白、注釋、不同大小寫。
//   實務上需要:
//     - 用 isspace 做雙端 trim
//     - 用 tolower 做 key 大小寫無視比對 (HTTP header 也是)
//   cctype 是這類「ASCII 字元判斷」的標準工具,且夠快、零相依。
//
//   注意:對 UTF-8 內容不適用 — 那是 byte-level 過濾,不是 codepoint
//   語意判斷。
// -----------------------------------------------------------------------------
std::string trim(std::string_view sv) {
    auto isSpace = [](char c){
        return std::isspace(static_cast<unsigned char>(c));
    };
    while (!sv.empty() && isSpace(sv.front())) sv.remove_prefix(1);
    while (!sv.empty() && isSpace(sv.back()))  sv.remove_suffix(1);
    return std::string(sv);
}

bool iequal(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) return false;
    }
    return true;
}

#include <string_view>

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 125. Valid Palindrome (Easy)
// 題目: 忽略非英數字並無視大小寫,判斷字串是否為迴文。
// 為何用 cctype: std::isalnum 判斷有效字元,std::tolower 統一大小寫。
//                兩個都是 cctype 的代表函式,完美結合。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
bool isPalindromeAlnum(const std::string& s) {
    size_t i = 0, j = s.size();
    if (j == 0) return true;
    --j;
    while (i < j) {
        while (i < j && !std::isalnum(static_cast<unsigned char>(s[i]))) ++i;
        while (i < j && !std::isalnum(static_cast<unsigned char>(s[j]))) --j;
        if (std::tolower(static_cast<unsigned char>(s[i])) !=
            std::tolower(static_cast<unsigned char>(s[j]))) return false;
        ++i; if (j == 0) break; --j;
    }
    return true;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】簡易密碼強度檢查
// 為何用 cctype: 用 isupper/islower/isdigit/ispunct 分類字元,判斷是否多樣化。
//                登入服務、註冊頁面常見需求。
// -----------------------------------------------------------------------------
int passwordStrength(const std::string& pw) {
    bool hasU = false, hasL = false, hasD = false, hasP = false;
    for (unsigned char c : pw) {
        if (std::isupper(c)) hasU = true;
        else if (std::islower(c)) hasL = true;
        else if (std::isdigit(c)) hasD = true;
        else if (std::ispunct(c)) hasP = true;
    }
    return (hasU ? 1 : 0) + (hasL ? 1 : 0) + (hasD ? 1 : 0) + (hasP ? 1 : 0);
}

int main() {
    demoClassify();
    demoConvert();

    std::cout << "\n=== LeetCode 1832 ===\n";
    std::cout << std::boolalpha
              << checkIfPangram("thequickbrownfoxjumpsoverthelazydog") << "\n"
              << checkIfPangram("leetcode") << "\n";

    std::cout << "\n=== LeetCode 125 ===\n";
    std::cout << isPalindromeAlnum("A man, a plan, a canal: Panama") << "\n";  // true
    std::cout << isPalindromeAlnum("race a car") << "\n";                       // false

    std::cout << "\n=== 日常實務: trim + iequal ===\n";
    std::cout << "trim(\"  hello  \") = \"" << trim("  hello  ") << "\"\n";
    std::cout << "iequal(\"Content-Type\", \"content-type\") = "
              << iequal("Content-Type", "content-type") << "\n";
    std::cout << "iequal(\"foo\", \"FOOX\") = "
              << iequal("foo", "FOOX") << "\n";

    std::cout << "\n=== 日常實務: passwordStrength ===\n";
    std::cout << passwordStrength("abcdef") << "\n";       // 1 (只有小寫)
    std::cout << passwordStrength("Abc123") << "\n";       // 3 (大寫+小寫+數字)
    std::cout << passwordStrength("Abc!123") << "\n";      // 4 (全部分類)

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼呼叫 std::isalpha(c) 必須先 cast 成 unsigned char?
    //    A：因為 char 在多數平台是 signed,負值傳給 isXxx 是 UB (除了 EOF=-1)。
    //       例如中文字 '\xC4' 會變成負數,直接呼叫立刻 UB。標準合約是「值
    //       必須能表示為 unsigned char」。永遠寫 isalpha((unsigned char)c)
    //       是字元處理的必修魔咒。
    //
    //  Q2：std::tolower 與 std::tolower (locale 版本) 的差別?
    //    A：<cctype> 的 tolower(int) 受全域 locale 影響但只處理單 byte。
    //       <locale> 中 std::tolower<char>(c, loc) 接收明確 locale 物件,
    //       不受全域 setlocale 干擾。在多執行緒服務中為避免「靜默 bug」,
    //       建議鎖定 std::locale::classic() ("C" locale) 或全程用顯式 locale。
    //
    //  Q3：要轉整個 std::string 為小寫,直接 std::transform + tolower 安全嗎?
    //    A：差一點。常見錯誤寫法:transform(s.begin(), s.end(), s.begin(),
    //       ::tolower) ── 直接傳 char 進去 = UB on non-ASCII。正確寫法是
    //       傳 lambda 並 cast: [](char c){ return (char)std::tolower(
    //       (unsigned char)c); }。或改用 ranges::transform 也一樣要 cast。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra cctype_functions.cpp -o cctype_functions

// === 預期輸出 (節錄) ===
// === LeetCode 125 ===
// true
// false
// === 日常實務: passwordStrength ===
// 1
// 3
// 4
