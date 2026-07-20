/*
 * std::equal：逐元素判斷兩範圍是否相等
 * ====================================
 * 四 iterator overload 能驗兩邊長度；舊的三 iterator overload 只知道第二範圍起點，
 * 呼叫者必須保證至少和第一範圍一樣長。時間 O(min length)，遇第一個不等可短路。
 * predicate 應表達 equivalence relation。演算法唯讀、不保存 iterator。
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <string>

// LeetCode 242：Valid Anagram。排序兩份後用 equal；O(N log N)。
bool leetcode_is_anagram(std::string first, std::string second) {
    if (first.size() != second.size()) {
        return false;
    }
    std::sort(first.begin(), first.end());
    std::sort(second.begin(), second.end());
    return std::equal(first.begin(), first.end(), second.begin(), second.end());
}

// 實務：比較 protocol magic bytes；四 iterator overload 同時驗長度。
bool practical_matches_signature(const std::string& payload,
                                 const std::string& expected) {
    return std::equal(payload.begin(), payload.end(), expected.begin(), expected.end());
}

int main() {
    const std::string a = "Hello";
    const std::string b = "hello";
    assert(!std::equal(a.begin(), a.end(), b.begin(), b.end()));
    assert(std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](char lhs, char rhs) {
                          return std::tolower(static_cast<unsigned char>(lhs)) ==
                                 std::tolower(static_cast<unsigned char>(rhs));
                      }));

    assert(leetcode_is_anagram("anagram", "nagaram"));
    assert(!leetcode_is_anagram("rat", "car"));
    assert(practical_matches_signature("PNG", "PNG"));
    assert(!practical_matches_signature("PNG-extra", "PNG"));
    assert(!practical_matches_signature("PN", "PNG"));
    std::cout << "equal：LeetCode 242 與實務 signature 比較測試通過\n";
}

/*
 * 易錯陷阱：對 signed char 直接傳 std::tolower 可能未定義，需先轉 unsigned char。
 * case-insensitive Unicode 不是逐 byte tolower，實務要使用 Unicode normalization/case
 * folding library。安全 signature 也不等於密碼學 constant-time compare；std::equal
 * 會在第一個差異短路，可能洩漏 timing。
 *
 * 面試：equal 與 is_permutation 差別？equal 要同位置相等；is_permutation 只要求
 * 多重集合相同。LeetCode anagram 可用 26-count 降到 O(N)，但需字元集約束。
 *
 * 【LeetCode 測試策略】
 * 必測長度不同、同字母不同次數、空字串與完全相同。sort 會改 local copies，不會
 * 改 caller；若參數改成 const reference 就要另建副本或採 frequency table。
 *
 * 【實務 signature 邊界】
 * 檔案 magic 通常只比較 payload prefix，不是整份 payload 等於 magic。本例函式刻意
 * 比整段，是為展示四 iterator equal；真實 parser 應先驗 payload.size>=magic.size，
 * 再比較 prefix。版本、endianness 與 checksum 仍需額外驗證。
 *
 * 練習：實作 practical_has_prefix，使用 equal 且安全處理短 payload。
 * 再加入空 magic、完全相等、payload 較短與較長四組測試。
 * 面試回答時先指出比較的是 sequence equality，不是物件 identity。
 */
