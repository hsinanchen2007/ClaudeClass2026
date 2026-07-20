// ============================================================================
// LeetCode 231：Power of Two
// ============================================================================
//
// 正整數若為 2^k，binary 恰有一個 set bit；`n & (n-1)` 會清掉最低 set bit，因此結果
// 為 0。必須先檢查 n>0：0 沒有 set bit 卻也讓式子為 0；負 signed 數更不是題意。
// C++20 可直接 `std::has_single_bit(unsigned_value)`，名稱比 magic expression 清楚。
//
// 不建議用 floating log2 判斷整數性，因 rounding/precision 會在大值邊界出錯。
// ============================================================================

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>

bool is_power_of_two(int value)
{
    return value > 0 && std::has_single_bit(static_cast<std::uint32_t>(value));
}

// 基礎示範：正的 2 次冪只有一個 set bit，清除最低 set bit 後必為 0。
void basic_example()
{
    const std::uint32_t value = 16U;
    assert((value & (value - 1U)) == 0U);
    assert(std::has_single_bit(value));
    assert(!std::has_single_bit(12U));
    std::cout << "[基礎] 16=10000b，只有一個 set bit\n";
}

void leetcode_example()
{
    assert(is_power_of_two(1));
    assert(is_power_of_two(16));
    assert(!is_power_of_two(3));
    assert(!is_power_of_two(0));
    assert(!is_power_of_two(-2));
    std::cout << "[LeetCode 231] only positive single-bit values pass\n";
}

// 實務：ring buffer 用 bit-mask 取代 modulo 的前提是 capacity 為 2 的冪。
std::optional<std::size_t> ring_index(std::size_t sequence, std::size_t capacity)
{
    if (!std::has_single_bit(capacity)) return std::nullopt;
    return sequence & (capacity - 1U);
}

void practical_example()
{
    assert(ring_index(10U, 8U).value() == 2U);
    assert(ring_index(15U, 8U).value() == 7U);
    assert(!ring_index(10U, 6U).has_value());
    std::cout << "[實務] sequence 10 in capacity 8 -> slot 2\n";
}

int main()
{
    basic_example();
    leetcode_example();
    practical_example();
}

// 易錯與面試：
//   * 一定先排除 0；否則 `0 & (0-1)` 在 unsigned 也會得到 0 而誤判。
//   * signed 負數的 representation/轉型不是題意，先以 value>0 封住 domain。
//   * `capacity-1` mask 只在 capacity 是 2 的冪時等價 modulo；本例用 optional 拒絕。
//
// 相關 C++20 API：has_single_bit 判斷、bit_floor 找不大於 x 的 2 次冪、bit_ceil 找不小於
// x 的 2 次冪。bit_ceil 超出回傳型別可表示範圍時有前置條件，配置前仍要做上界檢查。
//
// 面試追問：1 是否為 2 的冪？是，1=2^0；這是數學定義與常見題目契約，不應排除。
// 工作上 ring buffer 還需處理 producer/consumer synchronization，power-of-two 只解 index。
// 複雜度與生命週期：判斷及 mask index 都是 O(1)，純值運算且不保存任何借用資料。
// API 選型：已有 C++20 時優先 has_single_bit；手寫 expression 則必須把 `n>0` 放在契約中。
// 練習：LC 342 Power of Four 除了單一 bit，還要限制 bit 只出現在偶數位置。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_lc_231_power_of_two.cpp' -o '/tmp/codex_cpp_C_Bit_09_lc_231_power_of_two' && '/tmp/codex_cpp_C_Bit_09_lc_231_power_of_two'
//
// === 預期輸出（節錄）===
// [實務] sequence 10 in capacity 8 -> slot 2
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
