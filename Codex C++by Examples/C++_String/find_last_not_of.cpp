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

// LeetCode 58（Length of Last Word）。
int leetcode_length_of_last_word(const std::string& text) {
    const std::size_t end = text.find_last_not_of(' ');
    if (end == std::string::npos) return 0;
    const std::size_t before = text.find_last_of(' ', end);
    return static_cast<int>(end - (before == std::string::npos ? 0U : before + 1U) + 1U);
}

// 實務：完整 trim view，零配置；結果依賴 input 生命週期。
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
