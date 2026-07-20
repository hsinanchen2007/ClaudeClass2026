// ============================================================================
// 課題 1：位元運算子 &, |, ^, ~, <<, >>
// ============================================================================
//
// 整數在 memory/register 中由 bits 組成。位元運算逐 bit 處理：
//   a & b：兩者皆 1 才 1，常用來 mask/測試。
//   a | b：任一為 1 就 1，常用來設定旗標。
//   a ^ b：不同才 1，常用來切換旗標、找成對資料中的單一值。
//   ~a：全部反轉；記得結果寬度等於 promoted operand 的型別寬度。
//   a << n / a >> n：左/右移 n bits。
//
// 位元教材優先使用 unsigned：signed 負數右移具 implementation-defined 歷史語意，
// signed 左移 overflow 是 undefined behavior；shift count 若 >= type width 也是 UB。
// 運算子 precedence 容易誤判，條件中請明寫 `(flags & mask) != 0U`。
//
// 【面試】`&&/||` 是 logical、會 short-circuit；`&/|` 是 bitwise、不 short-circuit。
// ============================================================================

#include <cassert>
#include <cstdint>
#include <iostream>

void basic_example()
{
    const std::uint8_t left = 0b1100U;
    const std::uint8_t right = 0b1010U;
    assert(static_cast<unsigned>(left & right) == 0b1000U);
    assert(static_cast<unsigned>(left | right) == 0b1110U);
    assert(static_cast<unsigned>(left ^ right) == 0b0110U);

    const std::uint32_t bit_five = std::uint32_t{1} << 5U;
    assert(bit_five == 32U);
    assert((bit_five >> 5U) == 1U);
    std::cout << "[基礎] AND=8 OR=14 XOR=6，bit5=32\n";
}

// LeetCode 231：Power of Two。
// 2 的冪在 binary 中恰有一個 1；n & (n-1) 會清掉最低的 1。
bool is_power_of_two(int value)
{
    if (value <= 0) return false;
    const auto bits = static_cast<std::uint32_t>(value);
    return (bits & (bits - 1U)) == 0U;
}

void leetcode_231_example()
{
    assert(is_power_of_two(1));
    assert(is_power_of_two(16));
    assert(!is_power_of_two(3));
    assert(!is_power_of_two(0));
    std::cout << "[LeetCode 231] 1/16=true，3/0=false\n";
}

// 實務案例：Unix-like 權限 bit mask；不需要為每個組合建立獨立 bool 欄位。
enum Permission : std::uint8_t {
    read_permission = 1U << 0U,
    write_permission = 1U << 1U,
    execute_permission = 1U << 2U
};

void practical_example()
{
    std::uint8_t permissions = read_permission | write_permission;
    assert((permissions & read_permission) != 0U);
    assert((permissions & execute_permission) == 0U);
    permissions |= execute_permission;             // set
    permissions &= static_cast<std::uint8_t>(~write_permission); // clear
    assert((permissions & execute_permission) != 0U);
    assert((permissions & write_permission) == 0U);
    std::cout << "[實務] permission mask 最後為 read+execute\n";
}

int main()
{
    basic_example();
    leetcode_231_example();
    practical_example();
}

// 易錯：位移量不得為負或大於等於 promoted type 的 bit width；signed overflow 也不是
// 合法的 bit trick。面試與實務都應先把 wire/register 值放進明確寬度的 unsigned 型別。

// 練習：寫 set_bit/clear_bit/toggle_bit/test_bit，並拒絕超過 31 的 index。
// 複雜度與生命週期：固定寬度整數的單次 &,|,^,~,shift 是 O(1) 值運算；結果不借用 operand。
