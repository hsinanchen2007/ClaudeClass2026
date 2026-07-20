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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 28. Find the Index of the First Occurrence in a String（找出字串中第一個匹配項的索引）
// 題目：回傳 needle 在 haystack 中第一次出現的位置，不存在回 -1；例如
// sadbutsad/sad 回 0，空 needle 回 0。
// 為何使用本章主題：std::search 直接尋找第一個完整連續子序列，回傳 iterator 可轉成
// 索引；面試若要求手寫，仍需能提供 KMP 等演算法。
// 思路：1. 搜尋 haystack 與 needle 完整範圍；2. end 表示未命中；3. 否則計算起點距離。
// 複雜度：generic 最壞時間 O(N*M)、額外空間 O(1)，N/M 為 haystack/needle 長度。
// 易錯點：空 needle 會回 begin；索引轉 int 前需由題目約束保證可表示。
// -----------------------------------------------------------------------------
int leetcode_str_str(const std::string& haystack, const std::string& needle) {
    const auto it = std::search(haystack.begin(), haystack.end(),
                                needle.begin(), needle.end());
    return it == haystack.end()
               ? -1
               : static_cast<int>(std::distance(haystack.begin(), it));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Byte Stream Protocol Magic 搜尋
// 情境：解析器在 unsigned byte stream 中找第一個 magic pattern 起點，找不到時回
// bytes.size()，供呼叫端判斷是否需要更多資料。
// 為何使用本章主題：std::search 可對任意相等可比序列找連續 pattern，不必轉成
// std::string，也避免 signed char 對 binary byte 的歧義。
// 設計：1. 搜尋 bytes/magic 完整範圍；2. 將命中或 end iterator 統一轉成 offset。
// 成本：generic 最壞時間 O(N*M)、額外空間 O(1)，N/M 為 stream/pattern 長度。
// 上線注意：空 magic 會命中 0；magic 也可能出現在 payload，完整協定需 framing、長度與 checksum。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'search.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_search' && '/tmp/codex_cpp_C_Algorithm_non_modifying_search'
//
// === 預期輸出（節錄）===
// search：LeetCode 28 與實務 binary magic 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
