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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 191. Number of 1 Bits（二進位中 1 的個數）
// 題目：輸入 uint32_t，回傳其 bit pattern 中 1 的數量；...1011 回傳 3。
// 為何使用本章主題：C++20 std::popcount 直接命名題意，避免手寫迴圈與 signed shift 邊界。
// 思路：1. 保持輸入為 uint32_t；2. 交給 std::popcount；3. 將標準函式結果回傳為 int。
// 複雜度：固定 32-bit 輸入可視為時間 O(1)、額外空間 O(1)，實際指令由實作決定。
// 易錯點：std::popcount 只接受 unsigned integer type；不可假設所有目標 CPU 都必然用單一 POPCNT 指令。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】雜湊表容量取整
// 情境：將 requested bucket 數向上取成至少同樣大的 2 次冪，超出 uint32_t 可表示結果時拒絕。
// 為何使用本章主題：std::bit_ceil 清楚表達 power-of-two rounding，並以 has-single-bit 類契約取代 magic loop。
// 設計：1. 0/1 請求回 1；2. 先驗不超過最高可表示的 2 次冪；3. 呼叫 bit_ceil。
// 成本：單次容量計算時間與額外空間皆 O(1)。
// 上線注意：bit_ceil 結果不可表示時沒有可用結果；實際配置還要乘 bucket 大小並再次檢查總 byte 溢位。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_bit_header_cpp20.cpp' -o '/tmp/codex_cpp_C_Bit_03_bit_header_cpp20' && '/tmp/codex_cpp_C_Bit_03_bit_header_cpp20'
//
// === 預期輸出（節錄）===
// [實務] requested 1000 -> hash capacity 1024
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
