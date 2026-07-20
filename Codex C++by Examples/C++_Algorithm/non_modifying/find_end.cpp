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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 28. Find the Index of the First Occurrence in a String（找出字串中第一個匹配項的索引）
// 題目：原題要求 needle 在 haystack 的第一個索引；本 helper 教學改成找最後一次
// pattern，例如 mississippi/issi 回 4，不是 LC28 的正式輸出契約。
// 為何使用本章主題：find_end 專門回最後一個子序列起點，藉由改寫 LC28 搜尋情境
// 對照 std::search 的第一匹配語意。
// 思路：1. 空 pattern 依此 helper 契約回 text.size()；2. find_end 找最後匹配；
// 3. end 回 -1，否則轉成索引。
// 複雜度：最壞時間 O(N*M)、額外空間 O(1)，N/M 為 text/pattern 長度。
// 易錯點：此函式不是 LC28 提交解；空 pattern 的回值也刻意不同於原題要求的 0。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】Binary Log 最後同步點恢復
// 情境：bytes 中可能有多個同步 marker；程序重啟後要找到最後一個完整 marker，從
// 其尾後 offset 繼續解析，找不到或 marker 空時回 bytes.size()。
// 為何使用本章主題：find_end 直接取得最後一次子序列匹配，不必反轉 byte stream，
// 且保留原資料供後續 checksum/audit。
// 設計：1. 拒絕空 marker 的恢復語意；2. 找最後匹配；3. 未命中回 size；4. 命中回
// 起點距離加 marker 長度。
// 成本：最壞時間 O(N*M)、額外空間 O(1)，N/M 為 bytes/marker 長度。
// 上線注意：marker 可能自然出現在 payload，完整協定仍需 escaping、length framing 與 checksum。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'find_end.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_find_end' && '/tmp/codex_cpp_C_Algorithm_non_modifying_find_end'
//
// === 預期輸出（節錄）===
// find_end：LeetCode-style 最後匹配與實務 log marker 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
