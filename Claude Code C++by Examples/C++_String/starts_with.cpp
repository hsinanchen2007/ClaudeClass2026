// =============================================================================
// 檔名: starts_with.cpp
// 主題: std::string::starts_with (C++20: 檢查字串開頭)
// 參考連結 (References):
//   - cppreference: https://en.cppreference.com/cpp/string/basic_string/starts_with
//   - cplusplus.com: https://cplusplus.com/reference/string/string/
//     (cplusplus.com 尚未收錄此 C++20 新成員;主頁仍可作為 std::string 的整體參考)
// =============================================================================
//
// 【函式資訊 Information】
//   constexpr bool starts_with(std::string_view sv) const noexcept;       // (1)
//   constexpr bool starts_with(char ch) const noexcept;                    // (2)
//   constexpr bool starts_with(const char* s) const;                       // (3)
//
// 所屬類別: std::basic_string<CharT, Traits, Allocator>
// 回傳型別: bool
// 參數:
//   (1) sv : 任何可隱式轉成 std::string_view 的物件 (string、const char*、literals)
//   (2) ch : 單一字元
//   (3) s  : C-style 字串 (\0 結尾)
//
// 標準: C++20 加入。在此之前需用 compare(0, n, prefix) == 0 或 substr 自行判斷。
//
// 【詳細解釋 Explanation】
//
// (1) 函式定位 ── 一個被等了二十年的便利函式:
//     C++ 從 1998 年 C++98 起就有 std::string,但「判斷字串以 X 開頭」這種日常需求
//     一直沒有便利函式。十多年來 C++ 工程師被迫寫:
//
//         // 寫法 A:用 compare (容易出錯,參數順序很饒口)
//         bool sw = s.compare(0, prefix.size(), prefix) == 0;
//
//         // 寫法 B:用 substr (要 allocate 一個臨時 string!效率差)
//         bool sw = s.substr(0, prefix.size()) == prefix;
//
//         // 寫法 C:rfind(prefix, 0) (語意奇怪,但效率好)
//         bool sw = s.rfind(prefix, 0) == 0;
//
//     C++20 終於加入 starts_with,讓這個高頻操作變回直觀:
//
//         bool sw = s.starts_with(prefix);
//
// (2) 為何「無需 allocation」是重點:
//     寫法 B 的 substr 會建立新 string;對長字串、頻繁呼叫的場景會明顯拖慢效能。
//     starts_with 內部只比對 prefix 長度的字元,不配置任何記憶體,O(prefix.size()) 即返。
//     等同於這個 reference implementation:
//
//         constexpr bool starts_with(string_view sv) const noexcept {
//             return size() >= sv.size() && compare(0, sv.size(), sv) == 0;
//         }
//
//     注意它先檢查長度,prefix 比本身長就直接回 false ── 這也是它 noexcept 的基礎。
//
// (3) 三種重載的選擇:
//     - sv (string_view) : 最常用、最泛用。可吃 string、const char*、literal、
//                          std::string_view 等所有「字串型別」。
//     - char             : 比 sv("c") 更精準,且絕對 noexcept。
//                          常用於檢查首字符 (注釋字元、開頭符號)。
//     - const char*      : 與 sv 重疊;留著主要為了避免某些 SFINAE 歧義,實務上少直接用。
//                          注意它不是 noexcept ── 若 const char* 為 nullptr 是 UB。
//
// (4) 時間複雜度: O(N),N 是 prefix 長度。長字串檢查短前綴極快。
//
// (5) 與 starts_with 配套的 ends_with、contains:
//     C++20 同時加入 ends_with;C++23 加入 contains。三者語意一致、簽章對稱,
//     共同補齊了 std::string 的「開頭/結尾/包含」三大查詢工具。
//
// 【注意事項 Pay Attention】
// 1. 需 C++20。較舊編譯器需用 compare 替代:
//      bool sw = (s.compare(0, prefix.size(), prefix) == 0);
// 2. starts_with(const char*) 不接受 nullptr。
// 3. 若 prefix 比 string 長,直接回 false (不會丟例外)。
// 4. C++23 還新增了 contains() 系列。
// 5. 編譯器版本: GCC 11+、Clang 12+、MSVC 19.22+ 才完整支援。
// 6. 對 std::string_view 也有對應的 starts_with (C++20),語意一致。
//
// 【概念補充 Concept Deep Dive】
//
// (A) string_view 重載為什麼這麼重要:
//     std::string_view 是 C++17 加入的「不擁有」字串視圖,只持有指標 + 長度。
//     starts_with 接受 string_view 表示:呼叫端傳什麼類型都行,完全不需 allocate:
//
//         std::string s = "Hello, World";
//         std::string prefix = "Hello";
//         const char* cstr   = "Hello";
//         std::string_view sv = "Hello";
//
//         s.starts_with(prefix);   // OK,免拷貝
//         s.starts_with(cstr);     // OK,從 const char* 隱式構造 string_view
//         s.starts_with(sv);       // OK,直接傳 view
//         s.starts_with("Hello");  // OK,string literal 直接化為 string_view
//
//     反觀如果只接受 const string&,傳 "Hello" 時會建立暫時 std::string,效率差。
//
// (B) 為什麼 C++20 才加入這個「明顯該有」的函式?
//     - C++ 標準演進緩慢,小型便利函式經常被「太瑣碎、不值得加入標準」否決
//     - 加入 string_view (C++17) 後,starts_with 才有「最佳簽章」可用
//     - 同時 C++20 引入 spaceship、modules 等大改動,連帶補了這類缺口
//     - Java 從 JDK 1.0 (1996) 就有 startsWith;Python 也有 str.startswith ──
//       C++ 是直到 2020 才補上。
//
// (C) 實作面 ── 標準庫如何最佳化:
//     許多實作 (libstdc++, libc++) 內部用 char_traits::compare,
//     即 memcmp 的等價,通常被 SIMD 優化,長 prefix 也很快。
//
// (D) 與 std::ranges 的搭配:
//     C++23 起 std::ranges::starts_with 提供更通用的版本,可作用於任何 range
//     (vector、list、自訂 range)。對純字串需求 string::starts_with 仍較直接。
//
// =============================================================================

/*
補充筆記：std::string::starts_with
  - std::string::starts_with 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::starts_with 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <vector>

void demoStartsWith() {
    std::string s = "Hello, World!";
    std::cout << std::boolalpha;
    std::cout << s.starts_with("Hello") << "\n";  // true
    std::cout << s.starts_with('H')     << "\n";  // true
    std::cout << s.starts_with("hi")    << "\n";  // false

    // C++20 之前的等效寫法:
    bool legacy = (s.compare(0, 5, "Hello") == 0);
    std::cout << "legacy = " << legacy << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1957. Delete Characters to Make Fancy String
// 題目: 刪除最少字元,使字串中沒有「同一字元連續出現 ≥ 3 次」。
// 為何用 starts_with: 雖然這題用 back() 檢查更直覺,我們示範用 starts_with
//                     在窗口檢查,熟悉它的呼叫方式。
// -----------------------------------------------------------------------------
std::string makeFancyString(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        size_t n = out.size();
        if (n >= 2 && out[n-1] == c && out[n-2] == c) continue;
        out += c;
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【實務範例】解析 HTTP 請求行
// 為何用 starts_with: 判斷 method 類型。
// -----------------------------------------------------------------------------
std::string detectMethod(const std::string& line) {
    if (line.starts_with("GET "))    return "GET";
    if (line.starts_with("POST "))   return "POST";
    if (line.starts_with("PUT "))    return "PUT";
    if (line.starts_with("DELETE ")) return "DELETE";
    return "UNKNOWN";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】判斷是絕對路徑 or 相對路徑 / 是 URL 還是 path
// 為何用 starts_with: 路徑與 URL 的判斷在檔案瀏覽器、設定載入、CLI 工具
//                     都會用到。多個 starts_with 串起來最直觀。
// -----------------------------------------------------------------------------
enum class PathKind { Absolute, Relative, HttpUrl, FileUrl };

PathKind classifyPath(const std::string& s) {
    if (s.starts_with("http://") || s.starts_with("https://")) return PathKind::HttpUrl;
    if (s.starts_with("file://"))                              return PathKind::FileUrl;
    if (s.starts_with("/") || (s.size() > 1 && s[1] == ':'))   return PathKind::Absolute;
    return PathKind::Relative;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 2185. Counting Words With a Given Prefix
// 題目: 計算 words 中以 prefix 開頭的字串數量。
// 為何用 starts_with: 一行解,語意完美對應題意。
// 複雜度: O(N * prefix.size())。
// -----------------------------------------------------------------------------
int prefixCount(const std::vector<std::string>& words, const std::string& prefix) {
    int cnt = 0;
    for (const auto& w : words) if (w.starts_with(prefix)) ++cnt;
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】判斷字串是「IPv6」開頭 (含 ':' 字元在前) 還是 IPv4
// 為何用 starts_with: 簡單前綴條件直接判斷;若整段都是數字 + '.' 是 IPv4,
//                     含 ':' 即 IPv6。
// -----------------------------------------------------------------------------
std::string ipKind(const std::string& s) {
    if (s.starts_with("::") || s.find(':') != std::string::npos) return "IPv6";
    return "IPv4";
}

int main() {
    demoStartsWith();
    std::cout << "\n=== LeetCode 1957 ===\n";
    std::cout << makeFancyString("leeetcode") << "\n";        // "leetcode"
    std::cout << makeFancyString("aaabaaaa")  << "\n";        // "aabaa"

    std::cout << "\n=== detectMethod ===\n";
    std::cout << detectMethod("GET /index HTTP/1.1") << "\n";
    std::cout << detectMethod("POST /api HTTP/1.1") << "\n";

    std::cout << "\n=== LeetCode 2185 ===\n";
    std::cout << prefixCount({"pay","attention","practice","attend"}, "at") << "\n";  // 2
    std::cout << prefixCount({"leetcode","win","loops","success"}, "code") << "\n";   // 0

    std::cout << "\n=== 日常實務: classifyPath ===\n";
    auto names = std::vector{"https://example.com","/etc/hosts","./relative.txt","C:\\Users","file:///tmp/x"};
    for (auto& s : names) {
        auto k = classifyPath(s);
        std::cout << s << " -> "
                  << (k == PathKind::HttpUrl  ? "HttpUrl" :
                      k == PathKind::FileUrl  ? "FileUrl" :
                      k == PathKind::Absolute ? "Absolute" : "Relative") << "\n";
    }

    std::cout << "\n=== 日常實務: ipKind ===\n";
    std::cout << "192.168.1.1 -> " << ipKind("192.168.1.1") << "\n";
    std::cout << "::1 -> "         << ipKind("::1")         << "\n";
    std::cout << "fe80::1 -> "     << ipKind("fe80::1")     << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:starts_with(prefix) 與 substr(0, prefix.size()) == prefix 差在哪?
    //    A:starts_with O(N) 且零配置;substr 會建立新 std::string (對長字串
    //      可能 alloc 與拷貝),效能差一個量級。starts_with 還會先比 size,
    //      prefix 較長直接 false,連比都不比。
    //
    //  Q2:為什麼 C++20 才加,前 22 年 C++ 程式員怎麼活的?
    //    A:常見替代品有三種 — compare(0, n, prefix)==0 (參數順序饒口)、
    //      substr(0,n)==prefix (有 alloc 成本)、rfind(prefix, 0)==0 (語意
    //      奇怪但效能好)。C++20 加 string_view 後才有「最佳簽章」可寫,
    //      連同 ends_with 一併補齊。
    //
    //  Q3:starts_with(const char*) 是 noexcept 嗎?傳 nullptr 會怎樣?
    //    A:string_view 與 char 重載是 noexcept;const char* 重載「不是」
    //      noexcept,因為它要先以 char_traits::length 算長度。傳 nullptr
    //      是 UB (內部會 strlen)。string_view 重載最安全也最泛用,優先選它。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra starts_with.cpp -o starts_with

// === 預期輸出 (節錄) ===
// === LeetCode 2185 ===
// 2
// 0
// === 日常實務: ipKind ===
// 192.168.1.1 -> IPv4
// ::1 -> IPv6
// fe80::1 -> IPv6
