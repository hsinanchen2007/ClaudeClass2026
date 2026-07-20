/*
 * Lambda 教科書 01：基本語法與 closure object
 *
 * 語法：`[captures](parameters) specifiers -> return_type { body }`。
 * lambda expression 會建立一個匿名 closure class 的 object；每個 expression 都是不同型別，
 * 即使文字完全相同也不能互相 assignment。`auto` 最能零成本保存具體 closure 型別。
 *
 * 【return】單一一致 expression 通常可推導；多個分支型別不同時明寫 `-> Type`。
 * 【captureless】沒有 capture 的 lambda 可轉成相容 function pointer，適合 C callback。
 * 【constexpr】C++17 起可明寫 constexpr，符合條件時 call operator 也可隱含 constexpr；C++20 另加入 consteval lambda。
 * 【成本】直接呼叫通常可 inline；把它放進 std::function 才引入 type erasure/可能配置。
 * 【常見陷阱】兩個文字相同 lambda 仍是不同 closure type，不能假設可以互相 assignment。
 * 【面試題】兩個相同 lambda expression 的 decltype 是否相同？不同。
 * 【練習】替 practical_validate 加入長度上限，但維持 predicate 無副作用。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

namespace basic {
void demo() {
    const auto add = [](int left, int right) { return left + right; };
    const auto positive = [](int value) noexcept { return value > 0; };
    assert(add(2, 3) == 5 && positive(1));

    const auto first = [] { return 1; };
    const auto second = [] { return 1; };
    static_assert(!std::is_same_v<decltype(first), decltype(second)>);
}
}  // namespace basic

namespace leetcode {
// LeetCode 1480：Running Sum。lambda 作為 accumulate-like state update。
std::vector<int> leetcode_running_sum(const std::vector<int>& nums) {
    std::vector<int> output;
    output.reserve(nums.size());
    int total = 0;
    const auto append_prefix = [&output, &total](int value) {
        total += value;
        output.push_back(total);
    };
    std::for_each(nums.begin(), nums.end(), append_prefix);
    return output;
}

void leetcode_test() {
    assert(leetcode_running_sum({1, 2, 3, 4}) ==
           std::vector<int>({1, 3, 6, 10}));
}
}  // namespace leetcode

// 【實務案例】批次使用者驗證：把只在呼叫點使用的規則寫成無副作用 predicate。
namespace practical {
struct User {
    std::string name;
    int age;
};

bool practical_validate(const std::vector<User>& users) {
    const auto valid_user = [](const User& user) {
        return !user.name.empty() && user.age >= 0 && user.age <= 130;
    };
    return std::all_of(users.begin(), users.end(), valid_user);
}

void practical_test() {
    assert(practical_validate({{"Ada", 36}, {"Bjarne", 72}}));
    assert(!practical_validate({{"", 20}}));
    assert(!practical_validate({{"Future", 200}}));
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "lambda 基礎：closure、Running Sum、資料驗證測試通過\n";
}

/*
 * 【版本釐清】lambda 可明寫 constexpr 是 C++17；符合條件的 call operator 也可隱含 constexpr。
 * C++20 在這條演進上新增 consteval lambda 等能力，不是「第一次能明寫 constexpr」。
 * 判斷是否真的常數求值要把呼叫放進 static_assert/constant-expression context，而非只看宣告。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_basic_lambda.cpp' -o '/tmp/codex_cpp_C_Lambda_01_basic_lambda' && '/tmp/codex_cpp_C_Lambda_01_basic_lambda'
//
// === 預期輸出（節錄）===
// lambda 基礎：closure、Running Sum、資料驗證測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
