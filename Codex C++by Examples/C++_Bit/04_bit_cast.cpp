// ============================================================================
// 課題 4：std::bit_cast、安全查看 object representation
// ============================================================================
//
// std::bit_cast<To>(from) 逐 bit 複製 object representation，要求 sizeof(To)==sizeof(From)
// 且兩者 trivially copyable。它不做 numeric conversion：float 1.0F bit_cast 成 uint32_t
// 得 IEEE-754 bits，而 static_cast<uint32_t>(1.0F) 得數值 1。
//
// 相較 `reinterpret_cast<T&>` type-punning，bit_cast 不違反 strict aliasing/alignment。
// 但「能安全讀 bits」不代表內容具 portable 意義：padding、endianness、floating format、
// trap/indeterminate representation 都需考慮。wire format 應逐欄明定編碼，不直接 dump
// 任意 struct memory。
//
// 【陷阱】bit_cast 不取代 serialization，也不改 byte order。
// ============================================================================

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>

void basic_example()
{
    const float value = 1.0F;
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    assert(bits == 0x3F800000U); // 本教材目標平台採 IEEE-754 binary32。
    assert(std::bit_cast<float>(bits) == 1.0F);
    std::cout << "[基礎] float 1.0 representation=0x" << std::hex << bits << std::dec << '\n';
}

// LeetCode 190：Reverse Bits。bit_cast 不是解題必要條件；這裡用 bytes 顯示反轉前後
// representation，再以 shift 完成 32-bit reversal。
std::uint32_t reverse_bits(std::uint32_t value)
{
    std::uint32_t result = 0U;
    for (unsigned bit = 0U; bit < 32U; ++bit) {
        result = (result << 1U) | (value & 1U);
        value >>= 1U;
    }
    return result;
}

void leetcode_190_example()
{
    const std::uint32_t input = 0b00000010100101000001111010011100U;
    const std::uint32_t output = reverse_bits(input);
    assert(output == 964'176'192U);
    const auto bytes = std::bit_cast<std::array<std::byte, 4>>(output);
    assert(bytes.size() == 4U);
    std::cout << "[LeetCode 190] reversed value=" << output << '\n';
}

// 實務案例：把 32-bit float 明確編成 big-endian bytes。
std::array<std::uint8_t, 4> encode_float_be(float value)
{
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    return {
        static_cast<std::uint8_t>((bits >> 24U) & 0xFFU),
        static_cast<std::uint8_t>((bits >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((bits >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(bits & 0xFFU)
    };
}

void practical_example()
{
    const auto encoded = encode_float_be(1.0F);
    assert((encoded == std::array<std::uint8_t, 4>{0x3FU, 0x80U, 0x00U, 0x00U}));
    std::cout << "[實務] float 1.0 big-endian bytes=3f 80 00 00\n";
}

int main()
{
    basic_example();
    leetcode_190_example();
    practical_example();
}

// 練習：寫 decode_float_be，先重組 uint32_t 再 bit_cast<float>。
// 複雜度與生命週期：bit_cast 複製 sizeof(T) 個 representation bits，回傳全新的 value；
// 它不建立指向來源的 alias，因此來源離開生命週期後，結果仍是獨立物件。
