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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 58. Length of Last Word（最後一個單字的長度）
// 題目：忽略句尾空格，回傳最後一個單字的字元數；"a " 回 1。
// 為何使用本章主題：size() 提供 O(1) 尾後索引，兩個由 size 開始的倒序邊界避免空字串下溢。
// 思路：1. end=size；2. 向左略過尾空格；3. begin 向左走到前一空格；4. 回 end-begin。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 長度。
// 易錯點：不可從 size()-1 開始處理空字串；size 是 bytes/code units，轉 int 前應證明不溢位。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】協定欄位 byte 上限檢查
// 情境：wire protocol 的 payload 欄位最多 max_bytes，規格明確按 UTF-8 encoded bytes 而非人眼字數。
// 為何使用本章主題：string::size() 正好回 char code-unit/byte 數，無需 strlen，且能正確計入內嵌 NUL。
// 設計：1. 讀 payload.size；2. 與 max_bytes 做無號比較；3. 不修改或重新編碼內容。
// 成本：時間 O(1)、額外空間 O(1)。
// 上線注意：若產品限制 code points 或顯示字形，size 不適用；檢查通過後仍要驗編碼合法性與其他協定欄位。
// -----------------------------------------------------------------------------
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
