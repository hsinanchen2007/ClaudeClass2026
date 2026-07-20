/*
 * C++11 教科書：trailing return type
 *
 * auto function(args) -> ReturnType 把回傳型別放在參數列後方。C++11 最重要用途是
 * generic function：ReturnType 可引用已進入 scope 的參數，例如 decltype(left + right)。
 * C++14 可讓普通函式 auto return deduction，但 trailing form 在複雜模板仍具可讀性。
 *
 * 【保留參考】decltype(expression) 可能推導 T&/T&&；回傳 reference 前要確認對象壽命
 * 長於呼叫者。若只是計算值，明確 decay/copy 常更安全。
 * 【常見陷阱】回傳 `decltype((local))` 會洩漏 local reference，函式結束後立即 dangling。
 * 【SFINAE】舊式模板會把有效性檢查放進 trailing return；C++20 concepts 通常更清楚。
 * 【面試題】為何 `auto f(T t) -> decltype(t.member())` 在 C++11 可行？參數 t 已可見。
 */

#include <cassert>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

namespace basic {
template <class Left, class Right>
auto multiply(Left left, Right right) -> decltype(left * right) {
    return left * right;
}

void demo() {
    static_assert(std::is_same<decltype(multiply(2, 0.5)), double>::value,
                  "int * double 應推導為 double");
    assert(multiply(3, 4) == 12);
}
}  // namespace basic

namespace leetcode {
// LeetCode 1480：Running Sum。回傳型別依 input container 的 value_type 建立 vector。
template <class Container>
auto running_sum(const Container& nums)
    -> std::vector<typename Container::value_type> {
    using Value = typename Container::value_type;
    std::vector<Value> result;
    result.reserve(nums.size());
    Value total{};
    for (const Value value : nums) {
        total += value;
        result.push_back(total);
    }
    return result;
}

void test() {
    const std::vector<int> answer = running_sum(std::vector<int>{1, 2, 3, 4});
    assert(answer == std::vector<int>({1, 3, 6, 10}));
}
}  // namespace leetcode

// 【實務案例】泛型 normalization adapter：trailing return 由物件的 normalized() 決定結果型別。
namespace practical {
struct Reading {
    double value;
    double normalized() const { return value / 100.0; }
};

template <class T>
auto normalize(const T& object) -> decltype(object.normalized()) {
    return object.normalized();
}

void test() {
    const Reading cpu{75.0};
    assert(normalize(cpu) == 0.75);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "trailing return：泛型運算、running sum、normalize 測試通過\n";
}

// 【延伸練習】替 normalize 加 const/non-const accessor，逐一寫出 decltype 推導與 lifetime。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '15_trailing_return.cpp' -o '/tmp/codex_cpp_C_Cpp11_15_trailing_return' && '/tmp/codex_cpp_C_Cpp11_15_trailing_return'
//
// === 預期輸出（節錄）===
// trailing return：泛型運算、running sum、normalize 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
