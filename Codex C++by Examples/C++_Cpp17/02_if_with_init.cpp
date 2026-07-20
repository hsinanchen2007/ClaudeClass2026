/*
 * C++17 教科書：if statement with initializer
 *
 * `if (init; condition) { ... } else { ... }` 讓 iterator、lock、parse result 的 scope
 * 限定在 if/else 整體，避免臨時變數活太久或污染後續名稱。initializer 只執行一次，
 * 其變數在 condition、then 與 else 都可見，離開整個 statement 才析構。
 *
 * 【常見模式】`if (auto it = map.find(key); it != map.end())`，一次查找、不重複。
 * 【RAII】可在 init 建 lock_guard；但 lock 會涵蓋 then 與 else，臨界區是否太大要審查。
 * 【陷阱】不要在 condition 又呼叫 find；會做兩次查詢並可能在 concurrent 容器語意不一致。
 * 【面試題】init variable 的 scope 包不包含 else？包含。
 * 【練習】讓 practical_lookup 回傳 default 值並記錄 cache miss 次數。
 */

#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace basic {
void demo() {
    const std::unordered_map<std::string, int> ports{{"http", 80}, {"https", 443}};
    int selected = 0;
    if (const auto found = ports.find("https"); found != ports.end()) {
        selected = found->second;
    }
    assert(selected == 443);
    // found 在此已離開 scope，不能誤用。
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1. Two Sum（兩數之和）
// 題目：輸入 nums 與 target，回傳兩個相加等於 target 的索引；[3,2,4] 與 6 回傳 (1,2)。
// 為何使用本章主題：if-init 將 found iterator 限定在查找分支，查一次 map 就能判斷與讀取索引。
// 思路：1. 逐項計算 target-nums[i]；2. 在 seen 中查補數；3. 命中即回傳，否則記錄目前值。
// 複雜度：N 為元素數；平均時間 O(N)、雜湊最壞 O(N^2)，額外空間 O(N)。
// 易錯點：found 在整個 if/else 才有效；不要在 condition 重做 find，且必須先查後插。
// -----------------------------------------------------------------------------
std::pair<int, int> leetcode_two_sum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> seen;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        if (const auto found = seen.find(target - nums[i]); found != seen.end()) {
            return {found->second, static_cast<int>(i)};
        }
        seen.emplace(nums[i], static_cast<int>(i));
    }
    return {-1, -1};
}

void leetcode_test() {
    const auto answer = leetcode_two_sum({3, 2, 4}, 6);
    assert((answer == std::pair<int, int>{1, 2}));
}
}  // namespace leetcode

namespace practical {
using Cache = std::unordered_map<std::string, std::string>;

// -----------------------------------------------------------------------------
// 【日常實務範例】應用程式快取查詢
// 情境：依 key 讀取字串快取，命中時回傳值副本，miss 時回傳 std::nullopt。
// 為何使用本章主題：if-init 讓 iterator 只涵蓋命中判斷，避免離開查詢後誤用或污染外層名稱。
// 設計：1. 只呼叫一次 cache.find；2. 命中就複製 found->second 到 optional；3. 否則回 nullopt。
// 成本：平均查找 O(1)、雜湊最壞 O(N)；命中另複製 L 個字元，回傳空間 O(L)。
// 上線注意：副本適合跨 lock 使用；若改回 view/reference，rehash、erase 與 cache 生命週期都可能使其失效。
// -----------------------------------------------------------------------------
std::optional<std::string> practical_lookup(const Cache& cache, const std::string& key) {
    if (const auto found = cache.find(key); found != cache.end()) {
        return found->second;
    }
    return std::nullopt;
}

void practical_test() {
    const Cache cache{{"token", "abc"}};
    assert(practical_lookup(cache, "token") == std::optional<std::string>("abc"));
    assert(!practical_lookup(cache, "missing").has_value());
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "if-init：scope、Two Sum、cache lookup 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_if_with_init.cpp' -o '/tmp/codex_cpp_C_Cpp17_02_if_with_init' && '/tmp/codex_cpp_C_Cpp17_02_if_with_init'
//
// === 預期輸出（節錄）===
// if-init：scope、Two Sum、cache lookup 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
