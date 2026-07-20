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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 191. Number of 1 Bits（二進位中 1 的個數）
// 題目：輸入 32-bit unsigned 整數，回傳 set bit 數；二進位 ...1011 的答案是 3。
// 為何使用本章主題：binary literal 讓 32-bit 測資的位元位置可直接審查；Kernighan 式則逐次清掉最低 set bit。
// 思路：1. count 從 0 開始；2. 每輪執行 value&=value-1；3. value 歸零時回傳輪數。
// 複雜度：K 為 set bit 數；時間 O(K)、額外空間 O(1)。
// 易錯點：必須在 unsigned domain 運算；binary literal 的 U suffix 與 32-bit 位數不可省略或寫錯。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】服務權限遮罩驗證
// 情境：read、write、execute 各占一個 bit，要判斷帳號是否同時具備某組必要權限。
// 為何使用本章主題：0b001/010/100 直接顯示欄位配置，比十進位 magic number 更容易審查。
// 設計：1. 以單一 bit 定義各 Permission；2. OR 組合 granted/required；3. 比較 granted&required 是否等於 required。
// 成本：單次組合與查詢時間、空間皆 O(1)。
// 上線注意：外部 mask 要拒絕未知 bits；uint8_t 會 integer promotion，組合後需明確轉回目的型別。
// -----------------------------------------------------------------------------
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
