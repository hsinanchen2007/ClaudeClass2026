// ============================================================================
// LeetCode 338：Counting Bits
// ============================================================================
//
// 要求回傳 0..n 每個數的 popcount。逐數逐 bit 可 O(N log N)；動態規劃利用：
//   bits[x] = bits[x >> 1] + (x & 1)
// 因為右移移除最低 bit，而 x>>1 一定小於 x，前面的答案已算好。另一 recurrence 是
// `bits[x] = bits[x & (x-1)] + 1`。
//
// 結果長度 n+1；先拒絕負 n，並在 int→size_t 前驗證，避免負數轉成巨大 allocation。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>

std::vector<int> count_bits(int n)
{
    if (n < 0) throw std::invalid_argument("n must be nonnegative");
    const std::size_t limit = static_cast<std::size_t>(n);
    std::vector<int> result(limit + 1U, 0);
    for (std::size_t value = 1U; value <= limit; ++value) {
        result.at(value) = result.at(value >> 1U) + static_cast<int>(value & 1U);
    }
    return result;
}

// 基礎示範：右移一位等於拿掉最低 bit，因此可重用較小數字的答案。
void basic_example()
{
    const auto table = count_bits(7);
    assert(table.at(6U) == table.at(3U));      // 110 -> 11，最低 bit 是 0。
    assert(table.at(7U) == table.at(3U) + 1); // 111 -> 11，再加最低的 1。
    assert(table.size() == 8U);
    std::cout << "[基礎] bits[x]=bits[x>>1]+(x&1)\n";
}

void leetcode_example()
{
    assert((count_bits(2) == std::vector<int>{0, 1, 1}));
    assert((count_bits(5) == std::vector<int>{0, 1, 1, 2, 1, 2}));
    assert((count_bits(0) == std::vector<int>{0}));
    std::cout << "[LeetCode 338] n=5 -> 0,1,1,2,1,2\n";
}

// 實務：預建 byte popcount lookup table，之後大量資料可 O(1) 查每 byte 的 1 數。
std::vector<int> make_byte_popcount_table()
{
    return count_bits(255);
}

void practical_example()
{
    const auto table = make_byte_popcount_table();
    assert(table.size() == 256U);
    assert(table.at(0xF0U) == 4);
    assert(table.at(0xFFU) == 8);
    std::cout << "[實務] byte lookup: F0=4, FF=8\n";
}

int main()
{
    basic_example();
    leetcode_example();
    practical_example();
}

// 易錯與面試：
//   * output 本來就有 N+1 個答案，所以即使 recurrence 是 O(N)，空間也至少 O(N)。
//   * `value >> 1` 與 `value & (value-1)` 都引用較小 index，才能由左到右 DP。
//   * 不要每次呼叫 std::bitset<32>(x).to_string() 再數字元；那建立不必要字串。
//
// 工作應用的 byte lookup table 只有 256 entries，可重用；若硬體已有 popcount 指令，
// 大量 64-bit 資料直接 std::popcount 可能更快，應用 benchmark 而非假設 table 一定贏。
// 生命週期：回傳 vector 以值擁有 table；caller 可安全保存，函式內沒有逸出的暫存 reference。
// 面試追問：若只要單一 n 的答案，為何不該建立 0..n 整張 DP table？
// 練習：改用 x&(x-1) recurrence，比較產生的 table 是否逐項相同。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_lc_338_counting_bits.cpp' -o '/tmp/codex_cpp_C_Bit_08_lc_338_counting_bits' && '/tmp/codex_cpp_C_Bit_08_lc_338_counting_bits'
//
// === 預期輸出（節錄）===
// [實務] byte lookup: F0=4, FF=8
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
