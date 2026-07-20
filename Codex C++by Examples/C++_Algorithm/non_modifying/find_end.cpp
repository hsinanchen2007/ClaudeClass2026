/*
 * std::find_end：尋找子序列最後一次出現的位置
 * ===========================================
 * find_end(hay_first,hay_last,needle_first,needle_last) 回最後一個匹配子序列的起點，
 * 找不到回 hay_last。空 needle 依標準回 hay_last。一般 forward iterator 實作最壞
 * O(N*M)，bidirectional iterator 可從後方改善常數；演算法唯讀。
 *
 * 它和 std::search（第一個匹配）成對。回傳 iterator 不延長來源生命。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// LeetCode-style：回傳 pattern 最後一次出現 index；不存在回 -1。
int leetcode_last_substring_index(const std::string& text,
                                  const std::string& pattern) {
    if (pattern.empty()) {
        return static_cast<int>(text.size());
    }
    const auto it = std::find_end(text.begin(), text.end(),
                                  pattern.begin(), pattern.end());
    return it == text.end()
               ? -1
               : static_cast<int>(std::distance(text.begin(), it));
}

// 實務：binary log 中找最後一個同步 marker，從其後開始恢復解析。
std::size_t practical_resume_after_last_marker(
    const std::vector<int>& bytes, const std::vector<int>& marker) {
    if (marker.empty()) {
        return bytes.size();
    }
    const auto it = std::find_end(bytes.begin(), bytes.end(),
                                  marker.begin(), marker.end());
    if (it == bytes.end()) {
        return bytes.size();
    }
    return static_cast<std::size_t>(std::distance(bytes.begin(), it)) + marker.size();
}

int main() {
    const std::string text = "abc--abc--ab";
    const std::string needle = "abc";
    assert(std::find_end(text.begin(), text.end(), needle.begin(), needle.end()) ==
           text.begin() + 5);

    assert(leetcode_last_substring_index("mississippi", "issi") == 4);
    assert(leetcode_last_substring_index("abc", "z") == -1);
    assert(leetcode_last_substring_index("abc", "") == 3);

    const std::vector<int> bytes{1, 9, 9, 2, 9, 9, 3};
    assert(practical_resume_after_last_marker(bytes, {9, 9}) == 6U);
    assert(practical_resume_after_last_marker(bytes, {7}) == bytes.size());
    std::cout << "find_end：LeetCode-style 最後匹配與實務 log marker 測試通過\n";
}

/*
 * 易錯陷阱：pattern.empty() 的業務語意常和標準結果不同，應在 API 層先定義。本例
 * LeetCode-style 模仿 string search 慣例回 text.size()，實務 marker 則拒絕其意義。
 *
 * 面試：大量字串搜尋可用 KMP/Boyer-Moore/searcher；find_end 的 generic 最壞成本
 * 可能高。實務 binary marker 也可能出現在 payload 中，需要 escaping、length framing
 * 或 checksum，不能只靠 find_end 當完整 protocol parser。練習：回所有匹配位置。
 * LeetCode 若允許 overlapping matches，find_end 仍可找到最後一個；列舉全部時下一次
 * 起點加 1 才保留 overlap，若加 pattern.size 則只找不重疊匹配。
 */
