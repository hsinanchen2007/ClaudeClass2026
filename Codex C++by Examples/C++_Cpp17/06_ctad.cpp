/*
 * C++17 教科書：Class Template Argument Deduction（CTAD）
 *
 * 建構 class template object 時，compiler 可由 constructor arguments 推導 template args：
 * `std::pair p{1, 2.5};` -> pair<int,double>。function templates 很早就能推導；C++17
 * 把相同便利帶到 class。推導來源是 implicit deduction guides 或自訂 deduction guide。
 *
 * 【不是 auto】CTAD 推導 class template parameters；auto 推導變數型別，兩者可同時出現。
 * 【陷阱】copy/deduction、initializer_list、pointer/array decay 可能得到意外型別；公開 API
 * 若意圖不明，明寫 template args。CTAD 只在宣告 object 時用，function parameter 仍不能
 * 寫成 `void f(std::vector v)`（C++20 abbreviated template 也不是這種語法）。
 * 【面試題】deduction guide 是 function 嗎？語法像 function，但不是真正可呼叫函式。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace basic {
template <class T>
struct Box {
    T value;
};

template <class T>
Box(T) -> Box<T>;

void demo() {
    Box integer{42};
    std::pair pair{std::string("port"), 443};
    std::vector values{1, 2, 3};
    static_assert(std::is_same<decltype(integer), Box<int> >::value,
                  "custom deduction guide 應推導 Box<int>");
    static_assert(std::is_same<decltype(pair), std::pair<std::string, int> >::value,
                  "pair CTAD 型別錯誤");
    assert(integer.value == 42 && values.size() == 3U);
}
}  // namespace basic

namespace leetcode {
// LeetCode 1：Two Sum。CTAD 讓成功/失敗 pair 的 element types 由 arguments 決定。
std::pair<int, int> leetcode_two_sum(const std::vector<int>& nums, int target) {
    for (std::size_t left = 0; left < nums.size(); ++left) {
        for (std::size_t right = left + 1U; right < nums.size(); ++right) {
            if (nums[left] + nums[right] == target) {
                return std::pair{static_cast<int>(left), static_cast<int>(right)};
            }
        }
    }
    return std::pair{-1, -1};
}

void leetcode_test() {
    assert((leetcode_two_sum({2, 7, 11, 15}, 9) == std::pair<int, int>{0, 1}));
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
template <class Payload>
struct Envelope {
    std::string topic;
    Payload payload;
};

template <class Payload>
Envelope(std::string, Payload) -> Envelope<Payload>;

auto practical_event() {
    return Envelope{std::string("temperature"), 73.5};
}

void practical_test() {
    const auto event = practical_event();
    static_assert(std::is_same<decltype(event.payload), double>::value,
                  "payload 應推導為 double");
    assert(event.topic == "temperature" && event.payload == 73.5);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "CTAD：deduction guide、Two Sum、event envelope 測試通過\n";
}
