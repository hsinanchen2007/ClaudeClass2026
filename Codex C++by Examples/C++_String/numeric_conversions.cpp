/*
 * std::stoi/stol/stoll/stoul/stoull/stof/stod/stold 與 std::to_string
 *
 * 這組高階 API 接收 owning string、可略過前導空白、可用 idx 回報解析停止位置；
 * 無轉換時丟 invalid_argument，超出目標型別範圍丟 out_of_range。它們可能受 C locale
 * 影響且以例外報錯。設定檔一次性解析很方便；高頻/零配置解析可選 from_chars。
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

void basic_demo() {
    std::size_t consumed = 0U;
    const int value = std::stoi("  -42px", &consumed, 10);
    assert(value == -42);
    assert(consumed == 5U);  // 兩空白、負號、兩位數。
    const std::string rendered = std::to_string(value);
    const double parsed = std::stod("3.5");
    assert(rendered == "-42");
    assert(std::abs(parsed - 3.5) < 1e-12);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 8. String to Integer (atoi)（字串轉整數）
// 題目：略過前導空格、讀可選正負號與連續數字，遇非數字停止，超出 32-bit int 時 clamp；
//       例如 "4193 with words" 回 4193。
// 為何使用本章主題：雖然本章介紹 stoi，原題的精確 grammar 與 clamp 更適合手寫；本實作
//       在乘 10 前檢查上限，避免例外、locale 差異及 long long signed overflow。
// 思路：1. 略過 ASCII space；2. 讀符號；3. 逐位累積前先檢查 limit；4. 套符號並處理 INT_MIN。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是掃描到停止位置前的字元數。
// 易錯點：負數可容納的絕對值比 INT_MAX 多 1；必須在乘法前檢查，不能先 overflow 再 clamp。
// -----------------------------------------------------------------------------
int leetcode_my_atoi(const std::string& text) {
    std::size_t i = 0U;
    while (i < text.size() && text[i] == ' ') ++i;
    int sign = 1;
    if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
        sign = text[i] == '-' ? -1 : 1;
        ++i;
    }
    long long value = 0;
    const long long positive_limit = std::numeric_limits<int>::max();
    const long long negative_limit = positive_limit + 1LL;
    const long long limit = sign > 0 ? positive_limit : negative_limit;
    while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
        const int digit = text[i] - '0';
        // 在乘 10 前檢查，避免超長輸入先讓 long long 發生 signed overflow。
        if (value > (limit - digit) / 10LL) {
            return sign > 0 ? std::numeric_limits<int>::max()
                            : std::numeric_limits<int>::min();
        }
        value = value * 10LL + digit;
        ++i;
    }
    if (sign < 0 && value == negative_limit) return std::numeric_limits<int>::min();
    const int narrowed = static_cast<int>(value);
    return sign > 0 ? narrowed : -narrowed;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】重試次數設定解析
// 情境：設定值必須是完整十進位整數且介於 0..10；"3" 合法，"3x" 與 "99" 不合法。
// 為何使用本章主題：stoi 介面高階且適合低頻設定載入；搭配 idx 可拒絕部分解析，catch
//       兩種標準例外後以 optional 統一錯誤，而不是讓例外穿過設定邊界。
// 設計：1. stoi 並取得 consumed；2. 要求 consumed==size；3. 驗證領域範圍；4. 例外回 nullopt。
// 成本：時間 O(N)、額外空間 O(1)，N 是 text 長度；例外失敗路徑通常有較高常數成本。
// 上線注意：stoi 會略過前導空白，是否允許需明定；若要回報錯誤原因，optional 資訊不足。
// -----------------------------------------------------------------------------
std::optional<int> practical_parse_retry_count(const std::string& text) {
    try {
        std::size_t consumed = 0U;
        const int value = std::stoi(text, &consumed);
        if (consumed != text.size() || value < 0 || value > 10) return std::nullopt;
        return value;
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_my_atoi("42") == 42);
    assert(leetcode_my_atoi("   -042") == -42);
    assert(leetcode_my_atoi("4193 with words") == 4193);
    assert(leetcode_my_atoi("999999999999") == 2147483647);
    assert(leetcode_my_atoi(std::string(200U, '9')) == std::numeric_limits<int>::max());
    assert(leetcode_my_atoi("-999999999999999999999999") == std::numeric_limits<int>::min());
    assert(practical_parse_retry_count("3").value() == 3);
    assert(!practical_parse_retry_count("3x").has_value());
    assert(!practical_parse_retry_count("99").has_value());
    std::cout << "numeric conversions: tests passed\n";
}

/*
 * 【面試速查】
 * 【陷阱】不傳 idx 就可能把 "12px" 當成功；嚴格設定格式一定驗證消耗位置。
 * - 不傳 idx 時，stoi("12x") 會成功回 12；嚴格解析必須檢查 idx==size。
 * - base=0 可依 0/0x 前綴推斷進位，但設定格式若要求十進位就明確傳 10。
 * - to_string 浮點輸出格式不適合精密序列化；用 format/to_chars 並定義精度。
 * 【練習】解析百分比 "87.5" 到 double，要求完整消耗且範圍 0..100。
 */

/*
 * 【教科書補充：sto* parser 的完整輸入邊界】
 * - stoul/stoull 仍接受前導正負號；unsigned 目的型別不代表字串 "-1" 會自動被拒絕。
 * - runtime base 的合法值只有 0 或 2..36；0 會依前綴自動判定，必須是刻意選擇。
 * - pos 必須檢查是否消耗完整輸入，否則 "42junk" 會被當成成功前綴；另測空白、空字串與溢位。
 * - 外部格式要求「純十進位非負整數」時，from_chars 加完整消耗通常比 stoul 的 locale/符號語意清楚。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'numeric_conversions.cpp' -o '/tmp/codex_cpp_C_String_numeric_conversions' && '/tmp/codex_cpp_C_String_numeric_conversions'
//
// === 預期輸出（節錄）===
// numeric conversions: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
