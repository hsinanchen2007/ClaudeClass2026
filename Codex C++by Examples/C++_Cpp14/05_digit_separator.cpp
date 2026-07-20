/*
 * C++14 教科書：digit separator（單引號）
 *
 * 數字 literal 可在 digits 之間放 `'` 提升可讀性，例如 1'000'000、0xFF'00、
 * 0b1111'0000。separator 不影響型別或數值，compiler 會忽略它；分組方式也無標準強制。
 *
 * 【規則】只能放在數字序列中合理位置，不能放開頭、結尾或緊鄰小數點/exponent marker。
 * 【suffix】型別仍由 literal 與 suffix 決定：4'000'000'000U 與沒有 U 的 signed 規則不同。
 * 【風格】十進位按千位、binary 按 nibble/field、hex 按 byte/word；團隊應一致。
 * 【陷阱】`'` 在其他位置也表示 character literal；錯置會造成難懂 parser error。
 * 【面試題】digit separator 是否有 runtime cost？沒有，tokenization 時就被處理。
 * 【練習】用 separators 表示 IPv4 mask 0xFFFFFF00 與 64 MiB bytes。
 */

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

namespace basic {
void demo() {
    const int users = 1'250'000;
    const std::uint32_t mask = 0xFF'FF'FF'00U;
    const double frequency = 2.4'000e9;
    assert(users == 1250000);
    assert(mask == 0xFFFFFF00U);
    assert(frequency == 2400000000.0);
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 9. Palindrome Number（回文數）
// 題目：不轉字串，判斷整數正反讀是否相同；12'321 為 true，-121 與 10 為 false。
// 為何使用本章主題：digit separator 只改善測試常數 12'321 的閱讀，不參與演算法，也不提供範圍檢查。
// 思路：1. 排除負數與非零尾數 0；2. 逐位建立 reversed_half；3. 比較偶數位或去除中位數後的兩半。
// 複雜度：D 為十進位位數；時間 O(D)、額外空間 O(1)。
// 易錯點：只反轉一半才能避開整數溢位；奇數位數比較時要用 reversed_half/10 忽略中央位。
// -----------------------------------------------------------------------------
bool leetcode_is_palindrome(int value) {
    if (value < 0 || (value % 10 == 0 && value != 0)) return false;
    int reversed_half = 0;
    while (value > reversed_half) {
        reversed_half = reversed_half * 10 + value % 10;
        value /= 10;
    }
    return value == reversed_half || value == reversed_half / 10;
}

void leetcode_test() {
    assert(leetcode_is_palindrome(12'321));
    assert(!leetcode_is_palindrome(-121));
    assert(!leetcode_is_palindrome(10));
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】資料傳輸速率與月配額
// 情境：設定每秒 125,000,000 bytes 與每月 750,000,000,000 bytes，判斷單次傳輸是否超額。
// 為何使用本章主題：digit separator 依千位分組大型常數，review 時較不易漏零；ULL suffix 才真正決定 64-bit 型別。
// 設計：1. 以 uint64_t 保存速率與月額度；2. 建立 policy；3. 比較請求 bytes 是否不超過 monthly_bytes。
// 成本：單次檢查時間與空間皆 O(1)。
// 上線注意：separator 不會檢查單位或溢位；實際配額需累計已使用量、處理併發更新與十進位/二進位 GB 定義。
// -----------------------------------------------------------------------------
struct TransferLimit {
    std::uint64_t bytes_per_second;
    std::uint64_t monthly_bytes;
};

bool practical_can_transfer(const TransferLimit& limit, std::uint64_t bytes) {
    return bytes <= limit.monthly_bytes;
}

void practical_test() {
    const TransferLimit policy{125'000'000ULL, 750'000'000'000ULL};
    assert(practical_can_transfer(policy, 50'000'000'000ULL));
    assert(!practical_can_transfer(policy, 800'000'000'000ULL));
    static_assert(std::numeric_limits<std::uint64_t>::digits >= 64,
                  "教材需要至少 64-bit uint64_t");
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "digit separator：數值可讀性、Palindrome Number、傳輸配額測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++14 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_digit_separator.cpp' -o '/tmp/codex_cpp_C_Cpp14_05_digit_separator' && '/tmp/codex_cpp_C_Cpp14_05_digit_separator'
//
// === 預期輸出（節錄）===
// digit separator：數值可讀性、Palindrome Number、傳輸配額測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
