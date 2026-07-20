/*
 * std::adjacent_find：尋找第一對相鄰且相等/符合 predicate 的元素
 * =============================================================
 * 回傳 pair 中第一個元素的 iterator；找不到回 last。預設比較 `*it == *(it+1)`，
 * 自訂 binary predicate 接收相鄰兩值。時間 O(N)，最多 N-1 次比較，至少需要
 * forward iterator。演算法唯讀，不保存 iterator；回傳 iterator 生命跟原容器走。
 *
 * 空或單元素範圍一定找不到。它只看「相鄰」，未排序資料中的全域重複不一定相鄰。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 645. Set Mismatch（錯誤的集合）
// 題目：nums 原應含 1..N，但一個值重複並取代另一值；回 [duplicate,missing]，例如
// [1,2,2,4] 回 [2,3]。
// 為何使用本章主題：先排序使重複值相鄰，adjacent_find 取得 duplicate；再由 1..N
// 的期望總和與實際總和推回 missing。
// 思路：1. 排序 nums；2. 找第一對相鄰重複；3. 計算實際與期望總和；4. 扣除多算的
// duplicate 後求 missing。
// 複雜度：時間 O(N log N)、額外空間 O(N)，N 為 nums 數量；空間包含按值輸入副本。
// 易錯點：題目保證恰一組錯配；加總要用 long long；adjacent_find 自身只找相鄰重複。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_find_error_nums(std::vector<int> nums) {
    std::sort(nums.begin(), nums.end());
    const auto duplicate_it = std::adjacent_find(nums.begin(), nums.end());
    assert(duplicate_it != nums.end());
    const int duplicate = *duplicate_it;
    const long long actual = std::accumulate(nums.begin(), nums.end(), 0LL);
    const long long n = static_cast<long long>(nums.size());
    const long long expected = n * (n + 1LL) / 2LL;
    const int missing = static_cast<int>(expected - (actual - duplicate));
    return {duplicate, missing};
}

struct Event {
    int sequence;
    long timestamp_ms;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】事件時間戳相鄰重複診斷
// 情境：事件已依 sequence 排列，契約規定相鄰事件不可有同一 timestamp_ms；要回
// 第一組違規的左側索引，完全合法時回 events.size()。
// 為何使用本章主題：adjacent_find 的 binary predicate 正好檢查每一對鄰居，並回
// 首個違規 pair 起點，比手寫前一筆狀態更直接。
// 設計：1. 比較相鄰 Event.timestamp_ms；2. 找到即停止；3. end 轉 size sentinel，
// 否則轉成索引。
// 成本：時間 O(N)、額外空間 O(1)，N 為事件數。
// 上線注意：相同時間是否真違規須由資料契約決定；size sentinel 不可當成可索引位置。
// -----------------------------------------------------------------------------
std::size_t practical_first_duplicate_timestamp(
    const std::vector<Event>& events) {
    const auto it = std::adjacent_find(
        events.begin(), events.end(),
        [](const Event& lhs, const Event& rhs) {
            return lhs.timestamp_ms == rhs.timestamp_ms;
        });
    return it == events.end()
               ? events.size()
               : static_cast<std::size_t>(std::distance(events.begin(), it));
}

int main() {
    const std::vector<int> values{1, 3, 3, 7};
    assert(std::adjacent_find(values.begin(), values.end()) == values.begin() + 1);
    assert(std::adjacent_find(values.begin(), values.begin() + 1) ==
           values.begin() + 1);

    assert((leetcode_find_error_nums({1, 2, 2, 4}) ==
            std::vector<int>{2, 3}));
    assert((leetcode_find_error_nums({1, 1}) == std::vector<int>{1, 2}));

    const std::vector<Event> events{{1, 100}, {2, 120}, {3, 120}, {4, 180}};
    assert(practical_first_duplicate_timestamp(events) == 1U);
    assert(practical_first_duplicate_timestamp({{1, 100}, {2, 120}}) == 2U);
    std::cout << "adjacent_find：LeetCode 645 與實務事件診斷測試通過\n";
}

/*
 * 易錯陷阱：排序是 LeetCode 解法總成本 O(N log N) 的來源；adjacent_find 本身只是
 * O(N)。若不能修改輸入要先 copy，空間 O(N)；hash set 可平均 O(N) 但多空間。
 *
 * 面試：如何找第一個相鄰遞減？predicate `lhs > rhs`；如何找連續差距過大？比較
 * `rhs-lhs > threshold`，但先防整數溢位。實務 timestamp 相同是否真錯要由資料契約
 * 決定，多來源事件可能合法同時發生。練習：回傳兩個 offending event 的 id。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'adjacent_find.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_adjacent_find' && '/tmp/codex_cpp_C_Algorithm_non_modifying_adjacent_find'
//
// === 預期輸出（節錄）===
// adjacent_find：LeetCode 645 與實務事件診斷測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
