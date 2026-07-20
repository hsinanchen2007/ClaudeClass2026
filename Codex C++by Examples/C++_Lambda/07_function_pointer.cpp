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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element（移除元素）
// 題目：原地移除 nums 中所有等於 val 的元素並回傳新長度；[3,2,2,3]、val=3 得長度 2。
// 為何使用本章主題：函式指標把「是否保留」規則注入雙指標流程；原題規則固定，這是 callback
// 教學改寫，直接比較 value!=removed 會更簡潔且較利於 inline。
// 思路：write 指向下一個保留位置；逐項呼叫 keep；保留時覆寫 nums[write]，最後縮短 vector。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 nums 原長度；函式會原地改動容器。
// 易錯點：keep 不得為 null；只能保證前 K 個元素有效，這個示範額外 resize，與題目容許的尾段不同。
// -----------------------------------------------------------------------------
using KeepPredicate = bool (*)(int value, int removed);

bool keep_not_equal(int value, int removed) { return value != removed; }

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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】C API 事件 callback 橋接
// 情境：舊式 C API 發出整數事件，callback 需把事件寫入呼叫端提供的狀態物件。
// 為何使用本章主題：無捕獲 lambda 可轉成 CCallback 函式指標，狀態另走 void* context；
// 相較 std::function，介面 ABI 簡單且不需 type erasure，但型別與生命週期全由呼叫端維護。
// 設計：emit_event 檢查 callback；傳入 event 與 context；callback 將 context 轉回 int* 後寫值。
// 成本：每次事件時間 O(1)、額外空間 O(1)，只有一次間接函式呼叫，沒有配置。
// 上線注意：context 必須指向仍存活且型別正確的 int；非同步 API 還需同步寫入並防止重複釋放。
// -----------------------------------------------------------------------------
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

// 【延伸練習】替 C callback context 加明確 owner，證明 callback 被保存後不會指向已死 stack。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_function_pointer.cpp' -o '/tmp/codex_cpp_C_Lambda_07_function_pointer' && '/tmp/codex_cpp_C_Lambda_07_function_pointer'
//
// === 預期輸出（節錄）===
// function pointer：captureless conversion、Remove Element、C callback 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
