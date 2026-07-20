/*
 * std::shuffle：使用 UniformRandomBitGenerator 隨機排列範圍
 * ==========================================================
 * 需要 random-access iterator，做 O(N) 次隨機抽取/交換，理論上每個 permutation
 * 等機率（前提是合格 URBG）。它會修改元素順序、不改 size。固定 seed 只保證在
 * 同一實作便於除錯；標準不承諾跨 libstdc++/libc++ 的排列一致。
 *
 * 安全用途不可直接把 mt19937 當密碼學 RNG；抽獎、token、nonce 要用專用 CSPRNG。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

// LeetCode 384：Shuffle an Array。此函式回一份 shuffled copy，保留原輸入。
std::vector<int> leetcode_shuffle_copy(const std::vector<int>& nums,
                                       std::uint32_t seed) {
    std::vector<int> result = nums;
    std::mt19937 engine(seed);
    std::shuffle(result.begin(), result.end(), engine);
    return result;
}

// 實務：先打散使用者，再 round-robin 分到 A/B，可避免輸入排序造成偏差。
std::map<char, std::vector<int>> practical_assign_experiment(
    const std::vector<int>& user_ids, std::uint32_t seed) {
    std::vector<int> shuffled = user_ids;
    std::mt19937 engine(seed);
    std::shuffle(shuffled.begin(), shuffled.end(), engine);
    std::map<char, std::vector<int>> groups{{'A', {}}, {'B', {}}};
    for (std::size_t i = 0; i < shuffled.size(); ++i) {
        groups[i % 2U == 0U ? 'A' : 'B'].push_back(shuffled[i]);
    }
    return groups;
}

int main() {
    const std::vector<int> original{1, 2, 3, 4, 5};
    auto shuffled = leetcode_shuffle_copy(original, 7U);
    assert(original == std::vector<int>({1, 2, 3, 4, 5}));
    std::sort(shuffled.begin(), shuffled.end());
    assert(shuffled == original);  // 驗 permutation，不綁特定排列。

    const auto groups = practical_assign_experiment({10, 11, 12, 13, 14}, 9U);
    assert(groups.at('A').size() == 3U && groups.at('B').size() == 2U);
    std::vector<int> assigned = groups.at('A');
    assigned.insert(assigned.end(), groups.at('B').begin(), groups.at('B').end());
    std::sort(assigned.begin(), assigned.end());
    assert((assigned == std::vector<int>{10, 11, 12, 13, 14}));
    std::cout << "shuffle：LeetCode 384 與實務 A/B 分組測試通過\n";
}

/*
 * 易錯陷阱：測試 `shuffled != original` 不可靠，合法 shuffle 有機率剛好不變；只驗
 * permutation 與 reset 契約。固定 seed 是 reproducibility，不是 randomness 品質證明。
 *
 * 面試：shuffle 通常是 Fisher-Yates；每步從尚未固定區間均勻選一格交換，O(N)。
 * 實務 A/B test 還需要穩定分桶（常用 hash(user_id,experiment_id)），否則重跑會讓
 * 使用者換組；本例只示範一次性批次。練習：改成穩定 hash 分組並測重跑一致。
 *
 * LeetCode reset 通常要回原始排列，因此 class 應保存 immutable original；不要只
 * 保存上一輪 shuffled。實務還要版本化 seed/experiment id，否則部署改演算法會換組。
 * 面試可手寫 Fisher-Yates，確認 random index 範圍是 [i,N-1] 而不是固定 [0,N-1]。
 */
