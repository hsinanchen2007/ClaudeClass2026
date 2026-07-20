// ============================================================================
// LeetCode 201：Bitwise AND of Numbers Range
// ============================================================================
//
// 要求 left & (left+1) & ... & right。逐數 AND 在範圍很大時太慢。觀察只要某 bit 在
// 區間中曾為 0，結果該 bit 就是 0；答案其實是 left/right 的共同 binary prefix，
// 後方變動 bits 全清零。
//
// 作法：兩端同時右移直到相等，記錄 shift 次數，再把共同 prefix 左移回去。
// 時間 O(W)、空間 O(1)，W 最多 31。題目保證 0<=left<=right；一般函式仍應驗證。
// ============================================================================

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 201. Bitwise AND of Numbers Range（整數範圍按位與）
// 題目：回傳 left 到 right 所有整數的 bitwise AND；[5,7] 即 101&110&111，答案為 4。
// 為何使用本章主題：區間內會變動的低 bits 必含 0，只有 left/right 的共同 binary prefix 能保留。
// 思路：1. 將兩端轉 unsigned；2. 同時右移直到相等並記 shifts；3. 將共同 prefix 左移回原位。
// 複雜度：W 為整數有效位數；時間 O(W)、額外空間 O(1)，不依賴區間長度。
// 易錯點：一般 API 要驗 0<=left<=right；不可逐數 AND 到 INT_MAX，shift 也必須保持 unsigned。
// -----------------------------------------------------------------------------
int range_bitwise_and(int left, int right)
{
    if (left < 0 || right < left) throw std::invalid_argument("invalid nonnegative range");
    std::uint32_t low = static_cast<std::uint32_t>(left);
    std::uint32_t high = static_cast<std::uint32_t>(right);
    unsigned shifts = 0U;
    while (low != high) {
        low >>= 1U;
        high >>= 1U;
        ++shifts;
    }
    return static_cast<int>(low << shifts);
}

// 基礎示範：兩端共同 prefix 保留，第一個不同 bit 以下必定在區間中翻轉。
void basic_example()
{
    assert(range_bitwise_and(12, 15) == 12); // 1100..1111 共同 prefix 是 1100。
    assert(range_bitwise_and(8, 9) == 8);
    bool rejected = false;
    try { (void)range_bitwise_and(7, 5); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    std::cout << "[基礎] range AND 等於兩端共同 binary prefix\n";
}

void leetcode_example()
{
    assert(range_bitwise_and(5, 7) == 4);   // 101 & 110 & 111 = 100。
    assert(range_bitwise_and(0, 0) == 0);
    assert(range_bitwise_and(1, 2'147'483'647) == 0);
    std::cout << "[LeetCode 201] [5,7] -> 4\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】IPv4 範圍共同前綴
// 情境：給一段連續 IPv4 整數位址，求兩端都不變的 network bits 與 prefix length。
// 為何使用本章主題：兩端同步右移可找共同高位，低位差異數就是 host bits，與 range AND 同一 invariant。
// 設計：1. 先拒絕反向範圍；2. 右移直到 first==last；3. 左移還原 network 並回 32-host_bits。
// 成本：最多處理 32 個 bits，時間 O(32)、額外空間 O(1)。
// 上線注意：結果只是共同前綴，不一定精確表示原範圍的單一 CIDR；地址 byte order 也需在邊界明訂。
// -----------------------------------------------------------------------------
std::pair<std::uint32_t, unsigned> common_prefix(std::uint32_t first, std::uint32_t last)
{
    if (last < first) throw std::invalid_argument("reversed address range");
    unsigned host_bits = 0U;
    while (first != last) {
        first >>= 1U;
        last >>= 1U;
        ++host_bits;
    }
    return {first << host_bits, 32U - host_bits};
}

void practical_example()
{
    const auto [network, prefix_length] = common_prefix(0xC0A80100U, 0xC0A801FFU);
    assert(network == 0xC0A80100U && prefix_length == 24U); // 192.168.1.0/24
    std::cout << "[實務] 192.168.1.0..255 共同 prefix=/24\n";
}

int main()
{
    basic_example();
    leetcode_example();
    practical_example();
}

// 易錯：不要逐一迭代到 right；[1, INT_MAX] 會做數十億次。共同 prefix 固定最多 W 步。
// 面試追問：為何 right &= right-1 也成立？它逐次清掉 right 超過 left 的變動低位。
// 生命週期：演算法只處理 value copies；回傳 pair 也按值擁有結果，沒有 reference 失效問題。
// 練習：改用反覆 `right &= right-1` 直到 right<=left 的另一標準解法。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '10_lc_201_range_bitwise_and.cpp' -o '/tmp/codex_cpp_C_Bit_10_lc_201_range_bitwise_and' && '/tmp/codex_cpp_C_Bit_10_lc_201_range_bitwise_and'
//
// === 預期輸出（節錄）===
// [實務] 192.168.1.0..255 共同 prefix=/24
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
