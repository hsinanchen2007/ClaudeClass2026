// ============================================================================
// 課題 6：std::shuffle 與 Fisher-Yates
// ============================================================================
//
// std::shuffle(first,last,engine) 產生均勻隨機 permutation（假設 URBG 品質良好），時間
// O(N)。舊 random_shuffle 使用 global rand/implementation callback，C++17 已移除。
// 正確 Fisher-Yates 在 i 從 n-1 降到 1 時，從 [0,i] 均勻選 j 並 swap；若每次都從
// [0,n-1] 選，permutations 不是等機率。
//
// shuffle 修改原容器；需要 reset 時保留 original copy。測試應驗「是原資料排列」，
// 不應斷言必定改變順序（原排列本身也是合法隨機結果）。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

bool is_permutation_1_to_n(const std::vector<int>& values)
{
    std::vector<int> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    std::vector<int> expected(values.size());
    std::iota(expected.begin(), expected.end(), 1);
    return sorted == expected;
}

void basic_example()
{
    std::vector<int> cards(52U);
    std::iota(cards.begin(), cards.end(), 1);
    std::mt19937 engine(42U);
    std::shuffle(cards.begin(), cards.end(), engine);
    assert(cards.size() == 52U);
    assert(is_permutation_1_to_n(cards));
    std::cout << "[基礎] shuffled deck remains a 1..52 permutation\n";
}

// LeetCode 384：Shuffle an Array 的核心；完整 class 在第 8 課。
std::vector<int> shuffled_copy(const std::vector<int>& input, std::mt19937& engine)
{
    std::vector<int> output = input;
    std::shuffle(output.begin(), output.end(), engine);
    return output;
}

void leetcode_384_example()
{
    const std::vector<int> original{1, 2, 3};
    std::mt19937 engine(7U);
    const auto result = shuffled_copy(original, engine);
    assert(std::is_permutation(result.begin(), result.end(), original.begin(), original.end()));
    assert((original == std::vector<int>{1, 2, 3}));
    std::cout << "[LeetCode 384] shuffle returns permutation without mutating original\n";
}

// 實務：隨機化 test execution order，失敗時記錄 seed 以重播順序。
std::vector<int> randomized_test_order(int test_count, unsigned seed)
{
    if (test_count < 0) throw std::invalid_argument("negative test count");
    std::vector<int> order(static_cast<std::size_t>(test_count));
    std::iota(order.begin(), order.end(), 0);
    std::mt19937 engine(seed);
    std::shuffle(order.begin(), order.end(), engine);
    return order;
}

void practical_example()
{
    const auto first = randomized_test_order(10, 2026U);
    const auto replay = randomized_test_order(10, 2026U);
    assert(first == replay);
    std::cout << "[實務] seed 2026 exactly replays test order\n";
}

int main()
{
    basic_example();
    leetcode_384_example();
    practical_example();
}

// 易錯與面試：`random_shuffle` 已移除，因其隨機來源不可控；std::shuffle 要傳 URBG。
// 用 `sort` 搭隨機 key 不是無偏 Fisher-Yates，collision 與排序代價也讓它不合適。
// 練習：手寫 Fisher-Yates，與 std::shuffle 比較 permutation invariant。
// 生命週期：shuffle 原地修改 range；iterators 必須在操作期間有效，engine state 由 caller 持有。
