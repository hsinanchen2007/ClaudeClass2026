// =============================================================================
// 檔名: cstring_functions.cpp
// 主題: <cstring> C 風格字串與記憶體函式
//   strlen / strcpy / strncpy / strcat / strncat / strcmp / strncmp /
//   strchr / strrchr / strstr / strtok / strspn / strcspn /
//   memcpy / memmove / memset / memcmp / memchr
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/byte
//   cplusplus.com: https://cplusplus.com/reference/cstring/
// =============================================================================
//
// 【家族資訊 Information】
//   全部位於 <cstring> (C 對應為 <string.h>);std:: namespace 與 ::
//   皆可呼叫 (依平台偶有差異)。它們處理的是「以 '\0' 結尾的 C 風格
//   char 陣列」與「raw byte buffer」,完全不知道 std::string 為何物。
//
//   ── 字串 (NUL-terminated) ──
//   size_t strlen(const char* s);                              // 算長度
//   char*  strcpy(char* dst, const char* src);                 // 複製
//   char*  strncpy(char* dst, const char* src, size_t n);      // 限長複製
//   char*  strcat(char* dst, const char* src);                 // 連接
//   char*  strncat(char* dst, const char* src, size_t n);      // 限長連接
//   int    strcmp(const char* a, const char* b);               // 比較
//   int    strncmp(const char* a, const char* b, size_t n);    // 限長比較
//   char*  strchr(const char* s, int c);                       // 找字元
//   char*  strrchr(const char* s, int c);                      // 由右找
//   char*  strstr(const char* h, const char* n);               // 找子字串
//   char*  strtok(char* s, const char* delim);                 // 分詞 (狀態機)
//   size_t strspn(const char* s, const char* accept);          // 開頭符合長度
//   size_t strcspn(const char* s, const char* reject);         // 開頭不符合長度
//
//   ── 純 byte (不關心 \0) ──
//   void* memcpy(void* dst, const void* src, size_t n);         // 複製 n byte
//   void* memmove(void* dst, const void* src, size_t n);        // 允許重疊
//   void* memset(void* dst, int byte, size_t n);                // 填值
//   int   memcmp(const void* a, const void* b, size_t n);       // 二進位比較
//   void* memchr(const void* s, int c, size_t n);               // 找 byte
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼還要學 C 字串函式?
// ----------------------------------------------------------------------------
// 即便有了 std::string,你仍然會在這些場合接觸 C 字串函式:
//   - 與 C 函式庫接合 (POSIX read/write/getenv、第三方函式庫、kernel)。
//   - 操作純 byte buffer (網路封包、檔案 header、protobuf)。
//   - 嵌入式 / kernel 環境沒有 STL。
//   - 效能極限場景:strlen / memcpy 都 vectorize 為 SIMD 高度最佳化,
//     有時比手寫 std::string 操作更快。
//
// (二) 危險函式 vs 安全函式
// ----------------------------------------------------------------------------
// C 字串 API 的歷史包袱:多數函式對「buffer 不夠大 / 沒 NUL 結尾」毫無
// 防備,buffer overflow 是經典安全漏洞:
//   - **strcpy / strcat** : 不知道 dst 容量 → 容易溢位。盡量改用
//     strncpy / strncat,或更安全的 snprintf。
//   - **strncpy** : 截斷時「不會」自動補 '\0' —— 用後請務必手動加。
//   - **strtok** : 內部用 static 變數,**非執行緒安全**;有 strtok_r
//     (POSIX) / strtok_s (Annex K) 為可重入版本。
//   - **gets** : C11 起被刪除,永遠不要用。
//
// 現代 C 平臺常提供 strlcpy / strlcat (BSD)、snprintf 等更安全替代。
//
// (三) memcpy vs memmove
// ----------------------------------------------------------------------------
//   - memcpy:dst 與 src 「不可」重疊;若重疊行為未定義。但通常更快
//     (SIMD vectorize)。
//   - memmove:dst 與 src 「可以」重疊;標準保證內部會選對方向複製。
// 不知道有沒有重疊就用 memmove;確定沒重疊用 memcpy。
//
// (四) strlen 為何是熱點函式?
// ----------------------------------------------------------------------------
// 任何「以 NUL 結尾」的字串長度只能 O(n) 掃描;若你重複呼叫 strlen 可能
// 隱性帶來 quadratic 複雜度 (例如 for (int i=0; i<strlen(s); ++i))。
// 改寫成 size_t n = strlen(s); for (i=0; i<n; ++i) 可避免。
// 現代 libc 的 strlen 高度向量化,常以 16/32 byte 對齊掃描。
//
// (五) strcmp 的回傳值
// ----------------------------------------------------------------------------
// 回傳值是「< 0 / == 0 / > 0」三態:
//   < 0 表 a < b,> 0 表 a > b,== 0 表相等。
// 不要寫 if (strcmp(a, b) == 1) —— 標準只保證符號;具體值是實作細節。
// (古早教科書常見錯誤,被代代複製。)
//
// (六) strtok 的 thread-safety 大坑
// ----------------------------------------------------------------------------
// strtok 用 static 變數記住「上次切到哪」,所以:
//   - 同一執行緒中嵌套呼叫 (e.g. tokenize 後對每個 token 再 tokenize) 會壞。
//   - 多執行緒同時呼叫會 race。
// 若必須用,改 strtok_r (POSIX,自帶 saveptr 參數) 或 strtok_s (Annex K)。
//
// (七) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++03 起 <cstring> 就完整存在 (從 C89 引進)。
//   C++11 加入更多 type-safe 字元範本 (但 cstring 內容沒變)。
//   C++17 std::string_view 出現後,「在 C++ 內部」用 cstring 的場合
//        大幅減少,主要剩「C API 介面」。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) NUL-terminated 是 C 的「祖傳 sin」
//    NUL 結尾意味著「字串中不能含 NUL」;對二進位資料、JPEG、UTF-16
//    全都是地雷。Pascal 等語言用「length-prefixed」字串 (頭幾個 byte
//    存長度) 完全避開此問題。std::string 內部記長度,正是這套設計。
//
// 2) Annex K (_s 系列) 在 GCC/Clang 沒實作
//    C11 Annex K 提供 strcpy_s、memcpy_s 等加 buffer size 參數的安全版,
//    但 GCC/glibc 並未實作 (爭議:他們認為 Annex K API 設計差)。
//    僅 MSVC、QNX 等少數實作。Linux 寫法仍以 BSD 的 strlcpy / strlcat
//    為主。
//
// 3) memcpy 的「Trivially Copyable」要求
//    在 C++ 中 memcpy 一個 non-trivially-copyable 物件是 UB:
//    例如 memcpy 一個 std::string 可能會把指標複製,但兩個 string
//    指向同一塊 buffer,析構時 double-free。
//    用 std::copy / std::copy_n 對 trivial / non-trivial 都安全。
//
// 4) 與 std::char_traits 的關係
//    std::basic_string 內部呼叫 std::char_traits<char>::copy/compare/find,
//    這些對 char 而言通常是 memcpy / memcmp / memchr 的 wrapper。
//    所以「std::string 與 cstring 的效能上限相同」,差的是介面安全性。
//
// 5) `restrict` 關鍵字
//    C 的 cstring 函式宣告中很多 dst/src 參數都標 restrict,意思是
//    「呼叫時這兩個指標不會 alias」,讓編譯器更激進地優化。
//    違反 restrict 是 UB。memcpy 的 dst/src 是 restrict;memmove 不是。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. strncpy 截斷時不補 '\0';要的話自己 dst[n-1] = 0。
// 2. strcat / strcpy 不檢查 dst 容量,易 overflow;盡量避免。
// 3. memcpy 不允許 alias,須用 memmove。
// 4. strtok 非執行緒安全,要 thread-safe 改 strtok_r。
// 5. strcmp 回傳「符號」而非確切的 ±1;只比較與 0 的關係。
// 6. memcpy 一個 non-trivially-copyable C++ 物件是 UB。
// 7. strlen 在迴圈條件中重複呼叫會引入 O(n²) 複雜度。
//
// =============================================================================

/*
補充筆記：std::string::cstring_functions
  - std::string::cstring_functions 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::cstring_functions 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】cstring (strlen / strcpy / memcpy …)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::string、const char*、std::string_view 三者怎麼選？
//     答：std::string 擁有資料、可修改可增長、保證 null-terminated，
//         要「存起來」（成員變數、回傳值）就用它。
//         const char* 不擁有資料，靠 '\0' 定界，strlen 是 O(n)，無法表達內含 '\0'
//         的資料，也不帶生命週期資訊——只該出現在 C API 邊界。
//         std::string_view（C++17）是不擁有的「指標 + 長度」，O(1) 建構與 substr，
//         但不保證 null-terminated，實務上當成 parameter-only type 使用。
//     追問：那什麼時候還是該用 const std::string&？→ 函式內部需要 c_str() 呼叫 C API 時。
//
// ⚠️ 陷阱. 為什麼 string_view 不能直接傳給 printf("%s") 或其他 C API？
//     答：因為 string_view 不保證結尾有 '\0'。它常常只是指向某個更大 buffer 的
//         一小段（例如從中間 substr 出來的），C API 會從起點一路讀到下一個 '\0'，
//         讀過頭就是 buffer overread。要先轉成 std::string 才能安全 c_str()。
//     為什麼會錯：view 印出來「看起來就是那段字」，讓人以為它和 const char* 可互換；
//         但兩者的定界方式根本不同——一個靠長度，一個靠 '\0'。
//
// 🔥 Q2. std::string 內部可以存 '\0' 嗎？傳給 C API 會怎樣？
//     答：可以。std::string 另外存了 size()，不靠 '\0' 判長度，
//         所以 "a\0b" 這種內容 size() 正確回 3。
//         但一旦交給 strlen / strcpy 這類 C 函式，它們只認 '\0'，資料會在第一個
//         '\0' 被截斷——這是「C++ 字串跨到 C 邊界」的典型資料遺失點。
//     追問：那 c_str() 和 data() 差在哪？→ C++11 起兩者完全等價，
//           都保證 data()[size()] == '\0'（C++98 時代 data() 才不保證）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>

void demoStringFamily() {
    std::cout << "=== <cstring> 字串函式 ===\n";

    char buf[64] = {};

    // strcpy / strcat
    std::strcpy(buf, "Hello");
    std::strcat(buf, ", World");
    std::cout << "after strcpy+strcat: " << buf
              << " (strlen=" << std::strlen(buf) << ")\n";

    // strncpy 不補 \0 的陷阱
    char small[6] = {};
    std::strncpy(small, "abcdef", 6);   // 寫滿 6 byte,沒位置補 \0
    small[5] = '\0';                     // 我們手動補
    std::cout << "strncpy + 手動補 \\0: " << small << "\n";

    // strcmp
    std::cout << "strcmp(\"abc\",\"abd\") sign = "
              << (std::strcmp("abc","abd") < 0 ? "<0" :
                  std::strcmp("abc","abd") == 0 ? "==0" : ">0") << "\n";

    // strchr / strrchr / strstr
    const char* path = "/home/hsinan/file.cpp";
    const char* lastSlash = std::strrchr(path, '/');
    const char* dot       = std::strchr(path, '.');
    const char* sub       = std::strstr(path, "hsinan");
    std::cout << "lastSlash → \"" << (lastSlash ? lastSlash : "") << "\"\n";
    std::cout << "dot       → \"" << (dot ? dot : "") << "\"\n";
    std::cout << "sub       → \"" << (sub ? sub : "") << "\"\n";
}

void demoMemFamily() {
    std::cout << "\n=== <cstring> 記憶體函式 ===\n";

    char a[16] = {};
    std::memset(a, 'X', 8);
    a[8] = '\0';
    std::cout << "memset 'X' x8 + NUL : " << a << "\n";

    // memcpy:src/dst 不重疊
    char b[16] = "ABCDEFGH";
    char c[16] = {};
    std::memcpy(c, b, 8);
    c[8] = '\0';
    std::cout << "memcpy: " << c << "\n";

    // memmove:重疊安全
    char d[] = "ABCDEFGH";
    std::memmove(d + 2, d, 6);   // 重疊 → 必用 memmove
    d[8] = '\0';
    std::cout << "memmove (overlap): " << d << "\n";  // ABABCDEF

    // memcmp:二進位比較 (含 \0 也比)
    char x[3] = {'A','\0','B'};
    char y[3] = {'A','\0','C'};
    std::cout << "memcmp(\"A\\0B\",\"A\\0C\",3) sign = "
              << (std::memcmp(x, y, 3) < 0 ? "<0" : ">0") << "\n";

    // memchr:找 byte
    auto p = std::memchr(b, 'D', sizeof(b));
    std::cout << "memchr 'D' offset = "
              << (p ? static_cast<const char*>(p) - b : -1) << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 28. Find the Index of the First Occurrence in a
//                  String (Easy) — 用 strstr 解
//
// 題目敘述:
//   給字串 haystack 與 needle,回傳 needle 第一次出現在 haystack 的
//   index;不存在則回傳 -1。needle 為空字串時回 0。
//   範例: haystack="sadbutsad", needle="sad" → 0
//        haystack="leetcode",  needle="leeto" → -1
//
// 為何用 strstr:
//   C 函式 strstr 內部通常實作為 Two-Way 或 Boyer-Moore 等高效演算法,
//   是「在小規模字串中找子字串」的工業級基準。題目剛好直接對應
//   strstr 的回傳語意。
//
// 注意:這示範的是「用 cstring API 解 LeetCode」,實務 C++ 同樣場景
//      建議用 std::string::find 或 std::string_view::find。
//
// 複雜度: 時間 O(n+m) (libc 通常如此),空間 O(1)
// -----------------------------------------------------------------------------
int strStr(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return 0;
    const char* p = std::strstr(haystack.c_str(), needle.c_str());
    return p ? static_cast<int>(p - haystack.c_str()) : -1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】解析環境變數 / 取出 KEY=VALUE 的 VALUE
//
// 為何用 cstring:
//   getenv() 回傳 const char*,且 envp 在 main 簽名中也是 char**。
//   實務常需要把 "KEY=VALUE" 切成 KEY 與 VALUE。雖然 std::string_view
//   也能做,但若你想「不額外配置任何 std::string」(例如在 dlopen 環境
//   或 init 早期 STL 還未就緒),C 函式是唯一選項。
// -----------------------------------------------------------------------------
const char* envValueAfterEqual(const char* keyEqValue) {
    // 找到 '=' 後的位置;沒有的話回 nullptr
    const char* eq = std::strchr(keyEqValue, '=');
    return eq ? eq + 1 : nullptr;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 58. Length of Last Word
// 題目: 給字串含空白,回傳最後一個單字的長度。
// 為何用 cstring: 用 std::strlen + 倒向掃描,完全在 const char* 上工作,
//                 不配置 std::string。展示 C 風格高效實作。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int lengthOfLastWordC(const char* s) {
    int n = static_cast<int>(std::strlen(s));
    int i = n - 1;
    while (i >= 0 && s[i] == ' ') --i;     // 跳過尾端空白
    int end = i;
    while (i >= 0 && s[i] != ' ') --i;     // 數最後一個單字
    return end - i;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】比較副檔名 (case-insensitive)
// 為何用 cstring: 用 strrchr 找最後 '.';再用 strcasecmp / 自寫小寫比對。
// 場景: 上傳檔案 / 圖片處理時判斷格式。
// -----------------------------------------------------------------------------
bool hasExtension(const char* filename, const char* ext) {
    const char* dot = std::strrchr(filename, '.');
    if (!dot) return false;
    // 逐字 case-insensitive 比對
    const char* a = dot + 1;
    const char* b = ext;
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? (*a + 32) : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (*b + 32) : *b;
        if (ca != cb) return false;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

int main() {
    demoStringFamily();
    demoMemFamily();

    std::cout << "\n=== LeetCode 28 ===\n";
    std::cout << strStr("sadbutsad", "sad")   << "\n";   // 0
    std::cout << strStr("leetcode",  "leeto") << "\n";   // -1
    std::cout << strStr("hello",     "")      << "\n";   // 0

    std::cout << "\n=== LeetCode 58 ===\n";
    std::cout << lengthOfLastWordC("Hello World")         << "\n";   // 5
    std::cout << lengthOfLastWordC("   fly me   to   the moon  ") << "\n";  // 4

    std::cout << "\n=== 日常實務: env 解析 ===\n";
    const char* line = "PATH=/usr/local/bin:/usr/bin";
    const char* val  = envValueAfterEqual(line);
    std::cout << "VALUE = " << (val ? val : "(none)") << "\n";

    std::cout << "\n=== 日常實務: hasExtension ===\n";
    std::cout << std::boolalpha
              << hasExtension("photo.PNG", "png") << "\n"   // true
              << hasExtension("report.pdf", "doc") << "\n"  // false
              << hasExtension("Makefile",   "mk")  << "\n"; // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：strncpy 真的比 strcpy 安全嗎?它有什麼陷阱?
    //    A：只是「比較不會 overrun」而已,並不真正安全。陷阱:(1) 來源 ≥ n
    //       時不會補 '\0',結果不是合法 C-string;(2) 來源 < n 時會補滿 '\0'
    //       到 n,浪費 CPU;(3) 函式名暗示複製字串,實則是「最多 n byte」。
    //       現代寫法用 snprintf(dst,n,"%s",src) 或直接 memcpy + 手動寫 '\0'
    //       才安全。
    //
    //  Q2：memcpy 與 memmove 在效能與行為上的差別?
    //    A：memcpy 假設來源與目的「不重疊」,實作可全速 SIMD;若實際重疊則
    //       是 UB (常見 bug)。memmove 處理重疊情況 (內部判斷方向決定從前
    //       或從後拷貝),稍慢但正確。不確定是否重疊時必選 memmove,例如
    //       std::string 內部插入時。
    //
    //  Q3：strtok 為什麼不執行緒安全?有什麼替代?
    //    A：strtok 內部用 static 變數記住「下一次從哪繼續」,多執行緒同時
    //       呼叫會互相覆寫狀態。POSIX 提供 strtok_r (reentrant 版,呼叫者
    //       自帶 saveptr),Annex K 提供 strtok_s (相同概念,Windows 上常見)。
    //       現代 C++ 應改用 std::string_view + find 或 std::ranges::split。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra cstring_functions.cpp -o cstring_functions

// === 預期輸出 (節錄) ===
// === LeetCode 58 ===
// 5
// 4
// === 日常實務: hasExtension ===
// true
// false
// false
