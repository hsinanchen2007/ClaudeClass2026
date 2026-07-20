/*
 * std::mismatch：找兩範圍第一個不相等位置
 * ========================================
 * 回 pair{it1,it2}。四 iterator overload 同時知道兩端，若共同前綴完全相等，至少
 * 一個 iterator 會到 end；三 iterator overload 要呼叫者保證第二範圍夠長。
 * 時間 O(min(N,M)) 且遇差異短路。predicate 應表達等價關係。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 859：Buddy Strings。恰兩個 mismatch 且交叉相等；完全相同時需有重複字。
bool leetcode_buddy_strings(const std::string& first, const std::string& second) {
    if (first.size() != second.size()) {
        return false;
    }
    if (first == second) {
        std::string sorted = first;
        std::sort(sorted.begin(), sorted.end());
        return std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end();
    }

    const auto first_diff = std::mismatch(first.begin(), first.end(), second.begin());
    if (first_diff.first == first.end()) {
        return false;
    }
    const auto next_first = std::next(first_diff.first);
    const auto next_second = std::next(first_diff.second);
    const auto second_diff = std::mismatch(next_first, first.end(), next_second);
    if (second_diff.first == first.end()) {
        return false;  // 只有一個差異。
    }
    const bool swapped = *first_diff.first == *second_diff.second &&
                         *first_diff.second == *second_diff.first;
    const auto after_second_first = std::next(second_diff.first);
    const auto after_second_second = std::next(second_diff.second);
    return swapped && std::equal(after_second_first, first.end(), after_second_second);
}

// 實務：找兩份 schema 第一個不同欄位；完全相同回 size。
std::size_t practical_first_schema_difference(
    const std::vector<std::string>& expected,
    const std::vector<std::string>& actual) {
    const auto [left, right] = std::mismatch(expected.begin(), expected.end(),
                                             actual.begin(), actual.end());
    static_cast<void>(right);
    return static_cast<std::size_t>(std::distance(expected.begin(), left));
}

int main() {
    const std::string a = "abc";
    const std::string b = "axc";
    const auto diff = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
    assert(diff.first == a.begin() + 1 && diff.second == b.begin() + 1);

    // 共同前綴相同但右邊較短：左 iterator 停在同一 index，不會跳到 expected.end()。
    const std::vector<int> expected_long{1, 2, 3};
    const std::vector<int> actual_short{1, 2};
    const auto right_ended = std::mismatch(expected_long.begin(), expected_long.end(),
                                           actual_short.begin(), actual_short.end());
    assert(right_ended.first == expected_long.begin() + 2);
    assert(right_ended.second == actual_short.end());

    // 左邊較短時才是 left==expected.end()；right 停在相同的共同前綴長度。
    const std::vector<int> expected_short{1, 2};
    const std::vector<int> actual_long{1, 2, 3};
    const auto left_ended = std::mismatch(expected_short.begin(), expected_short.end(),
                                          actual_long.begin(), actual_long.end());
    assert(left_ended.first == expected_short.end());
    assert(left_ended.second == actual_long.begin() + 2);

    assert(leetcode_buddy_strings("ab", "ba"));
    assert(!leetcode_buddy_strings("ab", "ab"));
    assert(leetcode_buddy_strings("aa", "aa"));
    assert(!leetcode_buddy_strings("abcd", "badc"));

    assert(practical_first_schema_difference({"id", "name"}, {"id", "title"}) == 1U);
    assert(practical_first_schema_difference({"id"}, {"id"}) == 1U);
    std::cout << "mismatch：LeetCode 859 與實務 schema 差異測試通過\n";
}

/*
 * 易錯陷阱：四 iterator overload 長度不同且共同前綴相等時，要檢查兩個 end 才知道
 * 是誰先結束。本實務函式只回共同前綴長度：actual 較短時，left 停在 expected 的
 * 同一 index；expected 較短時 left 才等於 expected.end()。完整診斷應同時檢查
 * left/right 與兩個 end，回傳「左短、右短、值不同或完全相同」。
 *
 * 面試：Buddy Strings 為何相同字串仍可能 true？交換兩個相同字元後字串不變，故需
 * 至少一個重複字元。LeetCode 字元集若固定 26 可用 array count。練習：回所有差異。
 */
