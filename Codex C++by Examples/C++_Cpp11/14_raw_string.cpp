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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 125. Valid Palindrome（驗證回文）
// 題目：忽略非英數字元與大小寫後判斷是否回文；"A man, a plan, a canal: Panama" 為 true。
// 為何使用本章主題：演算法不依賴 raw string；raw literal 只讓含標點的測試字串保持原貌而不需額外跳脫。
// 思路：1. 左右指標跳過非英數；2. 將兩端以 unsigned char 安全轉小寫比較；3. 相同則向中央收縮。
// 複雜度：N 為字串長度；時間 O(N)、額外空間 O(1)。
// 易錯點：cctype 參數須先轉 unsigned char；right 採半開區間，取字元時要用 right-1。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP 存取紀錄狀態碼擷取
// 情境：從單行 log 的 status=NNN 欄位擷取三位數狀態碼，格式不符時回傳 -1。
// 為何使用本章主題：raw string 保留 regex 的括號與反斜線可讀性，避免 C++ 字串跳脫干擾規則審查。
// 設計：1. 建立 status=([0-9]{3}) 規則；2. regex_search 找第一個命中；3. 將 capture group 轉為整數。
// 成本：L 為行長；搜尋成本依標準庫 regex 實作與內容而定，另有 match 與字串配置成本。
// 上線注意：-1 與合法領域要明確區分；需限制行長、防止高成本 regex 輸入，並處理 stoi 例外。
// -----------------------------------------------------------------------------
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
