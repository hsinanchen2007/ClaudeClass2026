/*
 * C++17 教科書：if constexpr
 *
 * 在 template 中，if constexpr 的 condition 必須可在 compile time 決定。未選分支會被
 * discarded，不需對該 specialization 形成有效程式；這讓一個模板可針對型別能力分流。
 * 普通 `if` 的兩個 branch 都必須編譯合法，不能拿來取代 SFINAE/tag dispatch。
 *
 * 【限制】discarded branch 仍必須能被 parser 理解；完全不依賴 template 的硬語法錯誤
 * 不一定被隱藏。branch 外的 return/type requirement 仍照常檢查。
 * 【選擇】少量局部分流用 if constexpr；公開約束在 C++20 通常用 concepts 更清楚。
 * 【陷阱】把 runtime flag 寫進 condition 會編譯失敗，因它不是 constant expression。
 * 【面試題】if constexpr 是否有 runtime branch cost？選定 specialization 後沒有該分支。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace basic {
template <class T>
std::string describe(const T& value) {
    if constexpr (std::is_integral<T>::value) {
        return "integer:" + std::to_string(value);
    } else {
        return "other";
    }
}

void demo() {
    assert(describe(7) == "integer:7");
    assert(describe(std::string("x")) == "other");
}
}  // namespace basic

namespace leetcode {
// LeetCode 136：Single Number。integral container 才能使用 XOR；不合法型別不實例化該 branch。
template <class T>
T leetcode_single_number(const std::vector<T>& nums) {
    static_assert(std::is_integral<T>::value, "Single Number 的 XOR 版本需要 integral type");
    T answer{};
    if constexpr (std::is_integral<T>::value) {
        for (const T value : nums) answer ^= value;
    }
    return answer;
}

void leetcode_test() {
    assert(leetcode_single_number(std::vector<int>{4, 1, 2, 1, 2}) == 4);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
template <class T>
std::string practical_encode(const T& value) {
    if constexpr (std::is_same<T, std::string>::value) {
        return "\"" + value + "\"";
    } else if constexpr (std::is_arithmetic<T>::value) {
        return std::to_string(value);
    } else {
        static_assert(std::is_arithmetic<T>::value,
                      "practical_encode 只支援 string 與 arithmetic types");
    }
}

void practical_test() {
    assert(practical_encode(std::string("ok")) == "\"ok\"");
    assert(practical_encode(42) == "42");
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "if constexpr：型別分流、Single Number、encode 測試通過\n";
}
