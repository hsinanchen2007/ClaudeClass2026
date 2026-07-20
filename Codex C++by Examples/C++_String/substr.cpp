/*
 * substr(pos,count)：建立一份新的子字串
 *
 * 從 pos 複製最多 count 個字元；count 預設 npos，表示到尾端。pos==size() 合法且回空；
 * pos>size() 丟 out_of_range。因為回傳擁有資料的 std::string，複雜度 O(result.size())
 * 且可能配置。只讀短暫切片可改用 string_view::substr，O(1)。
 */

#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

void basic_demo() {
    const std::string text = "012345";
    assert(text.substr(2U, 3U) == "234");
    const std::string clipped = text.substr(4U, 99U);  // count 過長會安全截到尾端。
    assert(clipped == "45");
    assert(text.substr(text.size()).empty());
}

// LeetCode 28 的教學版：逐位置建立 substring 比較，簡單但可能反覆配置。
int leetcode_str_str_substr(const std::string& text, const std::string& pattern) {
    if (pattern.size() > text.size()) return -1;
    for (std::size_t i = 0U; i + pattern.size() <= text.size(); ++i) {
        if (text.substr(i, pattern.size()) == pattern) return static_cast<int>(i);
    }
    return -1;
}

// 實務：解析 `scheme://rest`；結果需要脫離 input 生命週期，所以回 owning strings。
struct UrlParts {
    std::string scheme;
    std::string rest;
};

std::optional<UrlParts> practical_split_scheme(const std::string& url) {
    const std::size_t separator = url.find("://");
    if (separator == std::string::npos || separator == 0U) return std::nullopt;
    return UrlParts{url.substr(0U, separator), url.substr(separator + 3U)};
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_str_str_substr("sadbutsad", "sad") == 0);
    assert(leetcode_str_str_substr("abc", "d") == -1);
    const auto parsed = practical_split_scheme("https://example.test/a");
    assert(parsed.has_value());
    assert(parsed->scheme == "https" && parsed->rest == "example.test/a");
    assert(!practical_split_scheme("relative/path").has_value());
    std::cout << "substr: tests passed\n";
}

/*
 * 【效能】上述 LeetCode 教學版可能 O(n*m) 且有配置；find 或 string_view compare 更好。
 * 【生命週期】substr 回傳副本可安全保存；view substr 零拷貝但不可超過來源生命週期。
 * 【面試】count 超過剩餘長度是否丟例外？不會，自動截到尾端；只有 pos 超界會丟。
 * 【練習】以 string_view 改寫搜尋迴圈，確認零 substring 配置。
 */

/*
 * 【substr 邊界表】
 * - pos < size：從該元素開始複製。
 * - pos == size：合法空字串。
 * - pos > size：丟 out_of_range。
 * - count > size-pos 或 count==npos：截到尾端。
 * owning 結果不受來源後續修改影響；代價是 O(k) 複製/可能配置。
 * 【面試題】parser 為何偏好 string_view::substr？O(1) 切片；但 API 邊界需處理生命週期。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'substr.cpp' -o '/tmp/codex_cpp_C_String_substr' && '/tmp/codex_cpp_C_String_substr'
//
// === 預期輸出（節錄）===
// substr: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
