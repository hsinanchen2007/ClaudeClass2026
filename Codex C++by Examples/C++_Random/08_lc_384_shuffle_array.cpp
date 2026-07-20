// ============================================================================
// LeetCode 384：Shuffle an Array - 完整 class 設計
// ============================================================================
//
// `reset()` 回原始 configuration；`shuffle()` 回均勻 permutation。original_ 必須保持不變，
// 每次 shuffle 都從 original copy 開始，否則 reset/測試語意容易混亂。std::shuffle 實作
// Fisher-Yates 類演算法並接收 URBG。
//
// LeetCode constructor 不提供 seed；production/testing 版可 overload seed，使失敗可重播。
// 不可測「shuffle 後一定不同」，因 n! 個合法結果中原順序也有 1/n! 機率。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

class Solution {
public:
    Solution(std::vector<int> nums, unsigned seed)
        : original_(std::move(nums)), engine_(seed) {}

    std::vector<int> reset() const { return original_; }

    std::vector<int> shuffle()
    {
        std::vector<int> result = original_;
        std::shuffle(result.begin(), result.end(), engine_);
        return result;
    }

private:
    std::vector<int> original_;
    std::mt19937 engine_;
};

// 基礎示範：reset 回傳獨立 copy；修改回傳值不會破壞 class 保存的 original_。
void basic_example()
{
    Solution solution({1, 2, 3}, 1U);
    auto copy = solution.reset();
    copy.at(0U) = 99;
    // reset/shuffle 是本例要真正執行的 API；NDEBUG 不可連呼叫本身一起移除。
    [[maybe_unused]] const auto original = solution.reset();
    assert((original == std::vector<int>{1, 2, 3}));
    assert(copy.at(0U) == 99);
    std::cout << "[基礎] original 狀態與 reset 回傳 copy 各自擁有資料\n";
}

void leetcode_example()
{
    Solution solution({1, 2, 3}, 42U);
    const auto first = solution.shuffle();
    // 必須用 named expected；不同 temporary 的 begin/end 不屬同一 range，會是 UB。
    const std::vector<int> expected{1, 2, 3};
    assert(std::is_permutation(first.begin(), first.end(), expected.begin(), expected.end()));
    [[maybe_unused]] const auto reset_after_first = solution.reset();
    assert(reset_after_first == expected);
    const auto second = solution.shuffle();
    assert(std::is_permutation(second.begin(), second.end(), expected.begin(), expected.end()));
    [[maybe_unused]] const auto reset_after_second = solution.reset();
    assert(reset_after_second == expected);
    std::cout << "[LeetCode 384] shuffle/reset preserve permutation and original\n";
}

// 實務：同 seed 的兩 instances 應重播同一連串 shuffle calls（同 library/version）。
void practical_example()
{
    Solution first({1, 2, 3, 4}, 7U);
    Solution replay({1, 2, 3, 4}, 7U);
    [[maybe_unused]] const auto first_round = first.shuffle();
    [[maybe_unused]] const auto replay_first_round = replay.shuffle();
    assert(first_round == replay_first_round);
    [[maybe_unused]] const auto second_round = first.shuffle();
    [[maybe_unused]] const auto replay_second_round = replay.shuffle();
    assert(second_round == replay_second_round);
    std::cout << "[實務] recorded seed replays two shuffle operations\n";
}

int main()
{
    basic_example();
    leetcode_example();
    practical_example();
}

// 易錯與面試：reset 回原內容，但不必重設 engine state；題目契約要分清資料狀態與 RNG
// 狀態。Fisher-Yates 第 i 步只能從 [i,n-1] 均勻選，否則 permutation 會有偏。
// 複雜度：reset 與 shuffle 都需複製 N 個元素，時間 O(N)、回傳空間 O(N)；engine 與
// original_ 的生命週期跟著 Solution object，不能把臨時 engine 每次重建後誤稱「隨機」。
// 練習：加入 constructor 使用 random_device，但保留可注入 seed 的 overload。
