// ============================================================================
// 課題 6：C++20 std::bit_cast
// ============================================================================
//
// bit_cast<To>(from) 要求等大小且 trivially copyable，語意近似 compiler 最佳化的 memcpy。
// 它建立一個新的 To value，不是把同一 storage 當成另一型別的 reference，因此避開
// strict-aliasing UB。它不做 numeric conversion，也不交換 endian。
//
// 使用前問：To 的每個可能 bit pattern 是否都是有效值？對 integer/byte array 通常容易；
// 任意 pointer、含 padding/indeterminate bits 的 struct 或平台不保證的 float format 要更
// 小心。跨機 wire/storage format 應明確 encode fields，而非直接 bit_cast 整個 struct。
// ============================================================================

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>

void basic_example()
{
    const float value = -2.5F;
    const std::uint32_t representation = std::bit_cast<std::uint32_t>(value);
    assert(std::bit_cast<float>(representation) == value);
    std::cout << "[基礎] -2.5F round-trips through uint32 representation\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 191. Number of 1 Bits（二進位中 1 的個數）
// 題目：計算 32-bit unsigned 值中 set bit 數；0b1011 回傳 3，0xFFFFFFFD 回傳 31。
// 為何使用本章主題：正式介面直接 popcount；額外 adapter 以 bit_cast 保留 int32_t representation，誠實區分 bits 與 numeric conversion。
// 思路：1. uint32_t 直接 popcount；2. signed 教學輸入先 bit_cast<uint32_t>；3. 再對保留的 bits 計數。
// 複雜度：固定 32-bit 值，時間 O(1)、額外空間 O(1)。
// 易錯點：signed adapter 不是 LeetCode 必要解；bit_cast 要求同大小，且 -1 的 32 個 1 依賴 int32_t 表示契約。
// -----------------------------------------------------------------------------
int hamming_weight(std::uint32_t value)
{
    return std::popcount(value);
}

int representation_popcount(std::int32_t value)
{
    return std::popcount(std::bit_cast<std::uint32_t>(value));
}

void leetcode_191_example()
{
    assert(hamming_weight(0b1011U) == 3);
    assert(hamming_weight(0xFFFFFFFDU) == 31);
    assert(representation_popcount(-1) == 32); // 目標平台 int32_t 全 1 representation。
    std::cout << "[LeetCode 191] uint32 正式題解與 signed representation adapter 均驗證\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】浮點 wire format 編解碼
// 情境：把 float 編成四個 big-endian bytes，接收端再重組成完全相同的 float 值。
// 為何使用本章主題：bit_cast 在 float/uint32_t 間複製 representation；shift 明確處理 wire endian，避免 aliasing。
// 設計：1. encode 時 bit_cast 後拆高到低四 bytes；2. decode 時重組 uint32_t；3. 再 bit_cast 回 float。
// 成本：固定四 byte，encode/decode 時間與額外空間皆 O(1)。
// 上線注意：協定要保證 32-bit IEEE-754；NaN payload、版本與錯誤長度也需定義，bit_cast 不負責驗證。
// -----------------------------------------------------------------------------
std::array<std::uint8_t, 4> encode_float_big_endian(float value)
{
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    return {
        static_cast<std::uint8_t>((bits >> 24U) & 0xFFU),
        static_cast<std::uint8_t>((bits >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((bits >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(bits & 0xFFU)
    };
}

float decode_float_big_endian(const std::array<std::uint8_t, 4>& bytes)
{
    const std::uint32_t bits = (static_cast<std::uint32_t>(bytes[0]) << 24U) |
        (static_cast<std::uint32_t>(bytes[1]) << 16U) |
        (static_cast<std::uint32_t>(bytes[2]) << 8U) |
        static_cast<std::uint32_t>(bytes[3]);
    return std::bit_cast<float>(bits);
}

void practical_example()
{
    const auto wire = encode_float_big_endian(1.0F);
    assert((wire == std::array<std::uint8_t, 4>{0x3FU, 0x80U, 0U, 0U}));
    assert(decode_float_big_endian(wire) == 1.0F);
    std::cout << "[實務] float wire format=3f 80 00 00\n";
}

int main()
{
    basic_example();
    leetcode_191_example();
    practical_example();
}

// 易錯與面試：bit_cast 要求來源/目的大小相同且目的可 trivially copy；它保留 bits，
// 不負責 byte order、wire schema 或浮點格式。跨機器傳輸仍須明確 endian 與格式契約。
// 練習：加 static_assert(sizeof(float)==4)，並說明它仍不能單獨保證 IEEE-754。
// 複雜度與生命週期：成本與 sizeof(T) 成正比且常被最佳化成 register move；回傳值是新物件，
// 不像 reinterpret pointer 那樣依賴來源 storage 繼續存活。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_bit_cast_cpp20.cpp' -o '/tmp/codex_cpp_C_Cast_06_bit_cast_cpp20' && '/tmp/codex_cpp_C_Cast_06_bit_cast_cpp20'
//
// === 預期輸出（節錄）===
// [實務] float wire format=3f 80 00 00
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
