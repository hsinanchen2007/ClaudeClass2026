/*
 * C++14 教科書：binary literals（0b / 0B）
 *
 * `0b1010` 讓 bit pattern 直接出現在 source，適合 register mask、protocol flags、權限位。
 * 它與十六進位/十進位只是同一整數的不同拼法，不會建立特殊「binary 型別」。
 * 搭配 unsigned 型別及 digit separator 可避免 signed shift、位數看錯等問題。
 *
 * 【位元操作】設定位 `x |= mask`、清除 `x &= ~mask`、切換 `x ^= mask`、測試
 * `(x & mask) != 0`。對 signed 負數 shift 或移位超過寬度可能是 UB/implementation-defined。
 * 【選擇】長達 32/64 bit 的 mask 常以 hex 更緊湊；欄位切割教學則 binary 更直觀。
 * 【常見陷阱】signed shift、移位超過型別寬度或忘記 unsigned suffix 可能導致 UB。
 * 【面試題】`1 << 31` 為何危險？1 是 signed int；應視需求用 `1U << 31`。
 */

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace basic {
constexpr std::uint8_t read_flag = 0b0000'0001;
constexpr std::uint8_t write_flag = 0b0000'0010;

void demo() {
    std::uint8_t permissions = read_flag;
    permissions = static_cast<std::uint8_t>(permissions | write_flag);
    assert((permissions & read_flag) != 0U);
    assert((permissions & write_flag) != 0U);
}
}  // namespace basic

namespace leetcode {
// LeetCode 191：Number of 1 Bits。Brian Kernighan：每輪清掉最低 set bit。
// 複雜度 O(k)，k 是 1-bit 數量；空間 O(1)。
int leetcode_hamming_weight(std::uint32_t value) {
    int count = 0;
    while (value != 0U) {
        value &= value - 1U;
        ++count;
    }
    return count;
}

void leetcode_test() {
    assert(leetcode_hamming_weight(0b0000'0000'0000'0000'0000'0000'0000'1011U) == 3);
    assert(leetcode_hamming_weight(0b1000'0000'0000'0000'0000'0000'0000'0000U) == 1);
}
}  // namespace leetcode

// 【實務案例】權限 mask：二進位 literal 讓 read/write/execute 所占 bit 一眼可見。
namespace practical {
enum Permission : std::uint8_t {
    read = 0b001,
    write = 0b010,
    execute = 0b100
};

bool practical_has_all(std::uint8_t granted, std::uint8_t required) {
    return (granted & required) == required;
}

void practical_test() {
    const std::uint8_t developer = static_cast<std::uint8_t>(read | write);
    assert(practical_has_all(developer, read));
    assert(practical_has_all(developer, static_cast<std::uint8_t>(read | write)));
    assert(!practical_has_all(developer, execute));
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "binary literal：mask、Hamming Weight、權限測試通過\n";
}

// 【延伸練習】新增 admin mask，實作 has_any/has_all/remove，並避免 integer promotion 誤判。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++14 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_binary_literals.cpp' -o '/tmp/codex_cpp_C_Cpp14_04_binary_literals' && '/tmp/codex_cpp_C_Cpp14_04_binary_literals'
//
// === 預期輸出（節錄）===
// binary literal：mask、Hamming Weight、權限測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
