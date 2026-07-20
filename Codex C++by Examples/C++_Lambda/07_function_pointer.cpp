/*
 * Lambda 教科書 07：captureless lambda 與 function pointer
 *
 * 沒有 capture 的 lambda 可隱式轉成相容 function pointer；有 capture 的 closure 需要保存
 * state，無法塞進單純地址。function pointer 很小、ABI 簡單，適合 C API、interrupt/table；
 * 但它不擁有 context，也不能直接保存任意 state。
 *
 * 【型別】`noexcept` 自 C++17 是 function type 一部分；signature、calling convention 與
 * language linkage 必須符合外部 API。generic lambda 只有在選定可匹配 specialization 時轉換。
 * 【context】傳統 C API 通常提供 `callback(args, void* user_data)`，由 caller 管 context lifetime。
 * 【選擇】單一 stateless callback 用 pointer；需要 state 用 template/std::function 或 pointer+context。
 * 【面試題】為何 `[factor](int x){...}` 不能轉成 int(*)(int)？closure 需要 factor object。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

namespace basic {
using BinaryOperation = int (*)(int, int);

int apply(BinaryOperation operation, int left, int right) {
    return operation(left, right);
}

void demo() {
    BinaryOperation add = [](int left, int right) { return left + right; };
    assert(apply(add, 2, 3) == 5);
    // int factor = 2; BinaryOperation bad = [factor](int x){ return factor*x; }; // 不可轉換。
}
}  // namespace basic

namespace leetcode {
using KeepPredicate = bool (*)(int value, int removed);

bool keep_not_equal(int value, int removed) { return value != removed; }

// LeetCode 27：Remove Element。function pointer 注入保留規則，in-place O(n)/O(1)。
int leetcode_remove_element(std::vector<int>& nums, int removed, KeepPredicate keep) {
    std::size_t write = 0U;
    for (const int value : nums) {
        if (keep(value, removed)) nums[write++] = value;
    }
    nums.resize(write);
    return static_cast<int>(write);
}

void leetcode_test() {
    std::vector<int> nums{3, 2, 2, 3};
    assert(leetcode_remove_element(nums, 3, &keep_not_equal) == 2);
    assert(nums == std::vector<int>({2, 2}));
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
using CCallback = void (*)(int event, void* context);

void emit_event(int event, CCallback callback, void* context) {
    if (callback != nullptr) callback(event, context);
}

void practical_test() {
    int observed = 0;
    const auto callback = [](int event, void* context) {
        int* const output = static_cast<int*>(context);
        *output = event;
    };
    emit_event(42, callback, &observed);
    assert(observed == 42);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "function pointer：captureless conversion、Remove Element、C callback 測試通過\n";
}
