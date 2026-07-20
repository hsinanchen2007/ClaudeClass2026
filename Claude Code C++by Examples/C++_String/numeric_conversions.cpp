// =============================================================================
// 檔名: numeric_conversions.cpp
// 主題: std::stoi / stol / stoll / stof / stod / to_string / to_chars (數值轉換)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/stol
//   - https://en.cppreference.com/cpp/string/basic_string/to_string
//   - https://cplusplus.com/reference/string/stoi/
//   - https://cplusplus.com/reference/string/stol/
//   - https://cplusplus.com/reference/string/stof/
//   - https://cplusplus.com/reference/string/to_string/
// =============================================================================
//
// 【函式資訊 Information】
// 字串 → 數值 (C++11):
//   int       std::stoi (const string& s, size_t* pos = nullptr, int base = 10);
//   long      std::stol (const string& s, size_t* pos = nullptr, int base = 10);
//   long long std::stoll(const string& s, size_t* pos = nullptr, int base = 10);
//   unsigned long      std::stoul (...);
//   unsigned long long std::stoull(...);
//   float     std::stof (const string& s, size_t* pos = nullptr);
//   double    std::stod (const string& s, size_t* pos = nullptr);
//   long double std::stold(...);
//
// 數值 → 字串 (C++11):
//   std::string std::to_string(int);
//   std::string std::to_string(long);
//   std::string std::to_string(double); ...等
//
// C++17 加入更高效的 std::to_chars / std::from_chars (在 <charconv>)
// — 不會丟例外,locale 中立,效能與精度更好。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// 【1. stoi 系列的解析步驟 (簡化規格)】
//   1. 跳過前導空白 (依當前 locale 判斷)
//   2. 解析可選正負號 (+ / -)
//   3. 解析 base 對應的「合法字元」前綴 (0x / 0)
//   4. 收斂最長合法數字序列
//   5. 若一個合法字元都沒有 → throw std::invalid_argument
//   6. 若超出回傳型別範圍 → throw std::out_of_range
//   7. pos 指標 (若非 nullptr) 寫入「停止解析的位置」(下一個未消化字元的 index)
//
// 【2. base 參數規則】
//   - base = 10 (預設):純十進位數字
//   - base = 16:接受 0-9 a-f A-F,可選 0x / 0X 前綴
//   - base = 8:接受 0-7,可選 0 前綴
//   - base = 0:自動偵測 (0x → 16 進位,0 → 8 進位,其餘 → 10 進位)
//   - base = 2..36:任意基底 (字母 a-z 代表 10..35)
//
// 【3. pos 參數的妙用 ── 連續 token 解析】
//   string s = "12 34 56";
//   size_t i = 0;
//   while (i < s.size()) {
//       size_t consumed;
//       int v = std::stoi(s.substr(i), &consumed);   // 從子字串開始
//       i += consumed;
//       while (i < s.size() && s[i] == ' ') ++i;     // skip space
//   }
//
// 【4. 例外型別】
//   - std::invalid_argument:第一個字元就無法解析。
//   - std::out_of_range:解析成功但數值溢位。
//   - 其他 (bad_alloc 等):極少見。
//
// 【5. to_string 的限制】
//   - 內部以 sprintf-style 實作,locale 受設定影響。
//     例如德語 locale 的小數點是 ','。
//   - 對 double 預設精度 6,可能輸出 "0.100000",且不一定是 round-trip 安全。
//   - 想要 round-trip + locale 中立 → 請改用 std::to_chars。
//
// 【6. C++17 std::to_chars / from_chars (in <charconv>)】
//   - to_chars(char* first, char* last, T value)        → 數值 → 字串
//   - from_chars(const char* first, const char* last, T& value) → 字串 → 數值
//   - 特性:
//       (1) 不會 throw,改用 std::errc 回報錯誤
//       (2) Locale-free (不受 std::locale 影響)
//       (3) Round-trip safe (對浮點數,from_chars(to_chars(x)) == x)
//       (4) 比 stoi/sprintf 快好幾倍
//   - 缺點:浮點 from_chars/to_chars 在某些舊版 libstdc++ (gcc < 11) 沒實作完整。
//
// 【7. 與 C 風格 atoi 的比較】
//
//   性質           atoi         stoi         from_chars
//   ──────────────  ────────────  ────────────  ────────────
//   錯誤回報        無 (silent 0)  throw 例外    std::errc 結構
//   超出範圍        UB            throw         errc::result_out_of_range
//   base            10 only       任意 2..36    任意 2..36
//   locale          受影響        受影響        中立
//   效能            最快但不安全  慢 (例外開銷) 最快且安全
//   建議            不要用        日常使用      效能熱路徑
//
// 【8. 浮點數精度的小坑】
//   - to_string(0.1 + 0.2) 給你 "0.300000" 而不是 "0.3"。
//   - to_string(1e20) 可能變成 "100000000000000000000.000000"。
//   - 解法:用 ostringstream + std::setprecision、std::format (C++20)、或 to_chars。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 【為何 stoi 會 throw,而 atoi 不會?】
//   - atoi 是 C 庫函式,設計於 1970s,「失敗就回 0」── 看不出 0 是合法解析還是錯誤。
//   - stoi 是 C++11 新加,延續 STL 設計哲學:「錯誤 → 例外」,程式邏輯更明確。
//
// 【invalid_argument vs out_of_range】
//   - "abc"     → invalid_argument   (一個合法數字字元都沒有)
//   - "999999999999" (對 int) → out_of_range (合法但太大)
//   - "12abc"   → 不丟例外,回傳 12,pos = 2
//
// 【pos 參數的進階應用:增量解析器 (incremental parser)】
//   IP 解析示意:
//     string ip = "192.168.0.1";
//     size_t i = 0;
//     for (int k = 0; k < 4; ++k) {
//         size_t p;
//         int n = stoi(ip.substr(i), &p);
//         // ... 處理 n ...
//         i += p;
//         if (k < 3) ++i;   // skip '.'
//     }
//
// 【from_chars 的「成功 + 部分消化」模式】
//   auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
//   if (ec == std::errc{}) {
//       // 成功;ptr 指向「第一個未消化字元」
//       size_t consumed = ptr - str.data();
//   }
//   ── 比 stoi 的 pos 參數設計更乾淨,且零分配。
//
// 【to_string 與國際化】
//   - to_string(3.14) 在德文 locale 下不會給 "3,14",因為它呼叫 C-locale 的 sprintf。
//     但 stoi("3,14") 會!也就是 to_string 與 stoi 並非完美對稱。
//   - 走 to_chars / from_chars 完全避開 locale 噩夢。
//
// 【為何 from_chars 對浮點數那麼快?】
//   - C++17/20 採用 Ryu / Grisu 等近代演算法,比 sprintf 系列快 3~10 倍,且 round-trip 安全。
//   - 在 JSON serializer、metric 系統等熱路徑中,從 sprintf 換到 to_chars 是常見優化。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. stoi 會丟例外!不像 atoi 是 silent 失敗。要寫 try/catch。
// 2. base 預設 10;傳 0 自動偵測 (看前綴 0x / 0)。
// 3. 浮點 to_string 精度有限,需更高精度建議用 std::format (C++20) 或 std::to_chars。
// 4. C++17 std::from_chars 不丟例外,效能高,建議在效能熱路徑使用。
// 5. stoi("12abc") 不會丟例外 (回傳 12),要靠 pos 判斷是否完整解析完。
// 6. stoi 的 base 行為:傳 8 但前綴是 "0x" 會 throw invalid_argument。
//
// =============================================================================

/*
補充筆記：std::string::numeric_conversions
  - std::string::numeric_conversions 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::numeric_conversions 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】numeric_conversions (stoi / stod / to_string)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::stoi 轉換失敗時會怎樣？跟 C 的 atoi 差在哪？
//     答：stoi / stol / stod 系列（C++11）在完全無法轉換時拋 std::invalid_argument，
//         數值超出目標型別範圍時拋 std::out_of_range，失敗是「可偵測」的。
//         atoi 失敗只回 0，無法和「輸入真的是 "0"」區分；且 atoi 的溢位是 UB。
//     追問：那要怎麼做嚴格驗證？→ 用下面 Q2 的 pos 參數，或改用 <charconv> 的 from_chars。
//
// 🔥 Q2. stoi 的第二個參數 pos 有什麼用？
//     答：pos 會被寫入「解析停止的位置」，也就是實際消耗掉的字元數。
//         想確認整個字串都被吃完（而不是只吃了前綴），就檢查 pos == s.size()。
//     追問：stoi 會跳過前導空白嗎？→ 會，前導空白被略過，不算轉換失敗。
//
// ⚠️ 陷阱. std::stoi("42abc") 會拋例外嗎？
//     答：不會，回傳 42。stoi 只解析「合法的前綴」，遇到 'a' 就停下並成功回傳。
//         只有「連一個合法字元都沒有」才拋 invalid_argument（例如 stoi("abc")）。
//     為什麼會錯：多數人把 stoi 想成「整串驗證器」，但它規格上就是 prefix parser，
//         這不是 bug 是規格。要嚴格驗證必須自己檢查 pos，或用 from_chars 檢查 ptr。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <stdexcept>
#include <charconv>
#include <sstream>
#include <algorithm>

void demoConversions() {
    // 字串 → 數值
    std::string a = "42";
    int x = std::stoi(a);
    std::cout << "stoi(\"42\") = " << x << "\n";

    // pos 用法
    std::string b = "  123abc";
    size_t pos = 0;
    int y = std::stoi(b, &pos);
    std::cout << "stoi -> " << y << ", pos stopped at " << pos << " ('" << b[pos] << "')\n";

    // 16 進位
    int z = std::stoi("0xFF", nullptr, 16);
    std::cout << "stoi(\"0xFF\",,16) = " << z << "\n";

    // 失敗範例
    try {
        std::stoi("abc");
    } catch (const std::invalid_argument& e) {
        std::cout << "invalid_argument: " << e.what() << "\n";
    }

    // 溢位範例
    try {
        std::stoi("99999999999999999999");
    } catch (const std::out_of_range& e) {
        std::cout << "out_of_range: " << e.what() << "\n";
    }

    // 數值 → 字串
    std::cout << "to_string(3.14) = " << std::to_string(3.14) << "\n";    // "3.140000"
    std::cout << "to_string(-7)   = " << std::to_string(-7)   << "\n";

    // C++17 from_chars (不丟例外、locale 中立、效能最佳)
    int v;
    std::string c = "9999";
    auto [ptr, ec] = std::from_chars(c.data(), c.data() + c.size(), v);
    if (ec == std::errc()) std::cout << "from_chars -> " << v << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 8. String to Integer (atoi)】
// 題目: 實作簡化版 atoi。
// 為何用 stoi: 標準函式直接處理大部分邊界,我們僅做溢位 clamp。
// -----------------------------------------------------------------------------
#include <climits>
int myAtoi(const std::string& s) {
    size_t i = 0;
    while (i < s.size() && s[i] == ' ') ++i;
    if (i == s.size()) return 0;

    // 取出可解析片段
    size_t start = i;
    if (s[i] == '+' || s[i] == '-') ++i;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
    std::string num = s.substr(start, i - start);
    if (num.empty() || num == "+" || num == "-") return 0;

    try {
        long v = std::stol(num);
        if (v > INT_MAX) return INT_MAX;
        if (v < INT_MIN) return INT_MIN;
        return static_cast<int>(v);
    } catch (const std::out_of_range&) {
        return num[0] == '-' ? INT_MIN : INT_MAX;
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 412. Fizz Buzz】
// 題目: 1..n 列出數字,3 倍數印 "Fizz",5 倍數印 "Buzz",兩者皆是印 "FizzBuzz"。
// 為何用 to_string: 把整數轉成字串塞進結果 vector,這是 to_string 最典型的用法。
// -----------------------------------------------------------------------------
#include <vector>
std::vector<std::string> fizzBuzz(int n) {
    std::vector<std::string> out;
    out.reserve(n);
    for (int i = 1; i <= n; ++i) {
        if (i % 15 == 0)      out.push_back("FizzBuzz");
        else if (i % 3 == 0)  out.push_back("Fizz");
        else if (i % 5 == 0)  out.push_back("Buzz");
        else                  out.push_back(std::to_string(i));   // int → string
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【實務範例】用 to_string 組 IP
// (簡單示範:LeetCode 1108 Defang IP 也會用到)
// -----------------------------------------------------------------------------
std::string buildIP(int a, int b, int c, int d) {
    return std::to_string(a) + "." +
           std::to_string(b) + "." +
           std::to_string(c) + "." +
           std::to_string(d);
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】把 byte 數轉為人類可讀格式 (1024 -> "1.00 KB")
// 為何用 to_string + 數值: 顯示檔案大小、傳輸量、記憶體用量,UI / CLI 都會用。
// -----------------------------------------------------------------------------
std::string formatBytes(unsigned long long bytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int idx = 0;
    double value = static_cast<double>(bytes);
    while (value >= 1024.0 && idx < 5) { value /= 1024.0; ++idx; }

    // 用 ostringstream 格式化兩位小數;比 to_string(double) 更可控
    std::ostringstream os;
    os.setf(std::ios::fixed);
    os.precision(2);
    os << value << ' ' << units[idx];
    return os.str();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】解析 HTTP Content-Length / 環境變數轉型
// 為何用 stoull / stoi: HTTP header 與環境變數本質都是字串,
//                       後端服務必須安全地把它們轉成數值,有錯就明確 throw。
// -----------------------------------------------------------------------------
unsigned long long parseContentLength(const std::string& headerValue) {
    try {
        unsigned long long len = std::stoull(headerValue);
        return len;
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Bad Content-Length header: " + headerValue);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Content-Length too large: " + headerValue);
    }
}

int parseEnvInt(const char* envValue, int fallback) {
    if (!envValue) return fallback;                          // 環境變數未設定
    try {
        size_t pos;
        int v = std::stoi(envValue, &pos);
        // 確保整段字串都是數字 (避免 "42abc" 被當 42)
        while (envValue[pos] == ' ') ++pos;
        if (envValue[pos] != '\0') return fallback;
        return v;
    } catch (...) {
        return fallback;
    }
}

#include <cctype>
int main() {
    demoConversions();
    std::cout << "\n=== LeetCode 8 ===\n";
    std::cout << myAtoi("   -42")           << "\n";
    std::cout << myAtoi("4193 with words")  << "\n";
    std::cout << myAtoi("-91283472332")     << "\n";

    std::cout << "\n=== LeetCode 412: Fizz Buzz ===\n";
    auto fb = fizzBuzz(15);
    for (auto& s : fb) std::cout << s << " ";
    std::cout << "\n";

    std::cout << "\n=== buildIP ===\n";
    std::cout << buildIP(192, 168, 0, 1) << "\n";

    std::cout << "\n=== 日常實務: 格式化 bytes ===\n";
    std::cout << formatBytes(512)             << "\n";    // 512.00 B
    std::cout << formatBytes(1024)            << "\n";    // 1.00 KB
    std::cout << formatBytes(1500000)         << "\n";    // 1.43 MB
    std::cout << formatBytes(15000000000ULL)  << "\n";    // 13.97 GB

    std::cout << "\n=== 日常實務: HTTP Content-Length ===\n";
    std::cout << parseContentLength("1024") << "\n";

    std::cout << "\n=== 日常實務: 環境變數轉型 ===\n";
    std::cout << "TIMEOUT=" << parseEnvInt("30", 60) << "\n";       // 30
    std::cout << "TIMEOUT=" << parseEnvInt("oops", 60) << "\n";     // 60 (fallback)
    std::cout << "TIMEOUT=" << parseEnvInt(nullptr, 60) << "\n";    // 60 (未設定)

    // -----------------------------------------------------------------------------
    // 【LeetCode 範例 (補充)】LeetCode 7. Reverse Integer
    // 題目: 整數逆向各位數字,結果若溢位 int 回 0。
    // 為何用 to_string/stoi: 把數字轉字串、reverse 再轉回,展示「字串作為計算媒介」。
    // 注意: 處理負號需特別小心,反轉時負號要保持在最前。
    // -----------------------------------------------------------------------------
    auto reverseInt = [](int x) -> int {
        bool neg = x < 0;
        std::string s = std::to_string(neg ? -static_cast<long long>(x) : x);
        std::reverse(s.begin(), s.end());
        try {
            long long v = std::stoll(s);
            if (neg) v = -v;
            if (v < INT_MIN || v > INT_MAX) return 0;
            return static_cast<int>(v);
        } catch (...) {
            return 0;
        }
    };
    std::cout << "\n=== LeetCode 7 ===\n";
    std::cout << reverseInt(123)    << "\n";    // 321
    std::cout << reverseInt(-123)   << "\n";    // -321
    std::cout << reverseInt(120)    << "\n";    // 21

    // -----------------------------------------------------------------------------
    // 【日常實務範例 (補充)】UI 顯示倒數時間 (例如 "00:30s")
    // 為何用 to_string: 把秒數轉字串再 pad。
    // -----------------------------------------------------------------------------
    auto formatCountdown = [](int secs) {
        if (secs < 0) secs = 0;
        std::string s = std::to_string(secs);
        if (s.size() < 2) s = "0" + s;
        return "00:" + s + "s";
    };
    std::cout << "\n=== 日常實務: countdown ===\n";
    std::cout << formatCountdown(5)  << "\n";   // 00:05s
    std::cout << formatCountdown(45) << "\n";   // 00:45s

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:stoi("12abc") 會丟例外嗎?如何判斷「整段字串是不是純數字」?
    //    A:不會,回傳 12 而不丟。要驗證完整解析需傳 size_t* pos,呼叫後
    //       檢查 pos == s.size() (或剩下的全是空白)。stoi("abc") 才會丟
    //       invalid_argument;stoi("9999999999999") 對 int 太大丟 out_of_range。
    //
    //  Q2:to_string(double) 跟 std::to_chars 在浮點數的差別是什麼?
    //    A:to_string 內部用 sprintf,locale-aware 且預設精度 6 位,可能輸出
    //       "0.100000" 且不一定 round-trip safe (to_string 出去再 stod 回來
    //       不保證等值)。to_chars (C++17) 用 Ryu/Grisu 算法,locale-free、
    //       round-trip safe、不丟例外、效能高 3~10 倍 — 熱路徑首選。
    //
    //  Q3:from_chars 跟 stoi 在錯誤回報上差在哪?
    //    A:from_chars 不丟例外,回傳 {ptr, errc} pair (errc::invalid_argument
    //       / errc::result_out_of_range),且零分配 (不建臨時 string);
    //       stoi 走例外慢 (但 try/catch 在沒 throw 時近乎零成本)。
    //       對效能敏感、locale-中立的需求一律走 from_chars。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra numeric_conversions.cpp -o numeric_conversions

// === 預期輸出 (節錄) ===
// === LeetCode 7 ===
// 321
// -321
// 21
// === 日常實務: countdown ===
// 00:05s
// 00:45s
