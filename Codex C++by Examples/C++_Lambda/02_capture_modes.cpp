/*
 * Lambda 教科書 02：capture modes 與生命週期
 *
 * [] 不 capture；[x] value copy；[&x] reference；[=]/[&] 對 body 使用到的自動變數採預設；
 * [this] capture pointer，[*this]（C++17）capture object copy。capture 在建立 closure 時發生。
 *
 * 【value】closure 內保存一份，原變數之後改變不影響它；預設 call operator 是 const。
 * 【reference】不擁有對象，closure 若活過被 capture 變數就 dangling。async/callback 最危險。
 * 【this】只複製 pointer，不延長 object lifetime；owner 已析構後 callback 存取成員是 UB。
 * 【建議】列出必要 capture 比 [=]/[&] 更易 code review；跨 scope 要 capture value/owner。
 * 【常見陷阱】reference 或 this capture 不延長 lifetime，長期 callback 最容易 use-after-free。
 * 【面試題】[=] 是否複製 this object？傳統 [=] 隱式 capture this pointer，不是整個 object。
 * 【練習】把 practical_filter 改為 capture 上下限兩個值，測試原值改變不影響 closure。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace basic {
void demo() {
    int threshold = 10;
    const auto by_value = [threshold](int value) { return value >= threshold; };
    const auto by_reference = [&threshold](int value) { return value >= threshold; };
    threshold = 20;
    assert(by_value(15));       // closure 保存舊值 10
    assert(!by_reference(15)); // reference 看見新值 20
}
}  // namespace basic

namespace leetcode {
// LeetCode 1：Two Sum。target 以 value capture，seen/output 以 reference capture。
std::pair<int, int> leetcode_two_sum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> seen;
    std::pair<int, int> answer{-1, -1};
    const auto inspect = [target, &seen, &answer](int value, int index) {
        if (const auto found = seen.find(target - value); found != seen.end()) {
            answer = {found->second, index};
        } else {
            seen.emplace(value, index);
        }
    };
    for (std::size_t i = 0; i < nums.size() && answer.first < 0; ++i) {
        inspect(nums[i], static_cast<int>(i));
    }
    return answer;
}

void leetcode_test() {
    assert((leetcode_two_sum({2, 7, 11, 15}, 9) == std::pair<int, int>{0, 1}));
}
}  // namespace leetcode

// 【實務案例】門檻篩選：minimum 以 value capture 固定成這次查詢的不可變快照。
namespace practical {
std::vector<int> practical_filter(const std::vector<int>& values, int minimum) {
    std::vector<int> output;
    std::copy_if(values.begin(), values.end(), std::back_inserter(output),
                 [minimum](int value) { return value >= minimum; });
    return output;
}

void practical_test() {
    assert(practical_filter({2, 9, 5, 12}, 6) == std::vector<int>({9, 12}));
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "lambda capture：value/ref、Two Sum、filter 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_capture_modes.cpp' -o '/tmp/codex_cpp_C_Lambda_02_capture_modes' && '/tmp/codex_cpp_C_Lambda_02_capture_modes'
//
// === 預期輸出（節錄）===
// lambda capture：value/ref、Two Sum、filter 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
