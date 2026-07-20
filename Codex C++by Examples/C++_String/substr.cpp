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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 28. Find the Index of the First Occurrence in a String（尋找首次出現位置）
// 題目：回傳 pattern 在 text 中第一次完整匹配的位置，找不到回 -1；"sadbutsad"、"sad" 回 0。
// 為何使用本章主題：每個候選位置以 substr 建 owning 片段再比較，便於展示 API 邊界；
//       這是會反覆配置的教學寫法，實際提交宜用 find 或 string_view::compare。
// 思路：1. pattern 較長直接失敗；2. 枚舉所有可容納起點；3. 建等長 substring；4. 首次相等回索引。
// 複雜度：時間 O((N-K+1)*K)、額外暫存空間 O(K)，N、K 是 text、pattern 長度。
// 易錯點：迴圈上界要允許最後一個候選；空 pattern 會在位置 0 匹配，索引轉 int 前要驗範圍。
// -----------------------------------------------------------------------------
int leetcode_str_str_substr(const std::string& text, const std::string& pattern) {
    if (pattern.size() > text.size()) return -1;
    for (std::size_t i = 0U; i + pattern.size() <= text.size(); ++i) {
        if (text.substr(i, pattern.size()) == pattern) return static_cast<int>(i);
    }
    return -1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】URL scheme 與剩餘部分拆分
// 情境：將 `https://example.test/a` 拆成 owning scheme=https 與 rest=example.test/a，供稍後保存。
// 為何使用本章主題：substr 建立獨立擁有的結果，來源 url 離開 scope 後仍安全；相較 string_view
//       多一次複製，但 API 生命週期更簡單。
// 設計：1. find `://`；2. 拒絕缺分隔符或空 scheme；3. 以兩次 substr 建立欄位。
// 成本：搜尋 O(N)，複製 O(N)，額外空間 O(N)，N 是 url 長度。
// 上線注意：這不是完整 URL parser，未驗 scheme grammar、空 host、authority、encoding 或長度上限。
// -----------------------------------------------------------------------------
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
