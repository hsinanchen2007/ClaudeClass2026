// =============================================================================
// 檔名: charconv.cpp
// 主題: <charconv> std::to_chars / std::from_chars (C++17)
// 參考連結 References:
//   cppreference (to_chars):   https://en.cppreference.com/cpp/utility/to_chars
//   cppreference (from_chars): https://en.cppreference.com/cpp/utility/from_chars
//   cplusplus.com: https://cplusplus.com/reference/charconv/
// =============================================================================
//
// 【函式資訊 Information】
//   namespace std {
//     // ─── 整數 ↔ 字串 ───
//     std::to_chars_result   to_chars(char* first, char* last,
//                                     IntegerT value, int base = 10);
//     std::from_chars_result from_chars(const char* first, const char* last,
//                                       IntegerT& value, int base = 10);
//
//     // ─── 浮點 ↔ 字串 ───
//     std::to_chars_result   to_chars(char* first, char* last, FloatT value);
//     std::to_chars_result   to_chars(char* first, char* last, FloatT value,
//                                     std::chars_format fmt);
//     std::to_chars_result   to_chars(char* first, char* last, FloatT value,
//                                     std::chars_format fmt, int precision);
//     std::from_chars_result from_chars(const char* first, const char* last,
//                                       FloatT& value,
//                                       std::chars_format fmt = chars_format::general);
//   }
//
//   struct to_chars_result   { char*       ptr; std::errc ec; };
//   struct from_chars_result { const char* ptr; std::errc ec; };
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼 C++17 才加 charconv?
// ----------------------------------------------------------------------------
// 在 C++17 之前,字串 ↔ 數字轉換有三套工具:
//   - C 的 atoi / strtod / sprintf:不安全 (緩衝區漏)、依賴 locale、慢。
//   - C++ 的 stringstream:強型別但極度肥大、locale 依賴、配置 string、慢。
//   - std::stoi / std::to_string:會 throw,go through locale,仍非最快。
//
// 在大流量服務 (JSON parser、log 解析、CSV ETL、報表系統) 中,數字解析
// 與序列化是熱路徑;舊工具在 benchmark 中常常成為瓶頸。
// charconv 為此而生,目標是:
//   1. **絕對 locale-independent** (不受 setlocale 影響)。
//   2. **不配置記憶體** (使用者提供 buffer)。
//   3. **不丟例外** (用 errc 回報錯誤)。
//   4. **常數時間/精準的 round-trip** (浮點 to_chars 保證最短可 round-trip
//      之十進位表示)。
//   5. **比 sprintf / strtod 快數倍** (libstdc++ 用 Ryu / Eisel-Lemire 演算法)。
//
// (二) to_chars / from_chars 的回傳值
// ----------------------------------------------------------------------------
//   to_chars_result:
//     ptr — 指向「最後寫入字元的下一個位置」(buffer 內);失敗時不指定。
//     ec  — std::errc{} 表成功;std::errc::value_too_large 表 buffer 不足。
//
//   from_chars_result:
//     ptr — 指向「停止解析的位置」(可能是非數字字元);失敗時等於 first。
//     ec  — std::errc{} 表成功;std::errc::invalid_argument 表完全無法解析;
//           std::errc::result_out_of_range 表數值超出型別範圍。
//
// 注意:from_chars **不會 skip leading whitespace**!不像 strtol/scanf
// 會吃掉前置空白,from_chars 認真要求 first 就直接是數字字元。
// 這是設計上「明確 > 寬鬆」的選擇,避免無聲行為。
//
// (三) 與 std::stoi / std::to_string 的差異
// ----------------------------------------------------------------------------
//   面向                | stoi/to_string         | from_chars/to_chars
//   --------------------|------------------------|-----------------------
//   錯誤回報            | throw exception        | errc (no throw)
//   配置 std::string    | to_string 會           | 不會 (你提供 buffer)
//   locale             | 依賴目前 locale       | 永遠 C locale
//   解析 leading WS    | 會 skip                | 不會
//   浮點精度           | 預設 6 位數失真       | 可保證 short round-trip
//   速度              | 慢                     | 快 (Ryu / 手工最佳化)
//
// (四) chars_format 的選項 (浮點)
// ----------------------------------------------------------------------------
//   std::chars_format::scientific   : 1.234e+05
//   std::chars_format::fixed        : 123400.5
//   std::chars_format::hex          : 1.f4p+10  (16 進位浮點)
//   std::chars_format::general      : 自動選 fixed 或 scientific (預設)
// 不指定 precision 時,to_chars 會用「最少可正確 round-trip 的位數」。
// 這是 IEEE-754 → 十進位最佳化文章 (Ryu 演算法) 的標準應用。
//
// (五) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++17    : 加入 to_chars/from_chars 的整數版本與浮點版本。
//   C++20    : constexpr 可在常量計算上下文使用。
//   C++23    : 修正細節 (對 NaN、Infinity 的格式)。
//   實作支援: GCC 11+ 起整數 + 浮點完整;Clang 14+;MSVC 19.14+。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Round-Trip 保證
//    對任何 double d,序列 (d → to_chars → from_chars) 之後拿到的
//    double 與原 d 「位元等同」(bit-exact)。這是 charconv 對浮點唯一
//    嚴格保證的性質。也是 IEEE-754 規範自身的要求。
//
// 2) Ryu 演算法
//    Adams 的 Ryu (2018 PLDI) 找到「最短可 round-trip 的十進位字串」,
//    速度比舊式 grisu / dragon4 快數倍。libstdc++ / MSVC STL 都採用。
//
// 3) Buffer 大小估計
//    整數的最大長度:
//      - int32_t:11 (含負號)
//      - int64_t:20
//      - uint64_t:20 (no 負號) 或 base=2 的 64
//    浮點:
//      - double 在 general 格式至多約 24 字元;保險起見開 32 都安全。
//    若不確定,寫 64 byte buffer 不會錯。
//
// 4) constexpr 用途
//    在 C++23 起,from_chars / to_chars 可在 constexpr 評估中使用,
//    很適合「編譯期表格生成」(例如把 enum 對應到字串)。
//
// 5) charconv 不處理 thousands-separator
//    "1,000,000" 這種千分位逗號不會被 from_chars 解析。
//    如果你要解析「使用者格式」的數字,要先自己去掉逗號或改用
//    std::stoi (會走 locale)。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. from_chars 不會 skip leading whitespace。
// 2. 失敗時要先檢查 result.ec 而不是看 value;value 在失敗時保持原值。
// 3. 寫入時 to_chars 不會自動補 '\0';你要的話自己在 result.ptr 塞。
// 4. buffer 不夠大,to_chars 不會寫部分結果就回 value_too_large。
// 5. base=8 / 16 對整數 OK,但不接受 0x / 0o 前綴;前綴要自己消掉再傳。
// 6. 浮點的 to_chars 在不指定 precision 時,選的是「最短可 round-trip」,
//    這跟 printf("%g") 不一樣 —— "%g" 預設 6 位數,可能失真。
//
// =============================================================================

/*
補充筆記：std::from_chars / std::to_chars
  - std::from_chars / std::to_chars 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::from_chars / std::to_chars 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】charconv (from_chars / to_chars)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::from_chars 是哪個標準加入的？為什麼說它是最快的轉換方式？
//     答：C++17 的 <charconv>。它不拋例外（用回傳的 from_chars_result 帶 errc）、
//         不做記憶體配置、也完全不受 locale 影響，因此沒有 stringstream 的
//         virtual dispatch 與 locale 查詢成本，也沒有例外機制的開銷。
//     追問：失敗怎麼判斷？→ 檢查 result.ec：errc::invalid_argument 表示無法解析、
//           errc::result_out_of_range 表示溢位；成功時 ec 是預設值 errc{}。
//
// 🔥 Q2. from_chars 怎麼做到「整串必須合法」的嚴格驗證？
//     答：from_chars_result 除了 ec 還有 ptr，指向「解析停止的位置」。
//         要同時檢查 ec == std::errc{} 且 ptr == last，才代表整個範圍都被消耗完。
//         只看 ec 的話，"42abc" 一樣會成功回傳 42（與 stoi 的前綴語意相同）。
//     追問：to_chars 失敗呢？→ buffer 不夠時 ec 為 errc::value_too_large，
//           且此時 [first, last) 的內容是未指定的，不可拿來用。
//
// ⚠️ 陷阱. from_chars 會像 stoi 一樣跳過前導空白嗎？
//     答：不會。from_chars 不接受任何前導空白，" 42" 直接回 errc::invalid_argument；
//         整數版本連正號 "+42" 也不接受（只認負號）。
//     為什麼會錯：把 from_chars 當成 stoi 的 drop-in 替代品，忘了它是刻意設計成
//         「最小、無驚喜」的低階原語，所有寬鬆行為都要呼叫端自己補。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <string_view>
#include <charconv>
#include <system_error>
#include <array>
#include <cmath>
#include <vector>

void demoIntToChars() {
    std::cout << "=== to_chars (int) ===\n";
    std::array<char, 32> buf{};
    int v = -123456;
    auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), v);
    if (ec == std::errc{}) {
        std::cout << "value " << v << " → \""
                  << std::string_view(buf.data(), ptr - buf.data()) << "\"\n";
    }

    // base = 16
    auto [ptr2, ec2] = std::to_chars(buf.data(), buf.data() + buf.size(), 0xCAFE, 16);
    if (ec2 == std::errc{}) {
        std::cout << "value 0xCAFE (base 16) → \""
                  << std::string_view(buf.data(), ptr2 - buf.data()) << "\"\n";
    }
}

void demoIntFromChars() {
    std::cout << "\n=== from_chars (int) ===\n";
    std::string_view s = "42abc";
    int v = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec == std::errc{}) {
        std::cout << "parsed = " << v
                  << ", 剩餘=\"" << std::string_view(ptr, s.end() - ptr)
                  << "\"\n";
    }

    // 失敗:invalid_argument
    std::string_view bad = "xyz";
    int x = -1;
    auto [p2, ec2] = std::from_chars(bad.data(), bad.data() + bad.size(), x);
    std::cout << "parse \"xyz\" → ec=invalid_argument? "
              << std::boolalpha
              << (ec2 == std::errc::invalid_argument)
              << ", x 維持 " << x << "\n";

    // 失敗:result_out_of_range (超出 int 範圍)
    std::string_view huge = "999999999999999999999";
    int y = 0;
    auto [p3, ec3] = std::from_chars(huge.data(), huge.data() + huge.size(), y);
    std::cout << "parse 過大 → ec=result_out_of_range? "
              << (ec3 == std::errc::result_out_of_range) << "\n";
}

void demoDouble() {
    std::cout << "\n=== to_chars / from_chars (double) ===\n";
    std::array<char, 64> buf{};
    double d = 3.14159265358979;
    auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), d);
    std::cout << "double → \""
              << std::string_view(buf.data(), ptr - buf.data()) << "\"\n";

    double back = 0;
    [[maybe_unused]] auto [p2, ec2] = std::from_chars(buf.data(), ptr, back);
    std::cout << "round-trip back = " << back
              << ", bit-exact? " << std::boolalpha << (back == d) << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 8. String to Integer (atoi) (Medium)
//
// 題目敘述:
//   實作 atoi:給字串 s,跳過前置空白、處理 +/- 號,讀取連續數字成 int;
//   超出範圍要 clamp 到 INT_MIN / INT_MAX。
//   範例: "42"            → 42
//        "   -42"        → -42
//        "4193 with words" → 4193
//        "words and 987" → 0
//        "-91283472332"  → INT_MIN (clamp)
//
// 為何用 from_chars:
//   範本實作通常自己寫迴圈,但「字串轉整數」與「處理 overflow clamp」
//   是 from_chars 天生強項 (errc::result_out_of_range)。
//   注意一點:from_chars 不會 skip whitespace,需要自己先處理前置空白
//   與 +/- 號;跳過後直接交給 from_chars。
//
// 解題思路:
//   1. 跳前置空白;
//   2. 處理 sign;
//   3. 餘下交給 from_chars;
//   4. 若回 result_out_of_range,依 sign clamp 到 INT_MIN / INT_MAX。
//
// 複雜度: 時間 O(n),空間 O(1)
// -----------------------------------------------------------------------------
#include <climits>
int myAtoi(const std::string& s) {
    size_t i = 0;
    while (i < s.size() && s[i] == ' ') ++i;
    bool neg = false;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
        neg = (s[i] == '-');
        ++i;
    }

    int v = 0;
    auto first = s.data() + i;
    auto last  = s.data() + s.size();
    auto [ptr, ec] = std::from_chars(first, last, v);

    if (ec == std::errc::invalid_argument) return 0;
    if (ec == std::errc::result_out_of_range)
        return neg ? INT_MIN : INT_MAX;
    return neg ? -v : v;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】Log line 中數字欄位的零配置解析
//
// 為何用 from_chars:
//   後端結構化 log (e.g. "ts=1714912345 status=200 ms=42.78") 解析時,
//   每行可能有多個數字欄位;若用 stoi/stof 每一個都要額外配置 string、
//   走 locale,熱路徑代價高。
//
//   from_chars 直接吃 string_view (用 .data() / .data()+size()),全程
//   零配置、零 throw、不走 locale,是 high-throughput log 解析的必修招式。
// -----------------------------------------------------------------------------
struct LogFields {
    long ts = 0;
    int  status = 0;
    double ms = 0.0;
};

bool parseLogLine(std::string_view line, LogFields& out) {
    // 簡化:只處理 "ts=... status=... ms=..." 三欄 (位置不固定)
    auto extract = [&](std::string_view key, auto& dest) {
        auto pos = line.find(key);
        if (pos == std::string_view::npos) return false;
        const char* p = line.data() + pos + key.size();
        const char* end = line.data() + line.size();
        auto [ptr, ec] = std::from_chars(p, end, dest);
        return ec == std::errc{};
    };
    return extract("ts=", out.ts)
        && extract("status=", out.status)
        && extract("ms=", out.ms);
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1313. Decompress Run-Length Encoded List
// 題目: 給 nums (偶數長度),pair (freq, val) 表示 val 重複 freq 次。
// 為何用 charconv: 此題本身是 vector 不是 string,但常見變奏是「給字串
//                  '3a2b' 解碼為 'aaabb'」。我們從字串裡用 from_chars 取
//                  數字,效能比 stoi 高且不丟例外。
// 解題: 掃描字串,遇到數字 → from_chars 抓 freq;接下來字元就是 val,重複 freq 次。
// 複雜度: O(總長度)。
// 難度: easy
// -----------------------------------------------------------------------------
std::string decodeRLE(const std::string& s) {
    std::string out;
    const char* p = s.data();
    const char* end = p + s.size();
    while (p < end) {
        int freq = 0;
        auto [next, ec] = std::from_chars(p, end, freq);
        if (ec != std::errc{} || next >= end) break;
        char ch = *next;
        out.append(static_cast<size_t>(freq), ch);
        p = next + 1;
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把整數陣列序列化為逗號分隔字串 (CSV 一行)
// 為何用 charconv: 大量整數轉字串時,to_chars 比 to_string 快 3~5 倍 (免配置、
//                  無 locale),適合 ETL pipeline 與 log 匯出。
// -----------------------------------------------------------------------------
std::string intsToCsv(const std::vector<int>& xs) {
    std::string out;
    out.reserve(xs.size() * 4);
    char buf[16];
    for (size_t i = 0; i < xs.size(); ++i) {
        if (i > 0) out.push_back(',');
        auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), xs[i]);
        if (ec == std::errc{}) out.append(buf, ptr);
    }
    return out;
}

int main() {
    demoIntToChars();
    demoIntFromChars();
    demoDouble();

    std::cout << "\n=== LeetCode 8 ===\n";
    std::cout << myAtoi("42") << "\n";                    // 42
    std::cout << myAtoi("   -42") << "\n";                // -42
    std::cout << myAtoi("4193 with words") << "\n";       // 4193
    std::cout << myAtoi("words and 987") << "\n";         // 0
    std::cout << myAtoi("-91283472332") << "\n";          // INT_MIN

    std::cout << "\n=== LeetCode 1313 變奏 (decodeRLE) ===\n";
    std::cout << decodeRLE("3a2b1c") << "\n";   // aaabbc
    std::cout << decodeRLE("5x")     << "\n";   // xxxxx

    std::cout << "\n=== 日常實務: log line 解析 ===\n";
    LogFields f{};
    if (parseLogLine("ts=1714912345 status=200 ms=42.78", f)) {
        std::cout << "ts=" << f.ts
                  << " status=" << f.status
                  << " ms=" << f.ms << "\n";
    }

    std::cout << "\n=== 日常實務: intsToCsv ===\n";
    std::cout << intsToCsv({1, 2, 30, 400, 5000}) << "\n";    // 1,2,30,400,5000

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：to_chars 比 std::to_string 或 sprintf 快多少?為什麼?
    //    A：實測通常快 2~10 倍 (浮點甚至更多)。原因:(1) 不配置 std::string,
    //       使用者提供 buffer;(2) 完全 locale-independent,跳過 locale 查表;
    //       (3) libstdc++ 浮點實作 Ryu/Eisel-Lemire 演算法比 sprintf 的
    //       grisu 快;(4) 不 throw,沒有 EH 開銷。熱路徑 ETL/JSON 必選。
    //
    //  Q2：from_chars 處理失敗如何回報?與 stoi 的差異?
    //    A：from_chars 完全不 throw。透過 from_chars_result.ec (std::errc)
    //       回報:errc{} 是成功;errc::invalid_argument 表開頭就不是數字
    //       (此時 ptr == first);errc::result_out_of_range 表數字過大溢位。
    //       stoi 反之會 throw invalid_argument 或 out_of_range,並走 locale。
    //
    //  Q3：to_chars 的浮點精度保證是什麼?
    //    A：預設 (不指定 precision) 保證「最短可 round-trip 之十進位表示」──
    //       to_chars 寫出來再 from_chars 回來必定 bit-exact 相等。這是 sprintf
    //       做不到的 (要寫 %.17g 才能保證 round-trip 但會多餘字元)。指定
    //       precision 時則退化為固定位數模式。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra charconv.cpp -o charconv

// === 預期輸出 (節錄) ===
// === LeetCode 1313 變奏 (decodeRLE) ===
// aaabbc
// xxxxx
// === 日常實務: intsToCsv ===
// 1,2,30,400,5000
