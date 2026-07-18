// =============================================================================
// 檔名: string_view.cpp
// 主題: std::string_view (C++17 非擁有式字串視圖)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string_view
//   cplusplus.com: https://cplusplus.com/reference/string_view/string_view/
// =============================================================================
//
// 【類別資訊 Information】
//   template<class CharT, class Traits = std::char_traits<CharT>>
//   class basic_string_view;
//
//   常用別名:
//     using string_view    = basic_string_view<char>;
//     using wstring_view   = basic_string_view<wchar_t>;
//     using u8string_view  = basic_string_view<char8_t>;    // C++20
//     using u16string_view = basic_string_view<char16_t>;
//     using u32string_view = basic_string_view<char32_t>;
//
// 介面與 std::string 高度重疊,但「不擁有」字串資料 —— 內部只持有
// 兩個欄位:const CharT* + size_t length。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 什麼是 string_view?
// ----------------------------------------------------------------------------
// std::string_view 是 C++17 引入的「不持有所有權的字串視圖」(non-owning
// view)。可以理解成「一對 (pointer, length) 的瘦殼」,提供與 std::string
// 幾乎一致的「只讀」介面 (size, find, substr, starts_with, compare 等),
// 但:
//   - 不會配置記憶體
//   - 不會釋放任何記憶體
//   - 不複製字串內容
//
// 它是「指向別處字串資料的窗口」,與 std::span<const char> 是同一精神,
// 專為 char 序列。
//
// (二) 何時該用 string_view?
// ----------------------------------------------------------------------------
// **黃金原則:函式參數想「只讀」字串時,優先選 string_view**。
//
//   void log(std::string_view sv);          // ✓ 推薦寫法
//   void log(const std::string& s);          // 老寫法,只能吃 string
//   void log(const char* s);                 // 古早寫法,要 strlen,危險
//
// 用 string_view 的好處:
//   1. 同時接受 std::string、const char*、char[N]、子字串、字面量,
//      不必為每種來源寫 overload。
//   2. 不需要「為了傳參而建臨時 string」(避免一次配置)。
//   3. 子字串 sv.substr(2, 5) 是 O(1) 不複製。
//   4. 比 const std::string& 的程式碼更短、更現代。
//
// (三) string_view 的危險:Dangling View
// ----------------------------------------------------------------------------
// string_view 只記住「pointer + length」,不管理生命週期。最常見地雷:
//
//   1) 視圖指向暫時 (rvalue):
//      std::string_view bad = std::string("hi");   // string 是 rvalue → dangling
//      std::cout << bad;                            // UB
//
//   2) 視圖比原 string 活得久:
//      std::string_view sv;
//      {
//          std::string s = "hello";
//          sv = s;                                  // sv 指向 s 的 buffer
//      }                                            // s 在此銷毀
//      std::cout << sv;                             // dangling
//
//   3) 原 string 被改後再用 view:
//      std::string s = "ab";
//      std::string_view v = s;
//      s.append(1000, 'X');                         // 可能 reallocate
//      std::cout << v;                              // dangling (buffer 已搬家)
//
//   4) 從返回 string 的函式接 view:
//      auto v = (std::string{"hi"}).substr(0);      // ✗ 仍是 std::string,但若
//                                                   //   你取它的 sv 就 dangling
//
// 規則:**string_view 只應該指向「明確比視圖長壽」的資料**。
// 字面量 "foo" 是靜態儲存期,永遠安全;局部 string 要小心 scope。
//
// (四) string_view 不一定 NUL-terminated
// ----------------------------------------------------------------------------
// std::string 的 c_str() 保證 NUL-terminated;string_view 不保證。
// 例子:
//   std::string s = "hello world";
//   std::string_view sv = std::string_view{s}.substr(0, 5);   // "hello"
//   sv.data();   // 指向 'h',但 sv.data()[5] 是 ' ',不是 '\0'!
//
// 所以**不要把 sv.data() 直接傳給期望 C-string 的 API** (printf("%s", ...)、
// fopen、execve 等)。要這樣做必須先轉成 std::string{sv}.c_str()。
//
// (五) ABI / sizeof / cost
// ----------------------------------------------------------------------------
// sizeof(std::string_view) == 16 bytes (在 64-bit:8 byte ptr + 8 byte size_t)。
// 比 sizeof(std::string) (libstdc++ 為 32 bytes) 小一半。複製是 O(1)。
// 這也是為什麼 string_view 通常 by-value 傳遞 —— 連 const& 都不必。
//
// (六) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++17 起加入 (P0220)。
//   C++20 加上 starts_with、ends_with、constexpr 全面強化。
//   C++23 加上 contains。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) 與 std::span<const char> 的關係
//    string_view 大致等同 span<const char> + 字串專屬 API (find/substr)。
//    span 是 C++20 通用範本;string_view 早 3 年加入且更聚焦字串語意。
//    兩者可互相轉換,各有領域。
//
// 2) string_view 不能 mutate
//    它只持 const CharT*,沒有可寫成員。要寫,改用 std::span<char>
//    或 std::string&。
//
// 3) 為什麼不要寫 const std::string& sv = some_string_view?
//    string_view 不是 string;它沒有 c_str()、沒有所有權。把它強塞回
//    std::string 會走 explicit 建構子並複製一次。如果只是要傳給內部
//    string 介面,不要強轉,改寫一個吃 string_view 的版本。
//
// 4) substr 是 O(1)
//    sv.substr(pos, count) 回傳新 string_view,只調整 ptr + len,不複製。
//    這跟 std::string::substr (O(n) 複製) 大不同。
//
// 5) Constexpr-friendly
//    string_view 全 API 在 C++17 就大多 constexpr,可在 compile time
//    比較字串、做 lookup table 等。對 constexpr 程式設計尤其重要。
//
// 6) 與 std::format 的合作
//    std::format("{}", sv) 直接列印 string_view,不需轉 string,避免配置。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 永遠不要把 string_view 指向暫時物件;不要存入比 source 長壽的物件。
// 2. string_view::data() 不保證 NUL-terminated;不可直接傳 C API。
// 3. by-value 傳遞 (16 byte) 是慣例,別寫成 const string_view&。
// 4. 不能「就地修改」字串內容;那要 string& 或 span<char>。
// 5. 雖然 sizeof(string) > sizeof(string_view),但 string 有 SSO,
//    短字串複製不一定比 string_view 慢;profile 為準。
// 6. 對 std::string 與 std::string_view 同一內容,標準保證 hash 相等,
//    可用於 transparent lookup (見 hash.cpp)。
//
// =============================================================================

/*
補充筆記：std::string_view
  - string_view 不擁有字串，只保存指標與長度；來源資料死亡後 view 立刻變成 dangling。
  - 它不保證 null-terminated，因此需要 C API 時不能直接當成 c_str。
  - 適合函式參數接受字串字面量、std::string、子字串切片；不適合長期保存外部字串內容。
  - std::string_view 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <cctype>

void demoBasic() {
    std::cout << "=== 建構與基本操作 ===\n";

    // 各種建構來源
    std::string_view sv1 = "hello";                // 從字面量
    std::string s = "world!";
    std::string_view sv2 = s;                       // 從 std::string (隱式)
    std::string_view sv3{s.data() + 0, 3};         // 從 ptr + len → "wor"

    std::cout << "sv1 = " << sv1 << " size=" << sv1.size() << "\n";
    std::cout << "sv2 = " << sv2 << "\n";
    std::cout << "sv3 = " << sv3 << "\n";

    // O(1) substr
    std::cout << "sv1.substr(1,3) = " << sv1.substr(1, 3) << "\n";  // "ell"

    // 比較
    std::cout << std::boolalpha
              << "sv1 == \"hello\": " << (sv1 == "hello") << "\n"
              << "sv1 < sv2:      " << (sv1 < sv2) << "\n";   // h < w → true

    // C++20: starts_with / ends_with
    std::cout << "sv1.starts_with(\"he\") = " << sv1.starts_with("he") << "\n";

    // remove_prefix / remove_suffix:就地調整視窗
    std::string_view sv4 = "  trim me  ";
    while (!sv4.empty() && std::isspace(static_cast<unsigned char>(sv4.front())))
        sv4.remove_prefix(1);
    while (!sv4.empty() && std::isspace(static_cast<unsigned char>(sv4.back())))
        sv4.remove_suffix(1);
    std::cout << "trimmed: \"" << sv4 << "\"\n";
}

void demoDangling() {
    std::cout << "\n=== Dangling 警告示範 (此處不真的存取) ===\n";

    // 危險案例 1:rvalue
    // std::string_view bad = std::string("hi");   // 註解掉,這就是 dangling
    std::cout << "(rvalue dangling 案例已註解;切勿在實務寫)\n";

    // 危險案例 2:scope
    std::string_view leaked;
    {
        std::string local = "local";
        leaked = local;
        std::cout << "in-scope: " << leaked << "\n";
    }
    // leaked 此處 dangling — 不存取
    std::cout << "(out-of-scope 後不可讀 leaked)\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 58. Length of Last Word (Easy)
//
// 題目敘述:
//   給字串 s,回傳其中「最後一個單字」的長度。單字以空白分隔。
//   範例: "Hello World"        → 5
//        "   fly me   to   "  → 2
//        "luffy is still joyboy" → 6
//
// 為何用 string_view:
//   只是「掃描末尾、計數」,完全不需要造任何新字串;直接以 view 處理
//   是最不浪費的寫法。即便題目原本給的是 std::string,介面寫成 sv
//   仍然能 implicit 轉接,沒有任何成本。
//
// 解題思路:
//   從尾端跳過空白,再從新末端往前數非空白。
//
// 複雜度: 時間 O(n),空間 O(1)
// -----------------------------------------------------------------------------
int lengthOfLastWord(std::string_view s) {
    while (!s.empty() && s.back() == ' ') s.remove_suffix(1);
    int len = 0;
    for (auto it = s.rbegin(); it != s.rend() && *it != ' '; ++it) ++len;
    return len;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】零配置 split (把字串切成 tokens 但不複製)
//
// 為何用 string_view:
//   解析 CSV / Authorization header / log line 時,大量 substring 是
//   常見操作。傳統 std::string::substr 每次複製,N 個 token 就 N 次配置;
//   改用 string_view::substr 全程零配置 —— 對小字串 token (路由解析、
//   query string 解析) 性能可有一個量級的差異。
//
//   實作中,vector<string_view> 持有的全部 view 都指向同一個 source
//   string,呼叫端必須保證 source 在使用 view 期間活著。
// -----------------------------------------------------------------------------
std::vector<std::string_view> splitView(std::string_view s, char delim) {
    std::vector<std::string_view> out;
    size_t start = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delim) {
            out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    out.push_back(s.substr(start));
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page. Subsequence
// 題目: LeetCode 392. Is Subsequence
// 判斷 s 是否為 t 的子序列。
// 為何用 string_view: 兩個輸入都用 view,呼叫端 string / cstr / substr 都可零拷貝。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
bool isSubsequenceView(std::string_view s, std::string_view t) {
    size_t i = 0;
    for (char c : t) {
        if (i < s.size() && s[i] == c) ++i;
    }
    return i == s.size();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從 SQL 查詢字串中取出第一個 table 名稱 (簡化)
// 為何用 string_view: 解析時零配置,只回傳 view 給上層,避免重複建 string。
// -----------------------------------------------------------------------------
std::string_view firstTableName(std::string_view sql) {
    auto p = sql.find("FROM ");
    if (p == std::string_view::npos) return {};
    size_t start = p + 5;
    while (start < sql.size() && sql[start] == ' ') ++start;
    size_t end = sql.find_first_of(" \t\n;,", start);
    if (end == std::string_view::npos) end = sql.size();
    return sql.substr(start, end - start);
}

int main() {
    demoBasic();
    demoDangling();

    std::cout << "\n=== LeetCode 58 ===\n";
    std::cout << lengthOfLastWord("Hello World") << "\n";              // 5
    std::cout << lengthOfLastWord("   fly me   to   ") << "\n";        // 2
    std::cout << lengthOfLastWord("luffy is still joyboy") << "\n";    // 6

    std::cout << "\n=== LeetCode 392 ===\n";
    std::cout << std::boolalpha
              << isSubsequenceView("abc", "ahbgdc") << "\n"    // true
              << isSubsequenceView("axc", "ahbgdc") << "\n";   // false

    std::cout << "\n=== 日常實務: 零配置 split ===\n";
    std::string csv = "alice,30,Taipei,engineer";
    auto tokens = splitView(csv, ',');
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "  [" << i << "] = \"" << tokens[i] << "\"\n";
    }

    std::cout << "\n=== 日常實務: firstTableName ===\n";
    std::cout << "[" << firstTableName("SELECT * FROM users WHERE id = 1") << "]\n";   // [users]
    std::cout << "[" << firstTableName("INSERT INTO logs VALUES (...)") << "]\n";       // []

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:string_view::data() 與 string::c_str() 有什麼根本差異?
    //    A:c_str() 保證 NUL-terminated;data() 不保證。view 可能是某個
    //      string 中段的子片段 (例如 sv.substr(2, 5)),其後一個 byte 不是
    //      '\0'。把 sv.data() 直接傳給 printf("%s") / fopen 等 C API 會
    //      讀到 view 範圍外的字元,屬 UB。要安全傳請走 std::string{sv}.c_str()。
    //
    //  Q2:為什麼 string_view 慣例「by value」傳遞而非 const 參考?
    //    A:sizeof(string_view) 只有 16 bytes (8 ptr + 8 size),對 64-bit
    //      平台正好兩個暫存器,by value 通常被呼叫者直接放在 register,
    //      省下 indirection。寫 const string_view& 反而多一層解參考且
    //      不能用 register 傳遞。
    //
    //  Q3:從返回 std::string 的函式接 string_view 為什麼 dangling?
    //    A:函式回傳的 string 是 prvalue/xvalue,只在「完整表達式結束」前
    //      存活。若你寫 std::string_view sv = makeString(); 這個 string
    //      的生命週期到本行末就結束 (lifetime extension 對 sv 無效,因為
    //      sv 不是 reference),sv 立刻 dangling。要嘛保留 string,要嘛
    //      在同一行用完就丟。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra string_view.cpp -o string_view

// === 預期輸出 (節錄) ===
// === LeetCode 392 ===
// true
// false
// === 日常實務: firstTableName ===
// [users]
// []
