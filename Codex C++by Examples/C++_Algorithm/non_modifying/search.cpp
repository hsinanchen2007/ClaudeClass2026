/*
 * std::search：尋找子序列第一次出現位置
 * ======================================
 * search(haystack,needle) 回第一個完整匹配的起點，找不到回 haystack.end()；空 needle
 * 回 haystack.begin()。generic 版本最壞 O(N*M)。C++17 可搭配 Boyer-Moore searcher
 * 最佳化重複字串查詢，但需 random-access 與適合的 hash/equality。
 *
 * std::find 找單一值；search 找一段 pattern；find_end 找最後一段。演算法唯讀。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 28：Find the Index of the First Occurrence in a String。
int leetcode_str_str(const std::string& haystack, const std::string& needle) {
    const auto it = std::search(haystack.begin(), haystack.end(),
                                needle.begin(), needle.end());
    return it == haystack.end()
               ? -1
               : static_cast<int>(std::distance(haystack.begin(), it));
}

// 實務：在 byte stream 尋找 protocol magic，回 size sentinel 表示未找到。
std::size_t practical_find_magic(const std::vector<unsigned char>& bytes,
                                 const std::vector<unsigned char>& magic) {
    const auto it = std::search(bytes.begin(), bytes.end(), magic.begin(), magic.end());
    return static_cast<std::size_t>(std::distance(bytes.begin(), it));
}

int main() {
    const std::vector<int> values{1, 2, 3, 2, 3, 4};
    const std::vector<int> pattern{2, 3};
    assert(std::search(values.begin(), values.end(), pattern.begin(), pattern.end()) ==
           values.begin() + 1);

    assert(leetcode_str_str("sadbutsad", "sad") == 0);
    assert(leetcode_str_str("leetcode", "leeto") == -1);
    assert(leetcode_str_str("abc", "") == 0);

    const std::vector<unsigned char> bytes{0U, 1U, 0xCAU, 0xFEU, 2U};
    assert(practical_find_magic(bytes, {0xCAU, 0xFEU}) == 2U);
    assert(practical_find_magic(bytes, {0xFFU}) == bytes.size());
    std::cout << "search：LeetCode 28 與實務 binary magic 測試通過\n";
}

/*
 * 易錯陷阱：空 needle 的標準語意是 begin；API 若不接受空 pattern 要先拒絕。
 * UTF-8 std::string search 是 byte subsequence，未必等於使用者看到的 Unicode 文字搜尋。
 *
 * 面試：KMP 可保證 O(N+M)，Boyer-Moore 在常見文字可跳躍。LeetCode 面試若要求手寫
 * 不可只呼叫 search，應能說明 prefix table。實務 protocol magic 可能出現在 payload，
 * 仍需 length/checksum framing。練習：從上一匹配尾後繼續找所有不重疊位置。
 *
 * 【LeetCode 邊界】
 * pattern 比 text 長自然找不到；空 pattern 回 0；重複 pattern 回最左。轉 int index
 * 前，極大字串 distance 可能超過 int，題目約束需保證或 API 改 ptrdiff_t/size_t。
 *
 * 【實務搜尋器】
 * 同一 pattern 重複查很多 payload 時，可預建 boyer_moore_searcher，避免每次重做
 * preprocessing。小 pattern/小資料 generic search 反而常更簡單快速。
 *
 * 易錯：自訂 predicate 不可把大小寫比較寫成 locale-dependent 且不同呼叫不一致；
 * binary protocol 一律比較 unsigned byte，避免 char signedness。
 * 測試 searcher 最佳化前後只能比較結果，不應假設內部比較次數完全固定。
 * 練習：加入 overlapping pattern `aaaa` / `aa`，列出所有起點。
 */
