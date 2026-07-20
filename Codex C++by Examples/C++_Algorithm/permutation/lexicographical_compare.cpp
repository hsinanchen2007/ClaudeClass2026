/*
 * std::lexicographical_compare：像字典般比較兩序列
 * ================================================
 * 從頭找第一個不等元素，以 comparator 決定較小者；若共同前綴全相同，較短範圍
 * 較小；完全相等則回 false。時間 O(min(N,M))，額外空間 O(1)。
 *
 * 這是 strict comparison：只回答 lhs < rhs。C++20 可用 lexicographical_compare_three_way
 * 取得三向結果，但元素比較器與 ordering category 要正確。
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 953：Verifying an Alien Dictionary；先映射 rank，再做字典序比較。
bool leetcode_is_alien_sorted(const std::vector<std::string>& words,
                              const std::string& order) {
    std::vector<int> rank(26, 0);
    for (std::size_t i = 0; i < order.size(); ++i) {
        rank[static_cast<std::size_t>(order[i] - 'a')] = static_cast<int>(i);
    }
    const auto less_or_equal = [&rank](const std::string& lhs,
                                       const std::string& rhs) {
        return !std::lexicographical_compare(
            rhs.begin(), rhs.end(), lhs.begin(), lhs.end(),
            [&rank](char a, char b) {
                return rank[static_cast<std::size_t>(a - 'a')] <
                       rank[static_cast<std::size_t>(b - 'a')];
            });
    };
    return std::adjacent_find(words.begin(), words.end(),
                              [&less_or_equal](const std::string& lhs,
                                               const std::string& rhs) {
                                  return !less_or_equal(lhs, rhs);
                              }) == words.end();
}

// 實務：比較 semantic version 的數字段，1.10 > 1.2，不用字串字典序誤判。
bool practical_version_less(const std::vector<int>& lhs,
                            const std::vector<int>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                        rhs.begin(), rhs.end());
}

int main() {
    const std::string apple = "apple";
    const std::string application = "application";
    assert(std::lexicographical_compare(apple.begin(), apple.end(),
                                        application.begin(), application.end()));
    assert(!std::lexicographical_compare(application.begin(), application.end(),
                                         apple.begin(), apple.end()));

    assert(leetcode_is_alien_sorted({"hello", "leetcode"},
                                    "hlabcdefgijkmnopqrstuvwxyz"));
    assert(!leetcode_is_alien_sorted({"apple", "app"},
                                     "abcdefghijklmnopqrstuvwxyz"));
    assert(practical_version_less({1, 2, 9}, {1, 10, 0}));
    assert(!practical_version_less({2, 0}, {1, 99}));

    std::cout << "lexicographical_compare：alien order 與版本比較測試通過\n";
}

/*
 * 易錯陷阱：
 * - 字串 "10" 字典序小於 "2"；版本、數字 ID 必須先 parse 成數字段。
 * - char 直接傳 std::tolower 對負 signed char 可能 UB；應轉 unsigned char。此例只
 *   處理題目限定小寫 ASCII，因此沒有做 locale-aware case folding。
 * - comparator 必須 strict weak ordering；用 <= 會破壞排序與比較契約。
 * - 相等時 lhs<rhs 與 rhs<lhs 都 false；不要把 false 解讀成「lhs 大於 rhs」。
 *
 * 面試：prefix 規則是 "app" < "apple"；第一個差異優先於後續所有元素。
 * C++20 spaceship 可簡化三向比較，但 locale/collation 仍不是單純 code-point 比較。
 *
 * 練習：完成完整 SemVer 比較（prerelease、build metadata）；這比整數 vector 多出
 * 規則，應先寫 parser，再把 canonical token 交給明確 comparator。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'lexicographical_compare.cpp' -o '/tmp/codex_cpp_C_Algorithm_permutation_lexicographical_compare' && '/tmp/codex_cpp_C_Algorithm_permutation_lexicographical_compare'
//
// === 預期輸出（節錄）===
// lexicographical_compare：alien order 與版本比較測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
