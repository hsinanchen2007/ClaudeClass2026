/*
 * C++17 教科書：std::byte
 *
 * std::byte 是 enum class，專門表示 raw memory byte，而不是數字/字元。它支援 bitwise
 * operator 與 shift，但不支援算術；要看數值用 std::to_integer<T>。這能避免把 buffer
 * element 誤拿去加減，意圖比 unsigned char 清楚。
 *
 * 【邊界】byte 不解決 endian、alignment、object lifetime、strict aliasing 或 serialization。
 * 跨機格式要逐欄 encode/decode；不要直接把 struct bytes 傳上網或寫檔。
 * 【aliasing】char/unsigned char/std::byte 可檢視 object representation，但反向建構 object
 * 仍受 lifetime/alignment/trivially-copyable 規則限制。
 * 【常見陷阱】std::byte 不會處理 endian、padding 或 object lifetime，禁止直接 memcpy struct 當協定。
 * 【面試題】std::byte 為何不是 arithmetic type？阻止意外數值運算，表達 raw storage。
 * 【練習】替 practical_encode_header 加 decode 並檢查 magic/version。
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace basic {
void demo() {
    std::byte flags{0b0000'0011};
    flags &= std::byte{0b0000'0001};
    assert(std::to_integer<unsigned>(flags) == 1U);
    flags <<= 2;
    assert(std::to_integer<unsigned>(flags) == 4U);
}
}  // namespace basic

namespace leetcode {
// LeetCode 191：Number of 1 Bits。以四個 byte 表達 32-bit input，逐 byte 計數。
int count_byte(std::byte value) {
    unsigned number = std::to_integer<unsigned>(value);
    int count = 0;
    while (number != 0U) {
        number &= number - 1U;
        ++count;
    }
    return count;
}

int leetcode_hamming_weight(const std::array<std::byte, 4>& bytes) {
    int total = 0;
    for (const std::byte value : bytes) total += count_byte(value);
    return total;
}

void leetcode_test() {
    const std::array<std::byte, 4> value{
        std::byte{0b0000'1011}, std::byte{0}, std::byte{0}, std::byte{0}};
    assert(leetcode_hamming_weight(value) == 3);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
struct Header {
    std::uint8_t version;
    std::uint16_t length;
};

// 實務：顯式 big-endian encode，不 memcpy struct（避免 padding/endian/ABI 問題）。
std::array<std::byte, 3> practical_encode_header(const Header& header) {
    return {std::byte{header.version},
            std::byte{static_cast<std::uint8_t>((header.length >> 8U) & 0xFFU)},
            std::byte{static_cast<std::uint8_t>(header.length & 0xFFU)}};
}

void practical_test() {
    const auto bytes = practical_encode_header(Header{2U, 0x1234U});
    assert(std::to_integer<unsigned>(bytes[0]) == 2U);
    assert(std::to_integer<unsigned>(bytes[1]) == 0x12U);
    assert(std::to_integer<unsigned>(bytes[2]) == 0x34U);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "std::byte：raw storage、Hamming Weight、packet encoding 測試通過\n";
}
