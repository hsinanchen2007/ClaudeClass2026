/*
 * C++11 教科書：raw string literal
 *
 * R"(text)" 內的 backslash、newline、quote 不需一般 escape，適合 regex、SQL、JSON
 * 範例與多行模板。若內容本身含 )"，可自訂 delimiter：R"tag(... )tag"。
 * raw string 只影響 C++ source 的表示法，不會自動驗證 JSON/SQL，也不防 injection。
 *
 * 【選擇】一般短字串仍用 "..."；有大量反斜線/換行時 raw literal 才較清楚。
 * 【安全】SQL 仍應用 parameterized query；regex 仍要考慮 catastrophic backtracking。
 * 【常見陷阱】raw string 只減少 escape，並不會驗證 JSON、SQL 或 regex 的安全性。
 * 【編碼】u8R"(...)" 在 C++20 是 const char8_t[]，與 std::string API 不直接相容。
 * 【面試題】如何在 raw string 中放入 )"？使用自訂 delimiter。
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <regex>
#include <string>

namespace basic {
void demo() {
    const std::string windows_path = R"(C:\Users\student\notes.txt)";
    const std::string json = R"json({"name":"Ada","active":true})json";
    assert(windows_path.find("\\Users\\") != std::string::npos);
    assert(json.find("\"active\":true") != std::string::npos);
}
}  // namespace basic

namespace leetcode {
// LeetCode 125：Valid Palindrome。raw strings 讓測試資料的標點更貼近原文。
bool is_palindrome(const std::string& text) {
    std::size_t left = 0U;
    std::size_t right = text.size();
    while (left < right) {
        while (left < right && !std::isalnum(static_cast<unsigned char>(text[left]))) ++left;
        while (left < right && !std::isalnum(static_cast<unsigned char>(text[right - 1U]))) --right;
        if (left < right) {
            const auto lhs = static_cast<unsigned char>(text[left]);
            const auto rhs = static_cast<unsigned char>(text[right - 1U]);
            if (std::tolower(lhs) != std::tolower(rhs)) return false;
            ++left;
            --right;
        }
    }
    return true;
}

void test() {
    assert(is_palindrome(R"(A man, a plan, a canal: Panama)"));
    assert(!is_palindrome(R"(race a car)"));
}
}  // namespace leetcode

// 【實務案例】log status 擷取：raw literal 讓 regex 規則可讀，但 parser 仍明確回報失敗。
namespace practical {
// 實務：從簡化 log 中抽 status；正式 production parser 應有格式邊界與錯誤處理。
int extract_status(const std::string& line) {
    const std::regex pattern(R"regex(status=([0-9]{3}))regex");
    std::smatch match;
    if (!std::regex_search(line, match, pattern)) return -1;
    return std::stoi(match[1].str());
}

void test() {
    assert(extract_status(R"(method=GET path="/health" status=200)") == 200);
    assert(extract_status(R"(malformed line)") == -1);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "raw string：path/JSON、palindrome、log regex 測試通過\n";
}

// 【延伸練習】寫一段同時含 )" 的 regex/JSON，選安全 delimiter，並測試 malformed input。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '14_raw_string.cpp' -o '/tmp/codex_cpp_C_Cpp11_14_raw_string' && '/tmp/codex_cpp_C_Cpp11_14_raw_string'
//
// === 預期輸出（節錄）===
// raw string：path/JSON、palindrome、log regex 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
