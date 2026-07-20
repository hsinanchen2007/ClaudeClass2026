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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 231. Power of Two（判斷是否為 2 的冪）
// 題目：輸入整數 value，判斷是否存在 k 使 value=2^k；16 為 true，3、0 為 false。
// 為何使用本章主題：正的 2 次冪恰有一個 set bit，`bits & (bits-1)` 可一次清除它。
// 思路：1. 先排除 value<=0；2. 轉成 uint32_t；3. 檢查清除最低 set bit 後是否為 0。
// 複雜度：固定寬度整數只做常數次位元運算，時間 O(1)、額外空間 O(1)。
// 易錯點：若未先排除 0，公式會誤判；signed 負值不在題意，位元式應在 unsigned domain 執行。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】Unix-like 權限遮罩維護
// 情境：單一 byte 同時保存 read、write、execute 權限，並支援查詢、授予與撤銷。
// 為何使用本章主題：每個權限占一個 bit，&、|、~ 可取代多個彼此獨立的 bool 欄位與組合分支。
// 設計：1. 以 shift 定義三個 masks；2. `|=` 授予 execute；3. `&=~mask` 撤銷 write 並以 & 查詢。
// 成本：每次權限操作時間與空間皆 O(1)。
// 上線注意：~ 會發生 integer promotion，需轉回 uint8_t；外部輸入還要拒絕未知 bits 並集中授權紀錄。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_basic_operators.cpp' -o '/tmp/codex_cpp_C_Bit_01_basic_operators' && '/tmp/codex_cpp_C_Bit_01_basic_operators'
//
// === 預期輸出（節錄）===
// [實務] permission mask 最後為 read+execute
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
