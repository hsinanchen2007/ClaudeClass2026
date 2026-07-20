/*
 * C++14 教科書：function auto return type deduction
 *
 * C++14 允許普通函式與 function template 只寫 auto，compiler 由 return expression 推導。
 * 所有可到達的 return 必須推導成同一型別；auto 採傳值推導，會去掉 top-level const/ref。
 * 若要精確保留 reference，使用同版新增的 decltype(auto)，但必須審查生命週期。
 *
 * 【何時使用】回傳 iterator、closure、複雜模板運算結果，且型別由實作顯然可知。
 * 公開 API 若回傳型別本身是契約，明寫型別通常更清楚，也能避免實作改動悄悄改 ABI。
 * 【遞迴】auto-return 遞迴函式必須先有可供推導的 return，否則遞迴 call 時尚未知型別。
 * 【面試題】`const int& f(); auto g(){return f();}` 的 g 回傳什麼？int（複製）。
 * 【練習】讓 practical_average 同時支援 vector<float> 且避免整數除法。
 */

#include <cassert>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace basic {
auto twice(int value) { return value * 2; }

template <class Left, class Right>
auto plus_value(Left left, Right right) {
    return left + right;
}

void demo() {
    static_assert(std::is_same<decltype(twice(2)), int>::value,
                  "twice 應推導為 int");
    static_assert(std::is_same<decltype(plus_value(2, 0.5)), double>::value,
                  "混合運算應依 operator+ 推導 double");
    assert(twice(6) == 12);
}
}  // namespace basic

namespace leetcode {
// LeetCode 1480：Running Sum，時間 O(n)、空間 O(n)。
// auto 由 vector<int> return expression 推導，不需要重複長型別。
auto leetcode_running_sum(const std::vector<int>& nums) {
    std::vector<int> result;
    result.reserve(nums.size());
    int total = 0;
    for (const int value : nums) {
        total += value;
        result.push_back(total);
    }
    return result;
}

void leetcode_test() {
    assert(leetcode_running_sum({1, 2, 3, 4}) ==
           std::vector<int>({1, 3, 6, 10}));
}
}  // namespace leetcode

// 【實務案例】監控樣本平均值：實作決定 double 回傳型別，空集合則明確拒絕。
namespace practical {
struct Sample {
    std::string name;
    double value;
};

// 實務：計算監控樣本平均值；空集合明確丟 exception，不回傳 NaN 掩蓋問題。
auto practical_average(const std::vector<Sample>& samples) {
    if (samples.empty()) {
        throw std::invalid_argument("samples 不可為空");
    }
    const double total = std::accumulate(
        samples.begin(), samples.end(), 0.0,
        [](double sum, const Sample& sample) { return sum + sample.value; });
    return total / static_cast<double>(samples.size());
}

void practical_test() {
    const std::vector<Sample> samples{{"cpu", 40.0}, {"gpu", 80.0}};
    assert(practical_average(samples) == 60.0);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "C++14 auto return：running sum 與監控平均值測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++14 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_auto_return_type.cpp' -o '/tmp/codex_cpp_C_Cpp14_01_auto_return_type' && '/tmp/codex_cpp_C_Cpp14_01_auto_return_type'
//
// === 預期輸出（節錄）===
// C++14 auto return：running sum 與監控平均值測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
