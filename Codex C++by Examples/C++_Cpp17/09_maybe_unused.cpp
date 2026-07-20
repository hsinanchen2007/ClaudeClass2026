/*
 * C++17 教科書：[[maybe_unused]]
 *
 * 此 attribute 告訴 compiler 某 entity 在部分 build configuration 合理地未使用，可標在
 * variable、function、type、enumerator 等。標準對 label 的支援是較晚的修訂（C++26
 * wording；部分 GCC/Clang 在較舊 `-std` 模式已提前接受），不要把實作延伸當成 C++17 保證。
 *
 * 【適用】assert-only 變數在 NDEBUG 消失、平台條件參數、debug instrumentation。
 * 【不適用】不要用它掩蓋真正 dead code、拼錯名稱或忘了呼叫。先理解為何未使用。
 * 【替代】對單一 expression 可用 `static_cast<void>(value)`，語意通常更局部明確。
 * 【面試題】[[maybe_unused]] 是否保證 compiler 移除變數？不保證，僅允許不警告。
 * 【練習】用 compile definition 控制 practical_trace 是否輸出，維持兩種 build 無 warning。
 */

#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace basic {
[[maybe_unused]] constexpr int diagnostic_build_id = 20260719;

int checked_double(int value) {
    if (value > std::numeric_limits<int>::max() / 2 ||
        value < std::numeric_limits<int>::min() / 2) {
        throw std::overflow_error("checked_double overflow");
    }
    return value * 2;
}

void demo() {
    assert(checked_double(5) == 10);
    for (const int boundary : {std::numeric_limits<int>::max(),
                               std::numeric_limits<int>::min()}) {
        bool rejected = false;
        try {
            static_cast<void>(checked_double(boundary));
        } catch (const std::overflow_error&) {
            rejected = true;
        }
        assert(rejected);
    }
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 167. Two Sum II - Input Array Is Sorted（排序陣列兩數之和）
// 題目：在遞增陣列找和為 target 的兩個 1-based 索引；[2,7,11,15] 與 9 回傳 (1,2)。
// 為何使用本章主題：comparisons 是可選診斷計數，[[maybe_unused]] 允許不同 build 不輸出它而不產生警告。
// 思路：1. 左右指標從兩端開始；2. 和太小移左、太大移右；3. 相等時轉成 1-based 回傳。
// 複雜度：N 為元素數；時間 O(N)、額外空間 O(1)。
// 易錯點：輸入必須排序且至少兩項；以 long long 計算和避免 int 溢位，attribute 不應掩蓋真正 dead code。
// -----------------------------------------------------------------------------
std::pair<int, int> leetcode_two_sum_sorted(const std::vector<int>& numbers, int target) {
    if (numbers.size() < 2U) return {-1, -1};
    std::size_t left = 0U;
    std::size_t right = numbers.size() - 1U;
    [[maybe_unused]] std::size_t comparisons = 0U;  // debug build 可輸出此統計。
    while (left < right) {
        ++comparisons;
        const long long sum = static_cast<long long>(numbers[left]) + numbers[right];
        if (sum == static_cast<long long>(target)) {
            return {static_cast<int>(left + 1U), static_cast<int>(right + 1U)};
        }
        if (sum < static_cast<long long>(target)) ++left;
        else --right;
    }
    return {-1, -1};
}

void leetcode_test() {
    assert((leetcode_two_sum_sorted({2, 7, 11, 15}, 9) == std::pair<int, int>{1, 2}));
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】跨建置模式的 trace 參數
// 情境：debug build 可能記錄 trace_id，production build 關閉追蹤但要維持相同 process API。
// 為何使用本章主題：參數上的 [[maybe_unused]] 表明某些組態刻意不用 trace_id，而非靠移除參數改變 ABI。
// 設計：1. 保留 payload 與 trace_id 簽章；2. production 只處理 payload；3. 回傳 payload byte 數的教材結果。
// 成本：目前 size() 時間 O(1)、額外空間 O(1)；真正 tracing 會增加格式化與 I/O 成本。
// 上線注意：attribute 不會保護敏感 trace ID 或消除資料；應以 build flag 控制行為並維持可觀測性政策。
// -----------------------------------------------------------------------------
int practical_process(const std::string& payload,
                      [[maybe_unused]] const std::string& trace_id) {
    // production build 可能完全不記 trace_id；parameter 仍保留以維持跨 build API 一致。
    return static_cast<int>(payload.size());
}

void practical_test() {
    assert(practical_process("event", "trace-42") == 5);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "maybe_unused：build 變體、Two Sum II、trace API 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_maybe_unused.cpp' -o '/tmp/codex_cpp_C_Cpp17_09_maybe_unused' && '/tmp/codex_cpp_C_Cpp17_09_maybe_unused'
//
// === 預期輸出（節錄）===
// maybe_unused：build 變體、Two Sum II、trace API 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
