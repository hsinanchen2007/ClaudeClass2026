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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 191. Number of 1 Bits（二進位中 1 的個數）
// 題目：計算 32-bit unsigned 值的 set bit 數；...1011 的答案為 3。
// 為何使用本章主題：本檔將官方 uint32_t 教學改寫成四個 std::byte，展示 raw byte 必須先 to_integer 才能做數值計數。
// 思路：1. 每個 byte 轉 unsigned；2. 以 n&=n-1 計數該 byte；3. 累加四個 byte 的結果。
// 複雜度：K 為四個 byte 中的 set bit 總數；時間 O(K)、額外空間 O(1)。
// 易錯點：byte order 不影響總 bit 數但會影響其他數值解讀；std::byte 不能直接做算術，輸入也必須恰有四格。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】三 byte 協定 header 編碼
// 情境：將 8-bit version 與 16-bit length 編成固定三 byte 的 big-endian wire format。
// 為何使用本章主題：std::byte 表達 raw wire storage，逐欄 shift/cast 避免把 struct padding、ABI 或 native endian 當協定。
// 設計：1. 第一格放 version；2. length 高 8 bits 放第二格；3. 低 8 bits 放第三格。
// 成本：固定三格，時間與輸出空間皆 O(1)。
// 上線注意：還需實作驗證過的 decode、版本/長度上限與封包完整性；不可直接 memcpy Header。
// -----------------------------------------------------------------------------
struct Header {
    std::uint8_t version;
    std::uint16_t length;
};

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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '12_byte.cpp' -o '/tmp/codex_cpp_C_Cpp17_12_byte' && '/tmp/codex_cpp_C_Cpp17_12_byte'
//
// === 預期輸出（節錄）===
// std::byte：raw storage、Hamming Weight、packet encoding 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
