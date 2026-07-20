/*
 * std::string::copy：把子區段複製到 caller-provided char buffer
 *
 * `copied = text.copy(dest,count,pos)` 最多複製 count 個字元，回傳實際數量；pos 超界
 * 丟 out_of_range。重要：它不會自動加 '\0'。目的 buffer 必須至少有 count 空間，
 * 這是低階 interop API；一般 C++ 切片更適合 substr/string_view。
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    const std::string text = "abcdef";
    std::array<char, 4U> buffer{};
    const std::size_t copied = text.copy(buffer.data(), 3U, 2U);
    buffer[copied] = '\0';  // copy 本身不做這件事。
    assert(copied == 3U);
    assert(std::string(buffer.data()) == "cde");
}

// LeetCode 28（Find the Index of the First Occurrence）：找到後 copy 出片段驗證。
int leetcode_find_first(const std::string& haystack, const std::string& needle) {
    const std::size_t position = haystack.find(needle);
    return position == std::string::npos ? -1 : static_cast<int>(position);
}

// 實務：寫入固定 8-byte 欄位，剩餘 bytes 填空白；不需要 NUL 結尾。
std::array<char, 8U> practical_fixed_width_field(const std::string& value) {
    std::array<char, 8U> field{};
    field.fill(' ');
    const std::size_t count = value.size() < field.size() ? value.size() : field.size();
    static_cast<void>(value.copy(field.data(), count));
    return field;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_find_first("sadbutsad", "sad") == 0);
    assert(leetcode_find_first("leetcode", "leeto") == -1);
    const auto field = practical_fixed_width_field("Ada");
    assert(std::string(field.begin(), field.end()) == "Ada     ");
    std::cout << "copy: tests passed\n";
}

/*
 * 【陷阱】配置 char dest[count] 後再寫 dest[count]='\0' 會越界；需要 count+1。
 * 【面試】copy 與 memcpy 差異？copy 知道 string 長度並處理 pos/count；兩者都需目的容量。
 * 【練習】practical_fixed_width_field 遇到過長輸入時改成回傳 optional，避免靜默截斷。
 */

/*
 * 【copy 的完整契約】
 * 1. dest 指向至少 count 個可寫 char；API 不知道 buffer 容量。
 * 2. pos 必須 <= size；pos==size 合法且複製 0，pos>size 丟 out_of_range。
 * 3. 實際量是 min(count,size-pos)，以回傳值為準。
 * 4. 不自動 NUL terminate；若要 C 字串，buffer 至少 count+1 並手動補 NUL。
 * 5. 二進位/fixed-width 欄位通常不需要 NUL，應保留長度或固定寬度。
 *
 * 【API 選擇】owning slice 用 substr；零拷貝唯讀 slice 用 string_view；必須寫入
 * 既有 C buffer 才用 copy。
 * 【面試題】copy 會改來源嗎？不會，它是 const member。
 * 【練習】讓 fixed-width encoder 回報截斷，而不是默默丟掉尾端資料。
 */
