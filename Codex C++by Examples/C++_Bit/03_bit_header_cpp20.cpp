// ============================================================================
// 課題 3：C++20 <bit> 標準工具
// ============================================================================
//
// `<bit>` 提供具名、可讀且通常映射 CPU instruction 的 unsigned integer operations：
//   popcount              set bits 數量
//   countl_zero/countr_zero 前/後方連續 0（輸入 0 有標準定義，結果為 bit width）
//   bit_width             表示值需要幾 bits（0 -> 0）
//   has_single_bit        是否為 2 的冪
//   bit_floor/bit_ceil    不大於/不小於值的 2 次冪
//   rotl/rotr             rotation，不會像 shift 丟掉移出的 bits
//   endian                host byte order
//
// 這些函式只接受 unsigned integer types。bit_ceil 若結果無法由 type 表示，行為未定義，
// 所以外部輸入仍要先做範圍檢查。
// ============================================================================

#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>

void basic_example()
{
    const std::uint32_t value = 0b00101000U;
    assert(std::popcount(value) == 2);
    assert(std::countr_zero(value) == 3);
    assert(std::bit_width(value) == 6);
    assert(std::bit_floor(value) == 32U);
    assert(std::bit_ceil(value) == 64U);
    assert(std::rotl(std::uint8_t{0b10000001U}, 1) == std::uint8_t{0b00000011U});
    std::cout << "[基礎] popcount=2 bit_width=6 ceil=64\n";
}

// LeetCode 191：Number of 1 Bits。C++20 直接表達題意。
int hamming_weight(std::uint32_t value)
{
    return std::popcount(value);
}

void leetcode_191_example()
{
    assert(hamming_weight(0b00000000000000000000000000001011U) == 3);
    assert(hamming_weight(0xFFFFFFFDU) == 31);
    std::cout << "[LeetCode 191] std::popcount answers 3 and 31\n";
}

// 實務案例：配置 hash table 容量為至少 requested 的 2 次冪。
std::optional<std::uint32_t> hash_capacity(std::uint32_t requested)
{
    if (requested <= 1U) return 1U;
    constexpr std::uint32_t highest_power = std::uint32_t{1} << 31U;
    if (requested > highest_power) return std::nullopt;
    return std::bit_ceil(requested);
}

void practical_example()
{
    assert(hash_capacity(1'000U).value() == 1'024U);
    assert(hash_capacity(1'024U).value() == 1'024U);
    assert(!hash_capacity(std::numeric_limits<std::uint32_t>::max()).has_value());
    std::cout << "[實務] requested 1000 -> hash capacity 1024\n";
}

int main()
{
    basic_example();
    leetcode_191_example();
    practical_example();
}

// 易錯與面試：<bit> 多數函式只接受 unsigned integer；先確認 domain，再轉型。bit_ceil
// 若數學結果超過回傳型別可表示範圍，不能把 wraparound 當合法配置容量。

// 成本與生命週期：這些 scalar API 都是 constexpr 值運算，沒有 allocation，也不保存
// reference；一般可視為 O(1)，但不能由 API 契約保證一定對應單一 CPU instruction。
// `std::endian::native` 描述編譯目標的 native byte order，不會在 runtime 幫資料換 endian。
// 面試時應說明 rotation 與 shift 的差別：rotation 把移出的 bits 繞回，shift 則丟棄。
// 練習：用 std::countr_zero 找 power-of-two alignment 對應的 exponent。
