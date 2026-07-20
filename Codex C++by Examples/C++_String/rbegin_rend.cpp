/*
 * rbegin()/rend()：反向 iterator 範圍
 *
 * rbegin 指向最後一個元素，rend 指向第一個元素之前的反向哨兵。走訪仍是 O(n)，
 * 不會真的反轉字串。reverse_iterator::base() 指向「目前元素的下一個正向位置」，
 * 這個 off-by-one 是面試與實務最常見陷阱。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    const std::string text = "abc";
    const std::string reversed(text.rbegin(), text.rend());
    assert(reversed == "cba");
}

// LeetCode 917（Reverse Only Letters）的雙指標版本。
bool is_ascii_letter(const char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

std::string leetcode_reverse_only_letters(std::string text) {
    auto left = text.begin();
    auto right = text.rbegin();
    while (left != text.end() && right != text.rend() && left < right.base() - 1) {
        if (!is_ascii_letter(*left)) {
            ++left;
        } else if (!is_ascii_letter(*right)) {
            ++right;
        } else {
            std::iter_swap(left, right);
            ++left;
            ++right;
        }
    }
    return text;
}

// 實務：從右側找副檔名；找到的 reverse iterator 對應正向 iterator 要用 base()-1。
std::string practical_extension_of(const std::string& filename) {
    const auto dot = std::find(filename.rbegin(), filename.rend(), '.');
    if (dot == filename.rend() || dot == filename.rbegin()) {
        return {};
    }
    return std::string(dot.base(), filename.end());
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_reverse_only_letters("a-bC-dEf-ghIj") == "j-Ih-gfE-dCba");
    assert(practical_extension_of("archive.tar.gz") == "gz");
    assert(practical_extension_of("README").empty());
    std::cout << "rbegin/rend: tests passed\n";
}

/*
 * 【面試】若 rit 指向 '.'，rit.base() 指哪？指 '.' 右邊的正向位置。
 * 【陷阱】對 rend() 做 base()-1 可能越界；先確認找到元素。
 * 【練習】以反向 iterator 實作 ends_with（不使用 C++20 ends_with）。
 */

/*
 * 【reverse_iterator 面試速查】
 * - `*rit` 對應正向 iterator `*(rit.base()-1)`。
 * - rbegin().base()==end()；rend().base()==begin()。
 * - 空字串 rbegin()==rend，兩者都不可解參考。
 * - 反向走訪不修改字串；用它建構新 string 才會 O(n) 複製。
 * 【陷阱】先對 rend().base() 做 -1 會跑到 begin 前方；必須先確認 rit!=rend。
 */
