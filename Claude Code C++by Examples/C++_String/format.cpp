// =============================================================================
// 檔名: format.cpp
// 主題: <format> std::format / std::vformat / format_to (C++20)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/utility/format
//   cppreference (format spec): https://en.cppreference.com/cpp/utility/format/spec
//   cplusplus.com: https://cplusplus.com/reference/format/
// =============================================================================
//
// 【函式資訊 Information】
//   namespace std {
//     // 1. 直接回傳 std::string
//     template<class... Args>
//     std::string format(std::format_string<Args...> fmt, Args&&... args);
//
//     // 2. type-erased (拿到 vformat_args,延遲到執行期格式化)
//     std::string vformat(std::string_view fmt, std::format_args args);
//
//     // 3. 寫到 OutputIterator(可寫到 vector<char>、ostreambuf_iterator 等)
//     template<class OutputIt, class... Args>
//     OutputIt format_to(OutputIt out, std::format_string<Args...> fmt,
//                        Args&&... args);
//
//     // 4. 預先計算長度 (不寫入)
//     template<class... Args>
//     std::size_t formatted_size(std::format_string<Args...> fmt,
//                                Args&&... args);
//   }
//
// C++23 加上 std::print / std::println,可直接列印不需先 format 成 string。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼要 std::format?
// ----------------------------------------------------------------------------
// C++20 之前的字串格式化選項:
//   - printf 系列:不安全 (型別不檢查)、locale-dependent、不擴充自訂型別。
//   - iostream <<:型別安全但極為冗長,且 manipulator (std::setw 等) 難記。
//   - 第三方:Boost.Format、{fmt} 函式庫 (廣泛採用,語法即 Python 風格)。
//
// std::format 直接把廣受喜愛的 {fmt} library 標準化進入 C++:
//   - **編譯期型別檢查** —— 寫錯參數型別、佔位符數目都不會通過編譯。
//   - **不依賴 locale** (除非顯式要求)。
//   - **可擴充** —— 你可以為自己的型別實作 std::formatter<T> 特化。
//   - **快** —— 比 printf / stringstream 都快。
//
// (二) 格式語法概要 (與 Python 相同)
// ----------------------------------------------------------------------------
//   "{}"                      → 預設格式
//   "{0} {1}"                 → 指定參數編號 (positional)
//   "{:>10}"                  → 寬度 10、右對齊
//   "{:<10}"                  → 左對齊
//   "{:^10}"                  → 置中
//   "{:*^10}"                 → 用 '*' 填滿置中
//   "{:+}"                    → 顯示正號
//   "{:.3f}"                  → 浮點 3 位小數
//   "{:e}"                    → 科學記號
//   "{:#x}"                   → 0xff;# 加 base 前綴
//   "{:08b}"                  → 二進位、寬度 8、補 0
//   "{:%Y-%m-%d}"             → 時間/日期 (chrono)
//
// (三) 編譯期 vs 執行期格式字串
// ----------------------------------------------------------------------------
//   std::format("...", args) — 「fmt」是 std::format_string<Args...>,
//     在編譯期檢查!型別不對、參數量不對、佔位符語法錯,直接編譯失敗。
//
//   std::vformat(fmt, std::make_format_args(args)) — 「fmt」是 string_view,
//     可在執行期動態組合 (例如從 i18n 檔讀來的格式),但失去編譯期檢查,
//     錯誤改為丟 std::format_error。
//
// 90% 場景用 std::format;只在「字串非編譯期常量」時用 vformat。
//
// (四) format_to / formatted_size 的價值
// ----------------------------------------------------------------------------
//   - format_to(out, fmt, args...): 把結果寫到任意 OutputIterator,
//     可避免建立中間 std::string。配合 std::back_inserter(vec) 直接
//     寫入 vector,或 std::ostreambuf_iterator 寫入 ostream。
//   - formatted_size(fmt, args...): 不寫資料,只回傳會寫多少 byte。
//     適合「先量再寫」(自己 reserve buffer 大小)。
//
// (五) 為什麼比 printf / stringstream 快?
// ----------------------------------------------------------------------------
//   - 編譯期解析格式字串 → 不需執行期 parse。
//   - 直接走 std::to_chars (charconv) 而非 printf 的 locale-aware path。
//   - 無 virtual call、無 sentry、無 locale facet 查找。
//
// (六) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++20    : 加入 std::format (P0645)。GCC 13、Clang 17、MSVC 19.30 起支援。
//   C++23    : 加入 std::print / std::println (P2093),可直接列印。
//              加入 std::formatter 對 std::ranges 的支援 (P2286)。
//   C++26    : 預期擴充 std::formatter 對更多型別的內建 specialization。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Compile-time format string checking
//    std::format_string<Args...> 是 C++20 引入的「consteval-friendly」型別,
//    它在編譯期解析格式字串並驗證 Args... 是否匹配。
//    如果你寫 std::format("{:d}", "hello"),編譯就會失敗 —— 字串不能 :d。
//    這比 printf 的「runtime UB」進步太多。
//
// 2) std::formatter<T> 自訂特化
//    要讓自己的 type 支援 {} 格式化:
//      template<> struct std::formatter<MyType> {
//          constexpr auto parse(std::format_parse_context& ctx) { ... }
//          auto format(const MyType& v, std::format_context& ctx) const { ... }
//      };
//    一旦特化,std::format("{}", myObj) 即可使用,介面等同 std type。
//
// 3) Locale 預設 = "C" 而非系統 locale
//    這是 std::format 與 printf 最大的安全差異 —— 不會因為使用者切了
//    德文 locale 就把 1.5 印成 "1,5",造成 CSV 解析悲劇。要 locale-aware
//    需明確 std::format(std::locale("de_DE"), "{:L}", 1.5)。
//
// 4) std::print (C++23) 的角色
//    把「format + write 到 stdout」合併,且預設用 fwrite 而非 printf,
//    對 UTF-8 友善:
//        std::println("Hello {}!", "World");
//
// 5) 與 fmt 函式庫的關係
//    {fmt} 是 Victor Zverovich 的開源函式庫,std::format 直接從它標準化
//    而來。若你的編譯器太舊,可改用 {fmt} (https://github.com/fmtlib/fmt),
//    語法 100% 相容,未來無痛換到 std::format。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 編譯器要夠新 (GCC 13+ / Clang 17+ / MSVC 19.30+)。GCC 12 之前對
//    <format> 支援不完整。
// 2. std::format("{}", value) 的 value 型別必須有 std::formatter 特化;
//    對自訂型別需先寫特化才能用。
// 3. format_string 在編譯期檢查,字面量錯了會 compile-fail —— 這是「好事」。
// 4. 避免在 hot path 重複 parse 格式字串;若反覆使用同一個格式,可以
//    用 vformat + std::make_format_args 配合預先 cache 格式 string。
// 5. std::format 的浮點預設無小數位限制 (走 charconv 的 short round-trip),
//    與 printf "%f" (預設 6 位) 行為不同。
// 6. 如果輸出包含「{」或「}」字面量,要寫成 {{ 或 }} 跳脫。
//
// =============================================================================

/*
補充筆記：std::format
  - std::format 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::format 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <iterator>

// GCC 13.3 上 <format> 已可用;若你的編譯器較舊,可改用 fmtlib。
#include <format>

void demoBasic() {
    std::cout << "=== std::format 基本用法 ===\n";

    // 預設格式
    std::cout << std::format("{} = {}\n", "answer", 42);

    // positional + 寬度 + 對齊
    std::cout << std::format("|{:<10}|{:^10}|{:>10}|\n", "L", "C", "R");

    // fill char + width + base + sign
    std::cout << std::format("hex={:#08x}  bin={:08b}  signed={:+d}\n",
                             255, 5, 7);

    // 浮點格式
    std::cout << std::format("pi={:.3f}  sci={:.4e}\n", 3.14159, 0.00012345);
}

void demoFormatTo() {
    std::cout << "\n=== format_to (寫到任意 output iterator) ===\n";
    std::vector<char> buf;
    std::format_to(std::back_inserter(buf), "user={} balance={:.2f}\n",
                   "alice", 1234.5);
    std::cout.write(buf.data(), static_cast<std::streamsize>(buf.size()));

    // formatted_size:量長度但不寫資料
    auto n = std::formatted_size("{}={}", "x", 12345);
    std::cout << "formatted_size = " << n << "\n";
}

void demoVformat() {
    std::cout << "\n=== vformat (執行期格式字串) ===\n";

    // 模擬從設定檔/i18n 讀來的字串:不能是編譯期常量
    std::string fmt = "Hello, {}! You have {} unread messages.\n";

    // make_format_args 要求 lvalue 參數;先放成具名變數
    std::string user = "alice";
    int unread = 7;
    std::string out = std::vformat(fmt, std::make_format_args(user, unread));
    std::cout << out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 1108. Defanging an IP Address (Easy)
//
// 題目敘述:
//   給合法 IPv4 字串,把每個 '.' 換成 "[.]",回傳新字串。
//   範例: "1.1.1.1" → "1[.]1[.]1[.]1"
//
// 為何用 std::format:
//   雖然此題用 string::replace 也很短,但若我們先 split 出四段,
//   format 可以一行就把它們重新「以 [.] 為分隔符」串起來:
//     std::format("{}[.]{}[.]{}[.]{}", a, b, c, d)
//   這示範了 format 在「結構化拼裝字串」時的可讀性優勢。
//
// 解題思路:
//   sscanf-style 或自己解析 4 個段;直接 format 出來。
//
// 複雜度: 時間 O(n),空間 O(n)
// -----------------------------------------------------------------------------
std::string defangIPaddr(const std::string& addr) {
    int a, b, c, d;
    std::sscanf(addr.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d);
    return std::format("{}[.]{}[.]{}[.]{}", a, b, c, d);
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】結構化 log line 與表格輸出
//
// 為何用 std::format:
//   後端 log / report / CLI 輸出常需要「對齊欄位」「右靠齊數字」
//   「固定小數位」「以 padding 字元填滿」等,printf 寫起來腦熱、
//   stringstream 寫起來臃腫;std::format 一行清晰表達意圖。
//   且編譯期型別檢查讓你不會把 int 印成 %s 而靜默 UB。
// -----------------------------------------------------------------------------
struct Row {
    std::string user;
    int requests;
    double avg_ms;
};

void printReport(const std::vector<Row>& rows) {
    std::cout << std::format("{:<12}{:>10}{:>12}\n", "USER", "REQS", "AVG_MS");
    std::cout << std::format("{:-<34}\n", "");
    for (const auto& r : rows) {
        std::cout << std::format("{:<12}{:>10}{:>12.2f}\n",
                                 r.user, r.requests, r.avg_ms);
    }
}

#include <cstdio>

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 412. Fizz Buzz
// 題目: 1..n,若被 3 整除輸出 Fizz、5 整除輸出 Buzz、15 都整除輸出 FizzBuzz,
//       否則輸出該數字。
// 為何用 format: std::format("{}", n) 安全地把 int 轉成 std::string,
//                取代 std::to_string 但有編譯期型別檢查。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::vector<std::string> fizzBuzz(int n) {
    std::vector<std::string> out;
    out.reserve(static_cast<size_t>(n));
    for (int i = 1; i <= n; ++i) {
        if      (i % 15 == 0) out.emplace_back("FizzBuzz");
        else if (i %  3 == 0) out.emplace_back("Fizz");
        else if (i %  5 == 0) out.emplace_back("Buzz");
        else                  out.emplace_back(std::format("{}", i));
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把秒數格式化為 "HH:MM:SS"
// 為何用 format: 一行解,寬度補零用 "{:02}" 即可,比 sprintf 安全。
// 場景: log 顯示經過時間、影片時間軸、報表 duration。
// -----------------------------------------------------------------------------
std::string formatHMS(int totalSeconds) {
    int h = totalSeconds / 3600;
    int m = (totalSeconds / 60) % 60;
    int s = totalSeconds % 60;
    return std::format("{:02}:{:02}:{:02}", h, m, s);
}

int main() {
    demoBasic();
    demoFormatTo();
    demoVformat();

    std::cout << "\n=== LeetCode 1108 ===\n";
    std::cout << defangIPaddr("1.1.1.1") << "\n";
    std::cout << defangIPaddr("255.100.50.0") << "\n";

    std::cout << "\n=== LeetCode 412 ===\n";
    for (auto& s : fizzBuzz(15)) std::cout << s << " ";
    std::cout << "\n";

    std::cout << "\n=== 日常實務: 表格輸出 ===\n";
    printReport({
        {"alice",   1234,   42.768},
        {"bob",       55, 1234.500},
        {"carol", 999999,    0.123},
    });

    std::cout << "\n=== 日常實務: formatHMS ===\n";
    std::cout << formatHMS(3661) << "\n";    // 01:01:01
    std::cout << formatHMS(45)   << "\n";    // 00:00:45

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:std::format 的格式字串為什麼能在編譯期檢查?關鍵 type 叫什麼?
    //    A:第一個參數是 std::format_string<Args...> (C++20),它是 consteval
    //       建構,編譯器會在編譯期解析格式字串並驗證 Args 型別與數量。
    //       寫錯 (如 "{:d}" 配 string) 直接 compile-fail,擺脫了 printf 的 runtime UB。
    //
    //  Q2:std::format("{:.2f}", x) 的浮點輸出會受 LANG=de_DE 影響嗎?
    //    A:預設不會。std::format 預設使用 "C" locale,小數點固定為 '.';
    //       要 locale-aware 必須顯式傳 std::format(std::locale("de_DE"), "{:L}", x)
    //       並加上 'L' specifier。這是它跟 printf "%f" 最大的安全差異。
    //
    //  Q3:既然 format string 編譯期檢查,那「從 i18n 檔讀來的格式字串」怎麼用?
    //    A:用 std::vformat(fmt_sv, std::make_format_args(args...))。它接受
    //       string_view 跑 runtime parsing,失敗時丟 std::format_error。
    //       90% 場景用 std::format,只在動態格式時退到 vformat。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra format.cpp -o format
// 注意: 需要 GCC 13+ / Clang 17+ / MSVC 19.30+ 對 <format> 的完整支援。

// === 預期輸出 (節錄) ===
// === LeetCode 412 ===
// 1 2 Fizz 4 Buzz Fizz 7 8 Fizz Buzz 11 Fizz 13 14 FizzBuzz
// === 日常實務: formatHMS ===
// 01:01:01
// 00:00:45
