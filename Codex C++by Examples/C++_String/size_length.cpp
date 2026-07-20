/*
 * size() / length()：目前擁有的 char（code unit）數量
 *
 * 兩者完全同義且 O(1)。回傳 size_type（通常是無號整數），不是 int。對 UTF-8，
 * size 是 byte/code-unit 數，不是使用者看到的 Unicode 字元數。與 int 比較或倒序
 * 運算時要小心無號 underflow。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    const std::string binary("A\0B", 3U);
    assert(binary.size() == 3U);
    assert(binary.length() == binary.size());
}

// LeetCode 58（Length of Last Word）。
int leetcode_length_of_last_word(const std::string& text) {
    std::size_t end = text.size();
    while (end > 0U && text[end - 1U] == ' ') {
        --end;
    }
    std::size_t begin = end;
    while (begin > 0U && text[begin - 1U] != ' ') {
        --begin;
    }
    return static_cast<int>(end - begin);
}

// 實務：協定限制以 bytes 計算；此處假設 payload 已是 UTF-8 bytes。
bool practical_fits_protocol_field(const std::string& payload, const std::size_t max_bytes) {
    return payload.size() <= max_bytes;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_length_of_last_word("Hello World") == 5);
    assert(leetcode_length_of_last_word("a ") == 1);
    assert(practical_fits_protocol_field("1234", 4U));
    assert(!practical_fits_protocol_field("12345", 4U));
    std::cout << "size/length: tests passed\n";
}

/*
 * 【陷阱】`for (size_t i=s.size()-1; i>=0; --i)` 永不以 i<0 結束，空字串還先 underflow。
 * 【面試】UTF-8 的 "中" size 是多少？通常 3 bytes，但不是標準對字形數的保證。
 * 【練習】寫一個安全倒序迴圈 `for (size_t i=s.size(); i-- > 0;)` 並測空字串。
 */

/*
 * 【長度觀念四分法】
 * - size/length：char code units。
 * - strlen(c_str)：第一個 NUL 前的 bytes，可能小於 size。
 * - UTF-8 code points：需解碼，不能直接用 size。
 * - 使用者可見 grapheme clusters：還需 Unicode segmentation。
 *
 * 【型別安全】size_type 通常是 size_t；轉 int 前先證明不超過 INT_MAX。signed/unsigned
 * 比較可能把負值轉成巨大無號值。索引差值需要帶負數時用 difference_type。
 *
 * 【面試題】size 是否要遍歷？對 std::string 是 O(1)，容器保存長度。
 * 【陷阱】`size()-1` 在空字串 underflow；先判空或用從 size 開始的倒序模板。
 * 【練習】寫 `checked_int_size`，過大時回 optional<int>。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'size_length.cpp' -o '/tmp/codex_cpp_C_String_size_length' && '/tmp/codex_cpp_C_String_size_length'
//
// === 預期輸出（節錄）===
// size/length: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
