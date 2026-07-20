/*
 * 字串 literal："text"、"text"s 與 "text"sv 的型別與生命週期
 *
 * "abc"    是 const char[4]，在多數運算式會退化為 const char*。
 * "abc"s   是 std::string，擁有內容，可修改，也能保留內嵌 '\0'。
 * "abc"sv  是 std::string_view，不擁有內容；指向靜態 literal 時生命週期安全。
 * u8/u/U/L 前綴代表不同 code unit 型別，並不自動做 Unicode 字元切割。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace std::string_literals;
using namespace std::string_view_literals;

void basic_demo() {
    const auto owning = "A\0B"s;
    constexpr auto view = "GET"sv;
    assert(owning.size() == 3U);
    static_assert(view.size() == 3U);

    // "a" + "b" 不合法：兩邊都不是 std::string。加入 s 後才選到 operator+。
    const std::string joined = "user="s + "alice";
    assert(joined == "user=alice");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 14. Longest Common Prefix（最長共同前綴）
// 題目：找出所有 words 的最長共同前綴；flower/flow/flight 得 fl，沒有共同前綴回空。
// 為何使用本章主題：prefix 以 string_view 借用第一個 word，remove_suffix 只縮長度不配置；
//       最後才建 owning string。literal 章節同時說明 view 與 owning 結果的生命週期差異。
// 思路：1. 空集合回空；2. prefix 起始為第一字；3. 每個 word 不符合時逐字縮尾；4. 複製結果。
// 複雜度：最壞時間 O(W*P^2)、額外空間 O(P)，W 是單字數、P 是首字長度，反覆 starts_with 會重比前綴。
// 易錯點：prefix 縮成空後 starts_with(empty) 為 true；view 借用 words.front，vector/字串不可在迴圈中修改。
// -----------------------------------------------------------------------------
std::string leetcode_longest_common_prefix(const std::vector<std::string>& words) {
    if (words.empty()) {
        return {};
    }
    std::string_view prefix = words.front();
    for (const std::string& word : words) {
        while (!word.starts_with(prefix)) {
            prefix.remove_suffix(1U);
        }
    }
    return std::string(prefix);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP 唯讀方法分類
// 情境：將 GET 與 HEAD 視為只讀方法，POST 等其他 token 不接受。
// 為何使用本章主題：`"GET"sv`、`"HEAD"sv` 指向靜態 literal，零配置且可直接與輸入 view 比內容；
//       相較每次建 std::string，不需要 ownership。
// 設計：1. 借用 method；2. 與兩個固定 sv literal 精確比較；3. 任一相等回 true。
// 成本：固定短 token 下時間與空間 O(1)；一般長度 M 時比較為 O(M)。
// 上線注意：HTTP method 區分大小寫且需要完整 token；輸入 view 的來源仍須在呼叫期間有效。
// -----------------------------------------------------------------------------
bool practical_is_read_method(const std::string_view method) {
    return method == "GET"sv || method == "HEAD"sv;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_longest_common_prefix({"flower", "flow", "flight"}) == "fl");
    assert(leetcode_longest_common_prefix({"dog", "racecar", "car"}).empty());
    assert(practical_is_read_method("GET"));
    assert(!practical_is_read_method("POST"));
    std::cout << "string literals: tests passed\n";
}

/*
 * 【陷阱】`std::string_view v = std::string("tmp");` 立刻懸空；literal 建立的 view 才有
 * 靜態生命週期。`u8"中文"` 在 C++20 是 const char8_t[]，不能直接當 std::string。
 * 【面試】"A\0B"s 為何長度 3？s literal 的 operator 接收陣列長度，不靠 strlen。
 * 【練習】用 "application/json"sv 寫一個 content-type 判定函式。
 */

/*
 * 【面試速查】沒有 suffix 的 literal 是陣列；`s` 建 owning string；`sv` 建 non-owning view。
 * 【陷阱】自訂 literal namespace 不會自動開啟，需明確 `using namespace std::string_literals`。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'string_literals.cpp' -o '/tmp/codex_cpp_C_String_string_literals' && '/tmp/codex_cpp_C_String_string_literals'
//
// === 預期輸出（節錄）===
// string literals: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
