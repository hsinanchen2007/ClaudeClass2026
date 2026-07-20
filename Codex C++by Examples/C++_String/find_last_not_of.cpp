/*
 * find_last_not_of(set)：找最後一個不屬於排除集合的字元
 *
 * 最常與 find_first_not_of 組成 trim。若字串全由排除字元構成，回 npos；找到的 end
 * 是「最後保留索引」，因此切片長度為 end-begin+1。無號索引的 +1/-1 要先判 npos。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view ascii_space = " \t\r\n";

void basic_demo() {
    const std::string text = "value \t\n";
    assert(text.find_last_not_of(ascii_space) == 4U);
    assert(std::string(" \n").find_last_not_of(ascii_space) == std::string::npos);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 58. Length of Last Word（最後一個單字的長度）
// 題目：忽略句尾空格後，回傳最後一個由非空格字元組成的單字長度；全空白則為 0。
// 為何使用本章主題：find_last_not_of(' ') 直接找最後一個有效字元，再找其前方空格，
//       不需建立 trim 後的暫存字串。
// 思路：1. 從右找非空格 end；2. 全空白回 0；3. 找 end 之前最後空格；4. 由邊界差算長度。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 長度。
// 易錯點：end==npos 時不可做加減；`before+1` 也只能在 before 不是 npos 時執行。
// -----------------------------------------------------------------------------
int leetcode_length_of_last_word(const std::string& text) {
    const std::size_t end = text.find_last_not_of(' ');
    if (end == std::string::npos) return 0;
    const std::size_t before = text.find_last_of(' ', end);
    return static_cast<int>(end - (before == std::string::npos ? 0U : before + 1U) + 1U);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定值雙側零拷貝 trim
// 情境：從設定行借用去除 space、tab、CR、LF 後的內容，例如 ` timeout=30 ` 得 timeout=30。
// 為何使用本章主題：first_not_of 與 last_not_of 直接給左右保留邊界，string_view::substr
//       只調整 pointer/length，相較 owning substr 不配置。
// 設計：1. 找左側第一個非空白；2. 全空白回空 view；3. 找右側最後非空白；4. 回傳閉區間切片。
// 成本：時間 O(N)、額外空間 O(1)，N 是 input 長度，結果為借用 view。
// 上線注意：ASCII 集合不等於 Unicode whitespace；來源必須比 view 活得久，owner 修改後要重取。
// -----------------------------------------------------------------------------
std::string_view practical_trim(const std::string_view input) {
    const std::size_t begin = input.find_first_not_of(ascii_space);
    if (begin == std::string_view::npos) return {};
    const std::size_t end = input.find_last_not_of(ascii_space);
    return input.substr(begin, end - begin + 1U);
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_length_of_last_word("   fly me   to   the moon  ") == 4);
    const std::string config = " \t timeout=30 \r\n";
    assert(practical_trim(config) == "timeout=30");
    assert(practical_trim("   ").empty());
    std::cout << "find_last_not_of: tests passed\n";
}

/*
 * 【陷阱】`erase(find_last_not_of(...) + 1)` 在全空白時 npos+1 wrap 成 0，結果雖可能
 * 碰巧清空，卻是難懂且脆弱；先明確判 npos。
 * 【面試】trim 能否正確處理 Unicode whitespace？此 byte 集合版本不能，需要 Unicode library。
 * 【練習】實作 owning `std::string practical_trim_copy(...)`，比較配置與生命週期。
 */

/*
 * 【右側清理模板】
 * `end=s.find_last_not_of(chars)`；若 npos，整串都應清除；否則 erase(end+1)。
 * end+1 只有在確認 end!=npos 後才安全。若只需 view，remove_suffix(size-end-1) O(1)。
 *
 * 【集合語意】參數每個 char 都是獨立候選；"\r\n" 代表 CR 或 LF，不要求 CRLF 成對。
 * 【複雜度】直觀 O(n*m)；m 很小時足夠，Unicode whitespace 請用專用 library。
 * 【面試題】為何 trim 常用 not_of？因為目標是定位第一/最後「要保留」的字元。
 * 【練習】寫 inplace rtrim，並測空、全空白、無空白及內嵌空白。
 *
 * 【選擇提醒】只去一個已知尾字元可用 back+pop_back；去「任意長度的一組尾字元」
 * 才適合 find_last_not_of。前者反覆迴圈也是 O(n)，後者較直接回分界索引。
 * 若結果要保存且來源很快銷毀，回 string 副本；若只在當前呼叫鏈使用，可回 view。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'find_last_not_of.cpp' -o '/tmp/codex_cpp_C_String_find_last_not_of' && '/tmp/codex_cpp_C_String_find_last_not_of'
//
// === 預期輸出（節錄）===
// find_last_not_of: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
