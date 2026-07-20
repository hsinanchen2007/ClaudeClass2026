// ============================================================================
// 課題 2：常見 bit tricks 與適用邊界
// ============================================================================
//
// 常見恆等式（x 為 unsigned）：
//   x & (x-1)  清除最低 set bit。
//   x & (~x+1) 取出最低 set bit（two's complement；也常寫 x & -x）。
//   x ^ x = 0、x ^ 0 = x，且 XOR 可交換/結合。
//   (value + alignment-1) & ~(alignment-1) 對齊到 2 的冪，但加法可能 overflow。
//
// trick 不是目的；可讀性與前置條件更重要。C++20 已有 std::popcount、has_single_bit、
// bit_ceil 等具名 API，能用就比手寫魔術式更清楚。
//
// 【陷阱】XOR swap 雖不用暫存變數，但 alias (`swap(x,x)`) 會清零且通常比 std::swap
// 更難讀；production code 不應使用。
// ============================================================================

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

unsigned count_set_bits(std::uint32_t value)
{
    unsigned count = 0U;
    while (value != 0U) {
        value &= value - 1U;
        ++count;
    }
    return count;
}

std::uint32_t lowest_set_bit(std::uint32_t value)
{
    return value & (~value + 1U);
}

void basic_example()
{
    assert(count_set_bits(0b10110100U) == 4U);
    assert(lowest_set_bit(0b10110100U) == 0b00000100U);
    assert(lowest_set_bit(0U) == 0U);
    std::cout << "[基礎] 0b10110100 有 4 個 1，lowbit=4\n";
}

// LeetCode 136：Single Number。每個元素兩次、只有答案一次；全部 XOR 即答案。
int single_number(const std::vector<int>& nums)
{
    int answer = 0;
    for (const int value : nums) answer ^= value;
    return answer;
}

void leetcode_136_example()
{
    assert(single_number({4, 1, 2, 1, 2}) == 4);
    assert(single_number({2, 2, -7}) == -7);
    std::cout << "[LeetCode 136] paired values XOR 後答案=4\n";
}

// 實務案例：安全的 power-of-two alignment，先檢查 alignment 與 overflow。
std::optional<std::uint32_t> align_up(std::uint32_t value, std::uint32_t alignment)
{
    if (alignment == 0U || (alignment & (alignment - 1U)) != 0U) return std::nullopt;
    const std::uint32_t add = alignment - 1U;
    if (value > std::numeric_limits<std::uint32_t>::max() - add) return std::nullopt;
    return (value + add) & ~add;
}

void practical_example()
{
    assert(align_up(13U, 8U).value() == 16U);
    assert(align_up(16U, 8U).value() == 16U);
    assert(!align_up(13U, 6U).has_value());
    assert(!align_up(std::numeric_limits<std::uint32_t>::max(), 8U).has_value());
    std::cout << "[實務] 13 對齊 8-byte boundary 得 16\n";
}

int main()
{
    basic_example();
    leetcode_136_example();
    practical_example();
}

// 練習：用 while(lowbits) 枚舉 mask 中每個 set bit 的 index。
// 複雜度與生命週期：單一 trick 為 O(1)，枚舉 set bits 為 O(K)；全程處理值副本、不保存借用。

/*
【本課面試問答】
Q1：`n & (n-1)` 的兩個常見用途與前提？
A：它清除最低 set bit，可 O(popcount) 枚舉；對 `n>0`，結果為 0 表示 power of two。若忘記排除
n==0，power-of-two 判斷會誤回 true。位元技巧優先使用 unsigned type。

Q2：signed integer 的 shift 為何危險？
A：負值右移與 signed representation 涉及版本/實作語意，左移若不可表示可能 UB；shift count 負數或
大於等於型別位數也不合法。位元遮罩使用固定寬 unsigned，並在 shift 前驗範圍。

Q3：`align_up(value,a)=(value+a-1)&~(a-1)` 何時正確？
A：a 必須為非零 power of two，而且 `value+a-1` 不得 overflow。一般 alignment 或外部輸入不能直接
套公式；production API 要驗 preconditions，或使用 checked arithmetic/標準配置工具。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_common_tricks.cpp' -o '/tmp/codex_cpp_C_Bit_02_common_tricks' && '/tmp/codex_cpp_C_Bit_02_common_tricks'
//
// === 預期輸出（節錄）===
// [實務] 13 對齊 8-byte boundary 得 16
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
