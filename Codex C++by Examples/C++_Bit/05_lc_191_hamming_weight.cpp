// ============================================================================
// LeetCode 191：Number of 1 Bits（Hamming weight）
// ============================================================================
//
// Hamming weight 是 bit pattern 中 1 的數量。三種常見解法：
//   1. 固定掃 type width，每次 `(n >> bit) & 1`：O(W)。
//   2. Brian Kernighan：反覆 `n &= n-1`：O(K)，K 是 set bits 數。
//   3. C++20 `std::popcount`：語意最清楚，compiler 常映射硬體 POPCNT。
//
// 題目參數是 uint32_t 很重要；若用 signed int 並反覆右移負數，可能永遠補 1 或有
// implementation-defined 行為。Hamming distance(x,y) 就是 popcount(x^y)。
// ============================================================================

#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int hamming_weight_kernighan(std::uint32_t value)
{
    int count = 0;
    while (value != 0U) {
        value &= value - 1U;
        ++count;
    }
    return count;
}

int hamming_weight(std::uint32_t value)
{
    return std::popcount(value);
}

// 基礎示範：Kernighan 每回合只移除最低的一個 set bit。
void basic_example()
{
    std::uint32_t value = 0b10110000U;
    value &= value - 1U;
    assert(value == 0b10100000U);
    value &= value - 1U;
    assert(value == 0b10000000U);
    assert(hamming_weight_kernighan(0b10110000U) == 3);
    assert(hamming_weight(0b10110000U) == 3);
    std::cout << "[基礎] n&(n-1) 每次清除最低 set bit\n";
}

void leetcode_example()
{
    assert(hamming_weight(0b1011U) == 3);
    assert(hamming_weight(0xFFFFFFFDU) == 31);
    assert(hamming_weight_kernighan(0xFFFFFFFDU) == 31);
    assert(hamming_weight(0U) == 0);
    std::cout << "[LeetCode 191] examples: 3, 31, 0\n";
}

// 實務：權限/feature mask 的啟用功能數量，可用於方案限制或 diagnostics。
int enabled_feature_count(std::uint32_t feature_mask)
{
    return std::popcount(feature_mask);
}

void practical_example()
{
    const std::uint32_t enabled = (1U << 0U) | (1U << 4U) | (1U << 9U);
    assert(enabled_feature_count(enabled) == 3);
    const std::uint32_t changed = 0b101101U ^ 0b111001U;
    assert(std::popcount(changed) == 2); // 兩個 feature states 不同。
    std::cout << "[實務] enabled features=3，兩份設定差 2 bits\n";
}

int main()
{
    basic_example();
    leetcode_example();
    practical_example();
}

// 易錯：`value - 1` 必須在 unsigned domain；signed minimum 再減一會 overflow，屬 UB。
// 面試延伸：Kernighan 法迴圈次數等於 set-bit 數 K；固定掃描是 O(W)，在 sparse mask
// 時前者少做很多步。std::popcount 接受 unsigned integer type，不要先做危險 signed cast。
//
// 工作選擇：優先 std::popcount 表達意圖；只有受限於舊標準、特殊 SIMD/vectorization，
// 或需教學演算法時才手寫。benchmark 必須包含不同 bit density，不能只測單一輸入。
// 生命週期：兩個函式都只接收 value copy、沒有保存指標或狀態，呼叫結束後沒有懸空風險。
// 面試延伸：若 CPU 不支援 POPCNT，compiler/runtime library 可能如何實作？
