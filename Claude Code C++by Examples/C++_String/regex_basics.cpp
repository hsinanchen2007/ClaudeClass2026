// =============================================================================
// 檔名: regex_basics.cpp
// 主題: <regex> 正規表達式基礎
//   std::regex / std::regex_match / std::regex_search /
//   std::regex_replace / std::sregex_iterator / std::smatch
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/regex
//   cplusplus.com: https://cplusplus.com/reference/regex/
// =============================================================================
//
// 【類別 / 函式資訊 Information】
//   namespace std {
//     class  regex;                      // 編譯好的正規表達式
//     class  smatch;                     // 對 std::string 的 match results
//     class  cmatch;                     // 對 const char* 的 match results
//
//     bool   regex_match (str, smatch&, regex);   // 整段匹配
//     bool   regex_search(str, smatch&, regex);   // 子字串匹配
//     string regex_replace(str, regex, repl);     // 取代
//
//     class  sregex_iterator;            // 走訪所有 match
//   }
//
//   regex 預設語法為 ECMAScript (近似 JS),也可指定 grep/egrep/awk/POSIX。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼要用正規表達式?
// ----------------------------------------------------------------------------
// 任何「依模式 (pattern) 比對 / 抽取 / 取代」的字串任務,正規表達式
// 通常是最簡潔表達意圖的工具:
//   - 驗證 email、電話、IP、UUID、URL。
//   - 從 log 抽取 timestamp、status code、user agent。
//   - 把多個 whitespace 壓縮為單一空白。
//   - 找出所有 hashtag、mention、URL。
//
// (二) regex_match vs regex_search 差別
// ----------------------------------------------------------------------------
//   - regex_match (s, re):整段 s 必須完全符合 re。
//   - regex_search(s, re):s 中只要有「子字串」符合 re 就算。
//
// 例子:re = "\\d+"  (一或多個數字)
//   "abc123" → regex_match  : false (整段不符合)
//             regex_search : true  ("123" 符合)
//   "123"   → regex_match  : true
//             regex_search : true
//
// (三) regex 為什麼這麼慢?
// ----------------------------------------------------------------------------
// std::regex 規格上要求支援 backtracking (回溯) 與 capture group,且 STL
// 實作 (尤其 libstdc++) 並未做最積極的優化,效能相較 RE2、Hyperscan、
// PCRE2 慢一個量級或更多。
//   - 對「熱路徑」的高吞吐解析任務,std::regex 通常 **不夠用**。
//   - 對「驗證 / 一次性 transform / 設定檔解析」這種低頻任務,std::regex
//     夠用且省事 (零外部相依)。
//
// (四) 編譯成本 vs 重用 regex 物件
// ----------------------------------------------------------------------------
// std::regex 的「建構」(parse + compile) 是昂貴的;呼叫 regex_match 才便宜。
// 在迴圈中**絕對不要每次重建** regex,改成:
//
//   static const std::regex re("…");        // compile 一次
//   for (auto& s : lines) {
//       if (std::regex_search(s, re)) { … }  // 重複使用
//   }
//
// (五) 不要用 std::regex 處理使用者輸入的 pattern 而不設防
// ----------------------------------------------------------------------------
// 著名的 "ReDoS" (Regex Denial-of-Service) 攻擊:某些 pattern 的回溯
// 行為對特定輸入是指數時間,讓 server 卡死。例:^(a+)+$ 對 "aaaa…!"
// 在許多 backtracking 引擎上是指數時間。
//
// 防禦:
//   - 別讓使用者直接送 regex 進來。
//   - 若必須,用 RE2 / Hyperscan 等「保證線性」的引擎。
//   - 限制 pattern 長度與輸入長度。
//
// (六) syntax_option_type 旗標
// ----------------------------------------------------------------------------
//   std::regex_constants::ECMAScript  (預設)
//   std::regex_constants::basic       (POSIX BRE)
//   std::regex_constants::extended    (POSIX ERE)
//   std::regex_constants::awk
//   std::regex_constants::grep / egrep
//   std::regex_constants::icase       (忽略大小寫)
//   std::regex_constants::multiline   (^ $ 匹配每行,C++17)
//
// (七) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++11 起加入 <regex>。
//   C++17 加入 multiline 旗標。
//   C++26 預期 std::regex 仍會存在,但 STL 內官方版本長期未升級;
//        若需高效能,業界廣泛採用 RE2 (Google) 或 ctre (compile-time).
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Capture Group
//    re = "(\\d{4})-(\\d{2})-(\\d{2})"
//    smatch m;
//    if (regex_search(s, m, re)) {
//        m[0]  // 整段匹配 ("2024-05-01")
//        m[1]  // 第一個 group ("2024")
//        m[2]  // ("05")
//        m[3]  // ("01")
//    }
//    Group 從 1 起算;m[0] 是整段。
//
// 2) Non-capturing group: (?:…)
//    當你只是要 group 邏輯但不需要捕捉,加 (?:),以節省 capture 開銷。
//
// 3) Greedy / Lazy
//    *、+、? 預設是 greedy(吃越多越好)。加 ? 變 lazy:*?、+?、??。
//    例如 ".*"  在 "<a><b>" 上吃整段 "<a><b>";".*?" 只吃 "<a>"。
//
// 4) Anchors:^ $ \\b
//    ^ 行首、$ 行尾、\\b 單字邊界。在 ECMAScript 預設下 ^ $ 只配整段
//    起始/結束;加 multiline 旗標後才會配每行。
//
// 5) sregex_iterator 取代手動 loop
//    要取出「所有」match 不只第一個,改用:
//        for (auto it = std::sregex_iterator(s.begin(), s.end(), re);
//             it != std::sregex_iterator(); ++it) {
//            std::smatch m = *it;
//            …;
//        }
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. regex 物件建構昂貴,務必 cache 重用 (static const)。
// 2. std::regex 預設語法為 ECMAScript;反斜線在 C++ 字串中要 escape
//    兩次:"\\d" 才是 "一個數字"。
// 3. std::regex 在 libstdc++ 上效能不佳;極端場景換 RE2 或 ctre。
// 4. 使用者可控的 pattern 要防 ReDoS。
// 5. regex_match 與 regex_search 語意不同,別寫錯。
// 6. smatch::operator[] 回傳 const sub_match&;使用 m[1].str() 取 std::string,
//    或 m[1].first / m[1].second 取 iterator 範圍。
//
// =============================================================================

/*
補充筆記：std::regex
  - std::regex 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::regex 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <regex>
#include <vector>

void demoMatchSearch() {
    std::cout << "=== regex_match vs regex_search ===\n";
    std::regex re("\\d+");
    std::cout << std::boolalpha
              << "match (\"abc123\") = " << std::regex_match("abc123", re) << "\n"
              << "match (\"123\")    = " << std::regex_match("123", re) << "\n"
              << "search(\"abc123\") = " << std::regex_search("abc123", re) << "\n";
}

void demoCapture() {
    std::cout << "\n=== capture group ===\n";
    std::string s = "今天日期: 2024-05-01,明天: 2024-05-02";
    std::regex  re(R"((\d{4})-(\d{2})-(\d{2}))");

    for (auto it = std::sregex_iterator(s.begin(), s.end(), re);
         it != std::sregex_iterator(); ++it) {
        std::smatch m = *it;
        std::cout << "整段: " << m[0].str()
                  << "  年: " << m[1].str()
                  << "  月: " << m[2].str()
                  << "  日: " << m[3].str() << "\n";
    }
}

void demoReplace() {
    std::cout << "\n=== regex_replace ===\n";
    std::string s = "Hello,    World\t! How are\tyou?";
    std::regex  ws(R"(\s+)");
    std::string normalized = std::regex_replace(s, ws, " ");
    std::cout << "normalize whitespace: \"" << normalized << "\"\n";

    // 用 backreference $1 重排格式: "first last" → "last, first"
    std::string name = "Ada Lovelace";
    std::regex  re(R"((\w+)\s+(\w+))");
    std::cout << "rearrange: "
              << std::regex_replace(name, re, "$2, $1") << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 65. Valid Number (Hard) — 簡化驗證版
//
// 題目敘述:
//   給字串,判斷是否為「合法數字」(可帶 sign、有小數點、可有指數 e/E)。
//   合法範例: "0", "0.1", ".1", "2e10", "-90E3", "3.", "-.9", "+6e-1"
//   非法範例: "abc", "1a", "e3", "99e2.5", " "
//
// 為何用 regex:
//   數字格式的「文法」很適合用 regex 表達:
//     ^[+-]? ( \d+ \.? \d* | \.\d+ ) ( [eE][+-]?\d+ )? $
//   一行表達意圖,比手寫 state machine 短且不易出錯 (本題 LeetCode
//   官方 editorial 也常見用 regex)。
//
//   注意:面試時對方可能要求手寫 DFA;regex 寫法當作熱身範例。
//
// 複雜度: 時間 O(n),空間 O(n) (regex 編譯)
// -----------------------------------------------------------------------------
bool isNumber(const std::string& s) {
    static const std::regex re(R"(^[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?$)");
    return std::regex_match(s, re);
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】從 access log 取出 timestamp + status + 路徑
//
// 為何用 regex:
//   後端的 access log 格式相對固定,例如:
//     "127.0.0.1 - - [01/May/2024:12:34:56 +0000] \"GET /api/users HTTP/1.1\" 200 1234"
//   要從中拆出時間、HTTP method、路徑、狀態碼,regex 是最快寫法。
//   對「一次性報表」「ad-hoc 分析」「dev 環境 log debug」都適合;
//   生產環境高流量解析應改用 RE2 或專屬 parser (faster, no ReDoS)。
// -----------------------------------------------------------------------------
struct AccessRecord {
    std::string timestamp;
    std::string method;
    std::string path;
    int         status = 0;
};

bool parseAccessLog(const std::string& line, AccessRecord& out) {
    static const std::regex re(
        R"(\[([^\]]+)\]\s+"(\w+)\s+([^"]+?)\s+HTTP/[\d.]+"\s+(\d+))");
    std::smatch m;
    if (!std::regex_search(line, m, re)) return false;
    out.timestamp = m[1].str();
    out.method    = m[2].str();
    out.path      = m[3].str();
    out.status    = std::stoi(m[4].str());
    return true;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1location7. 1location7. Long Pressed Name -簡化思路
// 題目: LeetCode 1page. 1page0. Find All Numbers Disappeared in an Array - 字串版
// 變奏: 用 regex 從一段文字中抽出所有整數,回傳 vector<int>
// 為何用 regex: regex_iterator + sregex_iterator 直接走訪所有 match,簡潔。
// 複雜度: O(N)。
// 難度: easy
// -----------------------------------------------------------------------------
std::vector<int> extractIntegers(const std::string& text) {
    static const std::regex re(R"(-?\d+)");
    std::vector<int> out;
    auto begin = std::sregex_iterator(text.begin(), text.end(), re);
    auto end   = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        try { out.push_back(std::stoi(it->str())); }
        catch (...) { /* skip out-of-range */ }
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】驗證 email 格式 (簡化版,不嚴格符合 RFC 5322)
// 為何用 regex: 簡單一行 regex 通常已足以擋掉 80% 的明顯錯誤輸入。
// -----------------------------------------------------------------------------
bool isValidEmail(const std::string& s) {
    static const std::regex re(R"(^[\w.+-]+@[\w.-]+\.[A-Za-z]{2,}$)");
    return std::regex_match(s, re);
}

int main() {
    demoMatchSearch();
    demoCapture();
    demoReplace();

    std::cout << "\n=== LeetCode 65 (簡化版) ===\n";
    for (auto* s : {"0", "0.1", ".1", "2e10", "-90E3",
                    "abc", "e3", "99e2.5"}) {
        std::cout << "\"" << s << "\" → " << std::boolalpha
                  << isNumber(s) << "\n";
    }

    std::cout << "\n=== LeetCode 變奏 (extractIntegers) ===\n";
    for (int v : extractIntegers("there are 3 apples, 5 oranges and -2 lemons"))
        std::cout << v << " ";
    std::cout << "\n";

    std::cout << "\n=== 日常實務: isValidEmail ===\n";
    for (const char* s : {"alice@example.com", "bad@", "no-at-sign", "a.b+x@y.io"}) {
        std::cout << s << " -> " << std::boolalpha << isValidEmail(s) << "\n";
    }

    std::cout << "\n=== 日常實務: access log 解析 ===\n";
    std::string line =
        "127.0.0.1 - - [01/May/2024:12:34:56 +0000] "
        "\"GET /api/users HTTP/1.1\" 200 1234";
    AccessRecord r;
    if (parseAccessLog(line, r)) {
        std::cout << "ts="     << r.timestamp << "\n"
                  << "method=" << r.method    << "\n"
                  << "path="   << r.path      << "\n"
                  << "status=" << r.status    << "\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:std::regex 比 PCRE / RE2 慢多少?該用哪個?
    //    A:libstdc++ 的 std::regex 比 PCRE2 慢約 5~20 倍,比 RE2 慢更多。
    //      原因:標準要求 backtracking 與 capture group,且 STL 多年未做
    //      JIT 等優化。低頻驗證用 std::regex 即可;高吞吐 log 解析請改
    //      用 RE2 (保證線性) 或 ctre (compile-time regex,零執行期成本)。
    //
    //  Q2:為什麼 regex 物件一定要寫 static const?
    //    A:regex 的「建構」(parse + NFA 編譯) 是昂貴步驟,呼叫
    //      regex_match/search 才便宜。在迴圈中每次重建會把編譯成本累乘 N
    //      倍。以 static const 在第一次呼叫時 init 一次、之後重用,
    //      效能可差 10 倍以上。
    //
    //  Q3:什麼是 ReDoS,為什麼 ^(a+)+$ 對 "aaaa...!" 是炸彈?
    //    A:Regex Denial-of-Service。某些 pattern 對 backtracking 引擎
    //      會產生指數級的回溯路徑,讓單一輸入耗盡 CPU。^(a+)+$ 對全 a
    //      的字串有 2^n 種拆分;末尾 '!' 不匹配迫使引擎窮舉所有拆分。
    //      解法:不接使用者 pattern、限長度、改用 RE2 等線性引擎。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra regex_basics.cpp -o regex_basics

// === 預期輸出 (節錄) ===
// === LeetCode 變奏 (extractIntegers) ===
// 3 5 -2
// === 日常實務: isValidEmail ===
// alice@example.com -> true
// bad@ -> false
// no-at-sign -> false
// a.b+x@y.io -> true
