/*
 * std::find_first_of：在第一範圍找「屬於候選集合」的第一個元素
 * =============================================================
 * 對第一範圍每個元素，與第二範圍逐一比較；找第一個匹配位置，沒有則回 first_last。
 * 一般複雜度最壞 O(N*M)。兩範圍角色不同：回傳位置永遠來自第一範圍。
 *
 * 若候選集合很大且可 hash，unordered_set + find_if 平均可降為 O(N)；小型 delimiter
 * 集合用 find_first_of 更簡潔。演算法唯讀、不保存 iterator。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 771：Jewels and Stones。反覆找下一顆屬於 jewels 的 stone。
int leetcode_num_jewels_in_stones(const std::string& jewels,
                                  const std::string& stones) {
    int count = 0;
    auto current = stones.begin();
    while (current != stones.end()) {
        const auto found = std::find_first_of(current, stones.end(),
                                              jewels.begin(), jewels.end());
        if (found == stones.end()) {
            break;
        }
        ++count;
        current = std::next(found);
    }
    return count;
}

// 實務：找 CSV-like 輸入的第一個可接受 delimiter。
std::size_t practical_first_delimiter(const std::string& line,
                                      const std::string& delimiters) {
    const auto it = std::find_first_of(line.begin(), line.end(),
                                       delimiters.begin(), delimiters.end());
    return static_cast<std::size_t>(std::distance(line.begin(), it));
}

int main() {
    const std::vector<int> input{8, 3, 7, 2};
    const std::vector<int> candidates{5, 7};
    assert(std::find_first_of(input.begin(), input.end(),
                              candidates.begin(), candidates.end()) ==
           input.begin() + 2);

    assert(leetcode_num_jewels_in_stones("aA", "aAAbbbb") == 3);
    assert(leetcode_num_jewels_in_stones("z", "ZZ") == 0);

    assert(practical_first_delimiter("name,value;rest", ",;|") == 4U);
    assert(practical_first_delimiter("plain", ",;|") == 5U);
    std::cout << "find_first_of：LeetCode 771 與實務 delimiter 測試通過\n";
}

/*
 * 易錯陷阱：第二範圍空時一定找不到；候選字元的大小寫由 == 決定。自訂 predicate
 * 參數是 first-range element、second-range candidate，異質型別時方向要正確。
 *
 * 面試：本 LeetCode 解 O(N*M)，ASCII 字元可用 bool[256] 降到 O(N+M)，並將 char
 * 轉 unsigned char 當 index。實務 parser 不能只找 delimiter，還要處理 quote/escape；
 * `"a,b"` 內的逗號通常不是欄位界線。練習：實作忽略 quoted delimiter 的掃描器。
 *
 * 測試要含候選集合空、命中第一/最後位置、完全不命中與重複候選。LeetCode jewels
 * 若保證每種 jewel 唯一，演算法不需去重；一般 API 若 candidates 重複，結果位置
 * 不變但比較工作增加。實務 delimiter 很小時 O(N*M) 通常可接受。
 * 練習：將 delimiter 預建成 256 格 lookup table，保留相同測試。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'find_first_of.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_find_first_of' && '/tmp/codex_cpp_C_Algorithm_non_modifying_find_first_of'
//
// === 預期輸出（節錄）===
// find_first_of：LeetCode 771 與實務 delimiter 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
