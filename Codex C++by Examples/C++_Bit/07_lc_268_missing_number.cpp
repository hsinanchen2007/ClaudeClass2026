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

// 實務：shard IDs 預期從 0..N，找唯一未回報的 shard。
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
