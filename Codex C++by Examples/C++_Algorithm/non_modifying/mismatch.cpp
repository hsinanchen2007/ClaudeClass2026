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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 859. Buddy Strings（親密字串）
// 題目：能否在 first 中恰交換兩個不同索引，使其等於 second；例如 ab/ba 為 true，
// ab/ab 為 false，而 aa/aa 因可交換兩個 a 為 true。
// 為何使用本章主題：mismatch 依序定位前兩個差異，equal 驗證其後沒有第三個；
// 完全相同時則排序後以 adjacent_find 檢查是否有可互換的重複字元。
// 思路：1. 長度不同 false；2. 完全相同時檢查 duplicate；3. 找前兩個 mismatch；
// 4. 驗證兩差異交叉相等且剩餘 suffix 全同。
// 複雜度：最壞時間 O(N log N)、額外空間 O(N)，N 為字串長度；排序分支決定最壞成本。
// 易錯點：完全相同不一定 true；必須恰有兩個可交換索引，不能接受只有一個或超過兩個差異。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】Schema 第一個差異 Offset
// 情境：expected 與 actual 是欄位名稱序列，要回共同前綴結束位置；值不同或任一側
// 先結束都停在該 index，完全相同則回 expected.size()。
// 為何使用本章主題：四 iterator mismatch 同時保護兩個不同長度範圍，直接提供首對
// 不同或先抵達 end 的位置。
// 設計：1. 比較兩個完整範圍；2. 取得 expected 側 iterator；3. 計算與 begin 的距離。
// 成本：時間 O(min(N,M))、額外空間 O(1)，N/M 為兩份 schema 欄位數。
// 上線注意：單一 index 無法區分完全相同、expected 較短或 actual 較短；完整 API 應回差異類型與兩側值。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'mismatch.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_mismatch' && '/tmp/codex_cpp_C_Algorithm_non_modifying_mismatch'
//
// === 預期輸出（節錄）===
// mismatch：LeetCode 859 與實務 schema 差異測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
