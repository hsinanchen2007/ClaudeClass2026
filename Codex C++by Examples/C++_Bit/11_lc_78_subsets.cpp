// ============================================================================
// LeetCode 78：Subsets（以 bitmask 枚舉 power set）
// ============================================================================
//
// n 個元素的 subset 有 2^n 個。令 mask 的第 i bit 表示是否選 nums[i]，枚舉
// mask=0..2^n-1 即涵蓋所有 subsets，且不重複。輸出本身就有 O(n*2^n) 大小，演算法
// 不可能對大 n 擴展；bitmask 只讓對應關係簡潔。
//
// `1 << n` 若左 operand 是 int 且 n 太大會 UB；用 `std::size_t{1} << n`，並先驗證
// n < numeric_limits<size_t>::digits。LeetCode n<=10，沒有實際寬度問題。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 78. Subsets（子集合）
// 題目：回傳互異元素陣列的所有子集合；[1,2,3] 有 8 個，並包含空集合與完整集合。
// 為何使用本章主題：mask 的第 i bit 表示是否選 nums[i]，枚舉 0..2^N-1 即涵蓋所有位置組合。
// 思路：1. 先驗 N 小於 size_t 位寬；2. 枚舉每個 mask；3. 掃各 bit 並把被選元素加入 subset。
// 複雜度：N 為元素數；時間與結果空間皆 O(N*2^N)，暫時單一 subset 空間 O(N)。
// 易錯點：`1<<N` 必須用 size_t 並先驗位寬；輸入有重複值時位置組合仍會產生重複內容。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> subsets(const std::vector<int>& nums)
{
    if (nums.size() >= std::numeric_limits<std::size_t>::digits) {
        throw std::length_error("too many values for bitmask");
    }
    const std::size_t subset_count = std::size_t{1} << nums.size();
    std::vector<std::vector<int>> result;
    result.reserve(subset_count);
    for (std::size_t mask = 0U; mask < subset_count; ++mask) {
        std::vector<int> subset;
        for (std::size_t index = 0U; index < nums.size(); ++index) {
            if ((mask & (std::size_t{1} << index)) != 0U) subset.push_back(nums.at(index));
        }
        result.push_back(std::move(subset));
    }
    return result;
}

// 基礎示範：mask 的第 i bit 決定是否選第 i 個元素。
void basic_example()
{
    const std::vector<int> values{10, 20, 30};
    const std::size_t mask = 0b101U;
    std::vector<int> selected;
    for (std::size_t index = 0U; index < values.size(); ++index) {
        if ((mask & (std::size_t{1} << index)) != 0U) selected.push_back(values.at(index));
    }
    assert((selected == std::vector<int>{10, 30}));
    std::cout << "[基礎] mask 101 選到 index 0 與 2\n";
}

void leetcode_example()
{
    const auto result = subsets({1, 2, 3});
    assert(result.size() == 8U);
    assert(result.front().empty());
    assert((result.back() == std::vector<int>{1, 2, 3}));
    assert(subsets({}).size() == 1U); // 空集合的 power set 含一個空集合。
    std::cout << "[LeetCode 78] 3 elements -> 8 subsets\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】小型功能組合測試矩陣
// 情境：對 cache、tls 等少量 flags 產生所有開關組合標籤，供整合測試逐一執行。
// 為何使用本章主題：每個 feature 對應一個 mask bit，完整枚舉能保證小規模 power set 無遺漏。
// 設計：1. 驗 feature 數不超過 mask 位寬；2. 枚舉所有 masks；3. 串接被選名稱，空集合標為 none。
// 成本：F 為功能數；時間與結果空間 O(F*2^F)。
// 上線注意：組合數指數成長，features 多時應採 pairwise/風險導向測試；名稱也要 escape 分隔符。
// -----------------------------------------------------------------------------
std::vector<std::string> feature_combinations(const std::vector<std::string>& features)
{
    if (features.size() >= std::numeric_limits<std::size_t>::digits) {
        throw std::length_error("too many feature flags");
    }
    std::vector<std::string> result;
    const std::size_t combinations = std::size_t{1} << features.size();
    for (std::size_t mask = 0U; mask < combinations; ++mask) {
        std::string label;
        for (std::size_t bit = 0U; bit < features.size(); ++bit) {
            if ((mask & (std::size_t{1} << bit)) != 0U) {
                if (!label.empty()) label += '+';
                label += features.at(bit);
            }
        }
        result.push_back(label.empty() ? "none" : label);
    }
    return result;
}

void practical_example()
{
    const auto combinations = feature_combinations({"cache", "tls"});
    assert((combinations == std::vector<std::string>{"none", "cache", "tls", "cache+tls"}));
    std::cout << "[實務] feature matrix: none/cache/tls/cache+tls\n";
}

int main()
{
    basic_example();
    leetcode_example();
    practical_example();
}

// 生命週期：result 以值擁有每個 subset；`std::move(subset)` 後只把 moved-from 物件解構，
// 不保存原 nums 的 reference，因此 caller 修改或銷毀輸入都不會讓輸出懸空。
// 練習：只枚舉某個 mask 的 non-empty submasks：`sub=(sub-1)&mask`。

/*
【本課面試問答】
Q1：bitmask 枚舉 subsets 的時間與空間複雜度？
A：共有 2^N 個 masks，每個 mask 若掃 N bits，時間 O(N*2^N)；只輸出所有元素本身也有相同量級。
結果儲存通常 O(N*2^N)。這是指數問題，不能因 bit operations 很快就忽略輸出規模。

Q2：`1 << n` 有哪些陷阱？
A：左側 literal 1 是 signed int；n 過大可能 overflow/UB，甚至在轉成 size_t 前已出錯。應用
`size_t{1} << n` 並先驗 `n < numeric_limits<size_t>::digits`，仍要限制實際可配置的 2^N。

Q3：輸入有重複值時，mask 法會自動去重嗎？
A：不會；它區分 positions，因此相同值可形成重複 subsets。若題目要求 unique subsets，可先排序，
在 backtracking 同一層跳過重複值，或生成後去重，但後者浪費時間/空間。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '11_lc_78_subsets.cpp' -o '/tmp/codex_cpp_C_Bit_11_lc_78_subsets' && '/tmp/codex_cpp_C_Bit_11_lc_78_subsets'
//
// === 預期輸出（節錄）===
// [實務] feature matrix: none/cache/tls/cache+tls
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
