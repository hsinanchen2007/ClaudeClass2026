// ============================================================================
// LeetCode 268：Missing Number
// ============================================================================
//
// nums 長度 n，內容是 [0,n] 中互異的 n 個值，缺一個。把完整 index 0..n 與所有
// values XOR，成對值抵消，留下 missing。這避免 arithmetic sum 的 overflow：
// `n*(n+1)/2 - sum(nums)` 若使用 int，n 大時可能 overflow。
//
// 演算法依賴「範圍正確且不重複」。若輸入有 duplicate/out-of-range，XOR 仍產生某值，
// 但不再是可靠答案；production input 應驗證或使用可回報錯誤的資料結構。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 268. Missing Number（缺失的數字）
// 題目：長度 N 的陣列含 [0,N] 中互異的 N 個值，找缺少者；[3,0,1] 回傳 2。
// 為何使用本章主題：把完整索引集合與觀測值全部 XOR，成對值消失，且避免等差總和的乘法溢位。
// 思路：1. answer 先放入 N；2. 每輪 XOR index；3. 再 XOR nums[index]，最後留下缺值。
// 複雜度：N 為元素數；時間 O(N)、額外空間 O(1)。
// 易錯點：初值必須包含 N；XOR 不會驗 duplicate 或 out-of-range，production 輸入需另做 O(N) 驗證。
// -----------------------------------------------------------------------------
int missing_number(const std::vector<int>& nums)
{
    int answer = static_cast<int>(nums.size());
    for (std::size_t index = 0U; index < nums.size(); ++index) {
        if (index > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::length_error("input too large for int problem contract");
        }
        answer ^= static_cast<int>(index);
        answer ^= nums.at(index);
    }
    return answer;
}

// 基礎示範：完整集合與觀測集合各 XOR 一次，已出現的值就會成對消失。
void basic_example()
{
    const int complete = 0 ^ 1 ^ 2 ^ 3;
    const int observed = 3 ^ 0 ^ 1;
    assert((complete ^ observed) == 2);
    assert((1 ^ 2 ^ 3 ^ 3 ^ 1) == 2);
    std::cout << "[基礎] complete XOR observed 留下 missing value\n";
}

void leetcode_example()
{
    assert(missing_number({3, 0, 1}) == 2);
    assert(missing_number({0, 1}) == 2);
    assert(missing_number({9, 6, 4, 2, 3, 5, 7, 0, 1}) == 8);
    assert(missing_number({0}) == 1);
    std::cout << "[LeetCode 268] answers 2, 2, 8, 1\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】分片回報缺號檢查
// 情境：N+1 個 shards 編號 0..N，監控批次收到 N 個互異回報，要找唯一未回報的 shard。
// 為何使用本章主題：在已驗證的連續編號契約下，index/value XOR 不需額外 seen table。
// 設計：1. 將回報列表交給 missing_number；2. 以 index 補完整集合；3. 回傳剩餘 shard ID。
// 成本：N 為回報數；時間 O(N)、額外空間 O(1)。
// 上線注意：重複、越界與缺多片會破壞答案；真實監控應先驗證來源、批次版本與完整頻次。
// -----------------------------------------------------------------------------
int missing_shard(const std::vector<int>& reported_shards)
{
    return missing_number(reported_shards);
}

void practical_example()
{
    assert(missing_shard({0, 4, 2, 1}) == 3);
    std::cout << "[實務] missing shard id=3\n";
}

int main()
{
    basic_example();
    leetcode_example();
    practical_example();
}

// 易錯與面試：
//   * size n 代表合法值範圍含 n，共 n+1 種；初始化 answer=n 才不會漏最後一值。
//   * XOR 會對 malformed input 產生看似合理的整數，並不具備驗證能力。
//   * arithmetic sum 可用較寬型別避免 overflow，但仍要證明型別上界。
//
// 替代法：排序後找 index/value 第一個不等處，O(NlogN) 且改資料；boolean seen 是 O(N)
// 空間但能直接偵測 duplicate/out-of-range。工程上要依「是否信任輸入」選，而非只追 O(1)。
// 生命週期：輸入由 const reference 借用；函式不修改資料，也不把 reference/iterator 帶出去。
// 練習：加入 O(N) 驗證，拒絕 duplicate 與不在 [0,n] 的值。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_lc_268_missing_number.cpp' -o '/tmp/codex_cpp_C_Bit_07_lc_268_missing_number' && '/tmp/codex_cpp_C_Bit_07_lc_268_missing_number'
//
// === 預期輸出（節錄）===
// [實務] missing shard id=3
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
