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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 384. Shuffle an Array（打亂陣列）
// 題目：設計物件支援 reset 與均勻 shuffle；例如 [1,2,3] 可回任一等機率排列。
// 本 helper 僅回傳一次 shuffled copy，未實作題目要求的完整 class/reset 介面。
// 為何使用本章主題：std::shuffle 以 mt19937 對可隨機存取的副本做均勻排列，避免
// 手寫有偏差的交換索引。
// 思路：1. 複製 nums；2. 由 seed 建立 engine；3. 原地 shuffle 副本；4. 回傳副本。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 nums 的元素數。
// 易錯點：合法 shuffle 可能剛好等於原序列；完整 LC384 還需保存 immutable original 供 reset。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_shuffle_copy(const std::vector<int>& nums,
                                       std::uint32_t seed) {
    std::vector<int> result = nums;
    std::mt19937 engine(seed);
    std::shuffle(result.begin(), result.end(), engine);
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】一次性 A/B 實驗批次分組
// 情境：user_ids 可能依註冊時間排序，先打散再交錯放入 A/B，讓兩組筆數最多差 1，
// 且每位使用者只出現一次。
// 為何使用本章主題：shuffle 可移除輸入順序與分組位置的直接關聯，再由 round-robin
// 均分；適合一次性批次，不適合需要跨重跑穩定黏著的線上實驗。
// 設計：1. 複製並打散 user_ids；2. 建立 A/B 容器；3. 偶數位置進 A、奇數位置進 B。
// 成本：時間 O(N)、額外空間 O(N)，N 為使用者數。
// 上線注意：正式實驗應以 hash(user_id,experiment_id) 穩定分桶；mt19937 也不是安全抽籤 RNG。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'shuffle.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_shuffle' && '/tmp/codex_cpp_C_Algorithm_modifying_shuffle'
//
// === 預期輸出（節錄）===
// shuffle：LeetCode 384 與實務 A/B 分組測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
