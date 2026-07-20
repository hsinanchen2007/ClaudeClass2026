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

// LeetCode 645：Set Mismatch。排序後重複值相鄰，再由期望總和求遺失值。
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

// 實務：已依 sequence 排序的事件不應有相同 timestamp 相鄰，回問題位置。
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
