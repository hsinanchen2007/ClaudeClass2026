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

// LeetCode 392（Is Subsequence）：字元相等比較配合雙指標。
bool leetcode_is_subsequence(const std::string& candidate, const std::string& text) {
    std::size_t matched = 0U;
    for (const char ch : text) {
        if (matched < candidate.size() && candidate[matched] == ch) {
            ++matched;
        }
    }
    return matched == candidate.size();
}

// 實務：版本字串若直接字典序排序會錯；先解析數字元件再比較。
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
