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

// 【實務案例】typed event envelope：deduction guide 由 payload 推出 Envelope<double>。
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

// 【延伸練習】加入 const char* payload，觀察 decay 結果；再寫 guide 讓它擁有 std::string。

/*
 * 【教科書補充：CTAD 的版本與儲存型別】
 * - 本例 Box 在 C++17 需要明寫 deduction guide；一般 aggregate CTAD 是 C++20 才加入。
 * - 編譯器也會考慮 copy deduction candidate，因此由既有 template object 建立新物件時結果可能不同。
 * - array/string literal 常經 guide 參數型別發生 decay；若要擁有字元，guide 應明確推導 std::string。
 * - guide 只是選出 class specialization，不會替你驗證 ownership；推成 pointer/view 仍可能懸空。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_ctad.cpp' -o '/tmp/codex_cpp_C_Cpp17_06_ctad' && '/tmp/codex_cpp_C_Cpp17_06_ctad'
//
// === 預期輸出（節錄）===
// CTAD：deduction guide、Two Sum、event envelope 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
