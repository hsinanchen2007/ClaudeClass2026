/*
 * <charconv>：無 locale、低配置的數字與字元範圍轉換
 *
 * from_chars(first,last,value[,base]) 不需 NUL、不中斷於 string 邊界之外、不丟例外；
 * 回傳 `{ptr, ec}`。ec==invalid_argument 表示起點無數字，result_out_of_range 表示溢位。
 * 成功但 ptr!=last 代表只有前綴被解析。整數 base 的合法範圍固定是 2..36；超出範圍
 * 是呼叫前置條件違反，不會以 error code 回報，所以 runtime base 必須先自行驗證。
 * to_chars 同樣回 ptr/ec，caller 管理 buffer。
 * 整數支援自 C++17；浮點支援與品質應依目標標準庫驗證。
 */

#include <array>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace {

// 不使用 assert，讓測試在 -DNDEBUG 建置中仍會執行。
void expect(const bool condition) {
    if (!condition) std::abort();
}

std::optional<int> parse_int_exact(const std::string_view text, const int base = 10) {
    if (base < 2 || base > 36 || text.empty()) return std::nullopt;

    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (error != std::errc{} || end != text.data() + text.size()) return std::nullopt;
    return value;
}

std::string int_to_string(const int value) {
    std::array<char, 32U> buffer{};
    const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    expect(error == std::errc{});
    return std::string(buffer.data(), end);
}

void basic_demo() {
    expect(parse_int_exact("2048").value() == 2048);
    expect(parse_int_exact("ff", 16).value() == 255);
    expect(!parse_int_exact("12px").has_value());
    expect(!parse_int_exact("").has_value());

    // 邊界測試：1、37 必須在呼叫 from_chars 前拒絕；2、36 則可正常解析。
    expect(!parse_int_exact("10", 1).has_value());
    expect(parse_int_exact("101010", 2).value() == 42);
    expect(parse_int_exact("z", 36).value() == 35);
    expect(!parse_int_exact("10", 37).has_value());

    expect(int_to_string(-42) == "-42");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 8. String to Integer (atoi)（字串轉整數，嚴格 token 變形）
// 題目：原題要略過前導空白、讀可選正負號與數字並 clamp；本例只測完整十進位 token，
//       "123" 得 123，而 "12x" 直接視為失敗並回 0，並非原題完整提交。
// 為何使用本章主題：from_chars 可在 string_view 範圍內無配置、無 locale、無例外地解析；
//       parse_int_exact 同時檢查 ec 與 ptr==end，刻意拒絕合法前綴後的垃圾。
// 思路：1. 驗證非空與 base；2. 解析整段；3. 任一錯誤或未完整消耗回 nullopt；4. 映射為值或 0。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 token 長度。
// 易錯點：這不實作原題空白與 clamp 規則；回 0 會混合合法 "0" 與錯誤，正式 API 應保留 optional。
// -----------------------------------------------------------------------------
int leetcode_strict_atoi(const std::string_view text) {
    const auto parsed = parse_int_exact(text);
    return parsed.value_or(0);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】TCP 連接埠設定解析
// 情境：設定值必須是完整十進位字串且位於 1..65535；"443" 合法，"0"、"80/tcp" 不合法。
// 為何使用本章主題：from_chars 不受 locale 影響且直接回錯誤位置，比 stoi 的例外與部分解析
//       更適合高頻設定驗證，也不需先建立 NUL 結尾副本。
// 設計：1. 解析成較寬 unsigned int；2. 同時檢查 ec 與完整消耗；3. 驗證領域範圍後才窄化。
// 成本：時間 O(N)、額外空間 O(1)，N 是 text 長度，沒有動態配置。
// 上線注意：空白與 '+' 不會自動接受；錯誤訊息若需區分語法、溢位、範圍，不能只回 nullopt。
// -----------------------------------------------------------------------------
std::optional<unsigned short> practical_parse_port(const std::string_view text) {
    unsigned int value = 0U;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value == 0U ||
        value > 65535U) {
        return std::nullopt;
    }
    return static_cast<unsigned short>(value);
}

}  // namespace

int main() {
    basic_demo();
    expect(leetcode_strict_atoi("123") == 123);
    expect(leetcode_strict_atoi("12x") == 0);
    expect(practical_parse_port("443").value() == 443U);
    expect(!practical_parse_port("0").has_value());
    expect(!practical_parse_port("65536").has_value());
    expect(!practical_parse_port("80/tcp").has_value());
    std::cout << "charconv: tests passed\n";
}

/*
 * 【與 stoi 比較】charconv 不配置、不依 locale、不丟例外，且可直接吃 string_view 範圍；
 * 但 API 較低階，空白與 '+' 等接受規則要看規格，不會自動略過一般空白。
 * 【陷阱】只檢查 ec 仍會接受合法數字前綴；嚴格 token 必須再檢查 ptr==last。
 * runtime base 也不能直接轉交：先驗證 2<=base<=36，因為超界不是可恢復的解析錯誤。
 * 【面試】為何一定檢查 ptr？ec 成功只代表有合法前綴；"123abc" 仍可能解析出 123。
 * 【練習】用 to_chars(base=16) 輸出無前綴的小寫十六進位，再自行加 "0x"。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'charconv.cpp' -o '/tmp/codex_cpp_C_String_charconv' && '/tmp/codex_cpp_C_String_charconv'
//
// === 預期輸出（節錄）===
// charconv: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
