/*
 * std::sample（C++17）：無放回地隨機抽取最多 n 個元素
 * =====================================================
 * sample(first,last,out,n,urbg) 不改來源，每個元素至多被選一次；若 n 大於 population
 * 大小，只輸出全部元素。時間 O(distance(first,last))，輸出數量 min(n,N)。
 *
 * 隨機引擎必須活過呼叫；測試使用固定 seed 便於重現，但標準不保證不同標準庫產生
 * 完全相同樣本，因此測試應驗證大小、來源成員與不重複，不 assert 特定排列。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <random>
#include <set>
#include <vector>

// LeetCode 398：Random Pick Index，reservoir sampling 選一個 target index。
std::size_t leetcode_reservoir_pick(const std::vector<int>& nums, int target,
                                    std::uint32_t seed) {
    std::mt19937 engine(seed);
    std::size_t chosen = nums.size();
    std::size_t seen = 0;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        if (nums[i] != target) {
            continue;
        }
        ++seen;
        std::uniform_int_distribution<std::size_t> distribution(1U, seen);
        if (distribution(engine) == 1U) {
            chosen = i;
        }
    }
    return chosen;
}

// 實務：從 request id 母體抽稽核樣本；回傳順序不列入契約。
std::vector<int> practical_audit_sample(const std::vector<int>& requests,
                                        std::size_t count,
                                        std::uint32_t seed) {
    std::mt19937 engine(seed);
    std::vector<int> result;
    result.reserve(std::min(count, requests.size()));
    std::sample(requests.begin(), requests.end(), std::back_inserter(result),
                count, engine);
    return result;
}

int main() {
    const std::vector<int> nums{1, 2, 3, 3, 3};
    const std::size_t picked = leetcode_reservoir_pick(nums, 3, 42U);
    assert(picked < nums.size() && nums[picked] == 3);
    assert(leetcode_reservoir_pick(nums, 9, 42U) == nums.size());

    const std::vector<int> population{10, 20, 30, 40, 50};
    const auto sample = practical_audit_sample(population, 3U, 123U);
    assert(sample.size() == 3U);
    const std::set<int> unique(sample.begin(), sample.end());
    assert(unique.size() == sample.size());
    for (int value : sample) {
        assert(std::find(population.begin(), population.end(), value) !=
               population.end());
    }
    std::cout << "sample：LeetCode reservoir 與實務稽核抽樣測試通過\n";
}

/*
 * 易錯陷阱：每次呼叫都用目前秒數 seed，快速重複呼叫可能得到相同序列；正式服務
 * 應管理 engine 生命週期。`rand()%N` 有 modulo bias，也不是 sample 的替代。
 *
 * 面試：reservoir sampling 為何每個 target 最終等機率？第 i 個以 1/i 取代，先前
 * 候選保留機率 (1-1/i)，歸納後每個為 1/count。LeetCode 若保證 target 存在，可省
 * sentinel；一般實務 API 應用 optional 而非 size() sentinel。
 * 練習：抽 k 個 stream 元素，證明不需事先知道總長度。
 */
