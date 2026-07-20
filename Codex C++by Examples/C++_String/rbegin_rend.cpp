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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 917. Reverse Only Letters（只反轉字母）
// 題目：反轉所有英文字母的位置，非字母維持原索引；例如 `a-bC-dEf-ghIj` 變 `j-Ih-gfE-dCba`。
// 為何使用本章主題：正向 iterator 找左側字母，reverse_iterator 找右側字母；base()-1
//       用來判斷兩者是否尚未交錯，無需把右端轉成 size 索引。
// 思路：1. 左右 iterator 由兩端開始；2. 各自跳過非 ASCII 字母；3. 交換字母並向內前進。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 text 長度，空間來自按值輸入副本。
// 易錯點：reverse_iterator::base() 指向目前元素後一格；不可對 rend 或無效位置盲目做 base()-1。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】由右側擷取副檔名
// 情境：從 `archive.tar.gz` 取得 `gz`；沒有點或檔名以點結尾時回空。
// 為何使用本章主題：rbegin/rend 讓 std::find 從尾端找第一個點；命中時 dot.base()
//       正好指向點後第一字元，可直接建構副檔名。
// 設計：1. 反向搜尋 `.`；2. 未找到或點在最後就回空；3. 由 base() 複製到 end。
// 成本：搜尋時間 O(N)、額外空間 O(E)，N 是 filename 長度，E 是副檔名長度。
// 上線注意：hidden file、多重副檔名、目錄中的點與大小寫政策未處理；正式路徑應用 filesystem。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'rbegin_rend.cpp' -o '/tmp/codex_cpp_C_String_rbegin_rend' && '/tmp/codex_cpp_C_String_rbegin_rend'
//
// === 預期輸出（節錄）===
// rbegin/rend: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
