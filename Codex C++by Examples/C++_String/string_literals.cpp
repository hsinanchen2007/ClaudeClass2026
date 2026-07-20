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

// LeetCode 14（Longest Common Prefix）：string_view 避免為每個候選字串建立副本。
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

// 實務：協定常數用 sv literal，不配置且可 constexpr 比較。
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
