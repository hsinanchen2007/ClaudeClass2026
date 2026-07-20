/*
 * std::string::find：由左至右尋找 substring、C 字串或 char
 *
 * 回傳第一個匹配的起始索引；找不到回傳 string::npos。第二參數 pos 是「從哪個
 * 索引開始考慮匹配」。不要把結果轉 int 再與 -1 比；npos 是 size_type 的最大值。
 * 標準只給上界，直觀最壞可視為 O((n-pos)*m)，實作可能使用更好的搜尋法。
 */

#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace {

void basic_demo() {
    const std::string text = "one two two";
    assert(text.find("two") == 4U);
    assert(text.find("two", 5U) == 8U);
    assert(text.find('x') == std::string::npos);
    assert(text.find("") == 0U);  // 空 needle 在合法 pos 立即匹配。
}

// LeetCode 28（Find the Index of the First Occurrence in a String）。
int leetcode_str_str(const std::string& haystack, const std::string& needle) {
    const std::size_t position = haystack.find(needle);
    return position == std::string::npos ? -1 : static_cast<int>(position);
}

// 實務：解析第一個 key=value；value 可以再含 '='，所以只切第一個。
std::optional<std::pair<std::string, std::string>>
practical_parse_assignment(const std::string& line) {
    const std::size_t equal = line.find('=');
    if (equal == std::string::npos || equal == 0U) {
        return std::nullopt;
    }
    return std::pair{line.substr(0U, equal), line.substr(equal + 1U)};
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_str_str("sadbutsad", "sad") == 0);
    assert(leetcode_str_str("leetcode", "leeto") == -1);
    assert(leetcode_str_str("abc", "") == 0);

    const auto parsed = practical_parse_assignment("token=a=b");
    assert(parsed.has_value());
    assert(parsed->first == "token" && parsed->second == "a=b");
    assert(!practical_parse_assignment("=missing-key").has_value());
    std::cout << "find: tests passed\n";
}

/*
 * 【npos 安全規則】
 * - 先判 `pos != npos`，才可做 substr(pos) 或 pos+needle.size()。
 * - `npos + 1` 會無號 wrap 成 0，可能把「找不到」誤當字串開頭。
 * - 找所有不重疊匹配：成功後從 pos + needle.size() 繼續；needle 空時另訂規格。
 *
 * 【面試】find 的 pos 是限制結果必須 >= pos，不是把字串先 substr；因此不需配置。
 * 【練習】實作 count_non_overlapping(text,needle)，明確處理空 needle。
 */

/*
 * 【搜尋所有匹配模板】
 * 1. `pos=find(needle,search_from)`。
 * 2. 若 npos 結束；否則處理 pos。
 * 3. 不重疊搜尋從 pos+needle.size()；重疊搜尋從 pos+1。
 * 4. needle 為空時必須另訂規格，否則游標可能永遠不前進。
 * 【陷阱】`if (text.find(x))` 把位置 0 當 false、npos 當 true，邏輯正好顛倒。
 * 【面試題】為何不先 substr(pos).find？那會建立副本並讓回傳索引變相對位置。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'find.cpp' -o '/tmp/codex_cpp_C_String_find' && '/tmp/codex_cpp_C_String_find'
//
// === 預期輸出（節錄）===
// find: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
