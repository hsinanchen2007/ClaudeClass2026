/*
 * 字串比較運算子：==、!=、<、<=、>、>=（C++20 亦支援 <=>）
 *
 * 比較是逐字典序比較 char 的 code unit，不是自然語言排序、大小寫忽略或數值排序。
 * 例如 "file10" < "file2"，因為第一個不同字元 '1' < '2'。最壞複雜度 O(min(n,m))。
 * C++20 可直接比較 std::string 與 C 字串；不要拿兩個 const char* 用 == 比內容。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    const std::string a = "apple";
    const std::string b = "apricot";
    assert(a < b);
    assert(a == "apple");
    assert("file10" < std::string("file2"));
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 392. Is Subsequence（判斷子序列）
// 題目：判斷 candidate 是否可由 text 刪除若干字元後取得且順序不變；"abc" 對 "ahbgdc" 為 true。
// 為何使用本章主題：operator== 比較目前候選字元與掃描字元，配合單向 matched 索引即可完成，
//       不需要建立 substring 或排序。
// 思路：1. matched 指向 candidate 下一個需求；2. 掃 text；3. 字元相等就前進；4. 最後看是否全匹配。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 長度；candidate 只以索引讀取。
// 易錯點：candidate 為空應回 true；matched 未小於 candidate.size() 時不可再索引。
// -----------------------------------------------------------------------------
bool leetcode_is_subsequence(const std::string& candidate, const std::string& text) {
    std::size_t matched = 0U;
    for (const char ch : text) {
        if (matched < candidate.size() && candidate[matched] == ch) {
            ++matched;
        }
    }
    return matched == candidate.size();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】數值版本排序
// 情境：要讓 1.9 排在 1.12 前；直接比較字串會因字典序得到錯誤結果。
// 為何使用本章主題：本例刻意不用字串比較運算子，而是先把版本解析成整數欄位後比較，
//       展示相對替代方案：operator< 適合 code-unit 字典序，不適合數值版本語意。
// 設計：1. 先比較 major；2. major 相同再比較 minor；3. 將 comparator 交給 std::sort。
// 成本：比較一次 O(1)，排序 V 筆為 O(V log V)、額外空間依 std::sort 實作通常 O(log V)。
// 上線注意：真實 SemVer 還有 prerelease/build metadata 與溢位；parser 必須先驗證，comparator
//       必須維持 strict weak ordering。
// -----------------------------------------------------------------------------
struct Version {
    int major{};
    int minor{};
};

bool practical_version_less(const Version lhs, const Version rhs) {
    if (lhs.major != rhs.major) {
        return lhs.major < rhs.major;
    }
    return lhs.minor < rhs.minor;
}

void practical_version_sort_test() {
    std::vector<Version> versions{{2, 0}, {1, 12}, {1, 9}};
    std::sort(versions.begin(), versions.end(), practical_version_less);
    assert(versions.front().major == 1 && versions.front().minor == 9);
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_is_subsequence("abc", "ahbgdc"));
    assert(!leetcode_is_subsequence("axc", "ahbgdc"));

    practical_version_sort_test();
    std::cout << "comparison operators: tests passed\n";
}

/*
 * 【陷阱】locale、Unicode 正規化、大小寫折疊都不是 operator< 的工作。
 * 【面試】const char* p, *q; p == q 比什麼？比位址；用 string/string_view 才比內容。
 * 【練習】實作不分 ASCII 大小寫的比較器；轉 cctype 前記得轉 unsigned char。
 */

/*
 * 【考前速查】== 只問相等；關係運算子做字典序；compare 適合片段三向比較。
 * C++20 的 <=> 可推導其他關係，但 std::string 的結果仍是字典序，而非自然排序。
 * 比較結果不配置，最壞檢查到第一個不同字元或較短字串結尾。
 * 【面試題】"Z" < "a" 是否符合語系字典？它按 char traits/code units，不按人類語系。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'comparison_operators.cpp' -o '/tmp/codex_cpp_C_String_comparison_operators' && '/tmp/codex_cpp_C_String_comparison_operators'
//
// === 預期輸出（節錄）===
// comparison operators: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
