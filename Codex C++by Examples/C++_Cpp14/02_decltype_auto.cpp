/*
 * C++14 教科書：decltype(auto)
 *
 * decltype(auto) 用 decltype 規則推導整個宣告：能保留 reference、const 與 value category。
 * `decltype(auto) x = expression;` 不能再加 `&`，因 reference 已由 expression 決定。
 * 函式回傳時：`return object.member;` 與 `return (object.member);` 可能不同；括號版本是
 * lvalue expression，會回 T&。這個力量也很危險，回傳 local/temporary reference 會 dangling。
 *
 * 【選擇】要 value/copy 用 auto；要透明 forwarding/accessor 才用 decltype(auto)。
 * 【容器】vector<bool>::operator[] 回 proxy，decltype(auto) 可保留 proxy，但 caller 必須理解。
 * 【常見陷阱】多一層括號可能把 value return 改成 reference return，造成 dangling。
 * 【面試題】auto、decltype(expr)、decltype(auto) 三者如何處理 reference？
 * 【練習】為 practical_setting 寫 const overload，確認回傳 const std::string&。
 */

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace basic {
decltype(auto) first(std::vector<int>& values) {
    return (values.front());  // 括號使 expression 明確是 lvalue，回 int&。
}

auto first_copy(std::vector<int>& values) {
    return values.front();  // auto 傳值推導，回 int。
}

void demo() {
    std::vector<int> values{3, 4};
    static_assert(std::is_same<decltype(first(values)), int&>::value,
                  "decltype(auto) 應保留 front() reference");
    static_assert(std::is_same<decltype(first_copy(values)), int>::value,
                  "auto 應複製 front() value");
    first(values) = 9;
    assert(values.front() == 9 && first_copy(values) == 9);
}
}  // namespace basic

namespace leetcode {
// LeetCode 303：Range Sum Query - Immutable。prefix 建構 O(n)，查詢 O(1)。
class NumArray {
public:
    explicit NumArray(const std::vector<int>& nums) : prefix_(nums.size() + 1U, 0) {
        for (std::size_t i = 0; i < nums.size(); ++i) {
            prefix_[i + 1U] = prefix_[i] + nums[i];
        }
    }

    long long leetcode_sum_range(std::size_t left, std::size_t right) const {
        return prefix_.at(right + 1U) - prefix_.at(left);
    }

private:
    std::vector<long long> prefix_;
};

void leetcode_test() {
    const NumArray values({-2, 0, 3, -5, 2, -1});
    assert(values.leetcode_sum_range(0U, 2U) == 1);
    assert(values.leetcode_sum_range(2U, 5U) == -1);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
struct Configuration {
    std::map<std::string, std::string> values;
};

decltype(auto) practical_setting(Configuration& config, const std::string& key) {
    return (config.values.at(key));  // 回 std::string&，讓管理介面可更新既有欄位。
}

void practical_test() {
    Configuration config{{{"mode", "safe"}}};
    practical_setting(config, "mode") = "fast";
    assert(config.values.at("mode") == "fast");
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "decltype(auto)：參考保留、前綴和、設定更新測試通過\n";
}
