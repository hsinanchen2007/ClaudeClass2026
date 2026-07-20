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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 384. Shuffle an Array（打亂陣列）
// 題目：建構時保存 nums，reset 回復原始內容，shuffle 回傳所有排列等機率的結果；例如 [1,2,3] 每次仍須是同一 multiset。
// 為何使用本章主題：std::shuffle 搭配長期保存的 mt19937 提供 Fisher-Yates 類排列；seed overload 是測試用改寫，原題介面不提供 seed。
// 思路：1. constructor 保存不可變 original 與 engine state。2. reset 回傳 original 副本。3. shuffle 複製 original。4. 洗牌副本後回傳。
// 複雜度：reset 與 shuffle 的時間、回傳空間皆為 O(N)，N 為原陣列長度；物件本身另保存 O(N) 原資料。
// 易錯點：shuffle 必須從 original 複製且不可破壞 reset 狀態；不能測試結果一定不同，reset 也不應暗中重設 RNG state。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】重播隨機測試資料排列
// 情境：測試失敗後，以 seed=7 建立另一個相同資料集，重播連續兩次 shuffle 的確切順序。
// 為何使用本章主題：engine 是有狀態物件；相同 seed、資料、標準庫版本與呼叫次序才能逐輪重現，比只保存最後排列更容易診斷。
// 設計：1. 建立原執行與 replay 兩個 instance。2. 各做第一輪 shuffle 並比較。3. 各做第二輪。4. 再比較序列位置。
// 成本：每輪兩邊各複製並洗牌 N 個元素，時間 O(N)、每個回傳值空間 O(N)。
// 上線注意：必須記錄 seed、輸入與 library/version；並行呼叫同一 instance 會競爭 engine state，需外部同步或獨立 instance。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_lc_384_shuffle_array.cpp' -o '/tmp/codex_cpp_C_Random_08_lc_384_shuffle_array' && '/tmp/codex_cpp_C_Random_08_lc_384_shuffle_array'
//
// === 預期輸出（節錄）===
// [實務] recorded seed replays two shuffle operations
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
