// ============================================================================
// LeetCode 528：Random Pick with Weight
// ============================================================================
//
// 建 prefix sums：weights [1,3,2] -> [1,4,6]。均勻抽 ticket [1,total]，用 lower_bound
// 找第一個 prefix>=ticket；每個 index 佔用恰好 weight 個 tickets，因此機率成比例。
// constructor O(N)、每次 pick O(log N)、空間 O(N)。
//
// weights/總和用 uint64_t，避免很多 int weights 相加 overflow；拒絕非正 weight 與空輸入。
// uniform_int_distribution 直接生成 [1,total]，不可 `% total` 造成 bias。
// ============================================================================

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

class Solution {
public:
    Solution(const std::vector<int>& weights, unsigned seed) : engine_(seed)
    {
        if (weights.empty()) throw std::invalid_argument("weights must not be empty");
        if (weights.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::length_error("too many weights for int return type");
        }
        std::uint64_t total = 0U;
        for (const int weight : weights) {
            if (weight <= 0) throw std::invalid_argument("weight must be positive");
            const auto add = static_cast<std::uint64_t>(weight);
            if (total > std::numeric_limits<std::uint64_t>::max() - add) {
                throw std::overflow_error("weight sum overflow");
            }
            total += add;
            prefix_.push_back(total);
        }
    }

    int pickIndex()
    {
        const std::uint64_t ticket =
            std::uniform_int_distribution<std::uint64_t>(1U, prefix_.back())(engine_);
        const auto found = std::lower_bound(prefix_.begin(), prefix_.end(), ticket);
        return static_cast<int>(std::distance(prefix_.begin(), found));
    }

private:
    std::vector<std::uint64_t> prefix_;
    std::mt19937 engine_;
};

// 基礎示範：只有一個 bucket 時，不論抽到哪張 ticket 都只能落在 index 0。
void basic_example()
{
    Solution single_bucket({7}, 3U);
    for (int sample = 0; sample < 10; ++sample) {
        assert(single_bucket.pickIndex() == 0);
    }
    std::cout << "[基礎] prefix [7] 的所有 tickets 都對應 index 0\n";
}

void leetcode_example()
{
    Solution picker({1, 3}, 42U);
    std::array<int, 2> counts{};
    for (int sample = 0; sample < 20'000; ++sample) ++counts.at(static_cast<std::size_t>(picker.pickIndex()));
    const double ratio = static_cast<double>(counts[1]) / static_cast<double>(counts[0]);
    assert(ratio > 2.7 && ratio < 3.3);
    std::cout << "[LeetCode 528] observed weight ratio=" << ratio << '\n';
}

// 實務：weighted backend routing；高容量 server 得更多 tickets。
void practical_example()
{
    Solution backends({2, 1, 1}, 8U);
    for (int request = 0; request < 100; ++request) {
        const int backend = backends.pickIndex();
        assert(backend >= 0 && backend < 3);
    }
    std::cout << "[實務] weighted router always returns backend 0..2\n";
}

int main()
{
    basic_example();
    leetcode_example();
    practical_example();
}

// 生命週期：prefix_ 與 engine_ 由 Solution 物件擁有；pickIndex 會推進 engine state，
// 因此同一 seed 只有在呼叫順序也相同時才可重播。不要回傳 prefix_ 內部 iterator。
// 練習：支援動態更新 weight 時，研究 Fenwick tree 將 update/pick 都降到 O(log N)。

/*
【本課面試問答】
Q1：prefix sum 加二分搜尋如何實作 weighted random？
A：令 prefix[i] 為 0..i 權重總和，均勻抽 ticket 於 `[1,total]`，再用 `lower_bound` 找第一個
prefix >= ticket；某 index 覆蓋的 ticket 數恰等於其權重，因此機率為 weight/total。

Q2：此解法最容易漏掉哪些輸入檢查？
A：權重必須符合契約（通常非負且至少一個正值），prefix total 要用足夠寬的整數並檢查 overflow，
空輸入要拒絕。若允許浮點權重，還要定義 NaN、負值與 rounding 行為。

Q3：若權重頻繁更新，為何不能每次重建 prefix？
A：靜態權重建表 O(N)、抽樣 O(log N)；每次更新重建是 O(N)。Fenwick/segment tree 可把單點更新
與按累積權重尋找都做到 O(log N)，代價是更複雜的不變式與同步。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_lc_528_random_pick_with_weight.cpp' -o '/tmp/codex_cpp_C_Random_09_lc_528_random_pick_with_weight' && '/tmp/codex_cpp_C_Random_09_lc_528_random_pick_with_weight'
//
// === 預期輸出（節錄）===
// [實務] weighted router always returns backend 0..2
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
