/*
 * find_first_not_of(set)：找第一個「不屬於排除集合」的字元
 *
 * 最常用於略過前導空白、零或 delimiter。若所有字元都在集合中，回傳 npos。
 * 注意 set 是 byte/code-unit 集合；`" \t\r\n"` 只涵蓋列出的 ASCII whitespace。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view ascii_space = " \t\r\n";

void basic_demo() {
    const std::string text = "\t  value";
    assert(text.find_first_not_of(ascii_space) == 3U);
    assert(std::string(" \n").find_first_not_of(ascii_space) == std::string::npos);
}

// LeetCode 58（Length of Last Word）的前置 helper：先找第一個非空白來判全空白。
int leetcode_count_leading_spaces(const std::string& text) {
    const std::size_t first = text.find_first_not_of(' ');
    return first == std::string::npos ? static_cast<int>(text.size()) : static_cast<int>(first);
}

// 實務：回傳零拷貝左 trim view；底層 input 必須活得比結果久。
std::string_view practical_ltrim(const std::string_view input) {
    const std::size_t first = input.find_first_not_of(ascii_space);
    return first == std::string_view::npos ? std::string_view{} : input.substr(first);
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_count_leading_spaces("   fly") == 3);
    assert(leetcode_count_leading_spaces("   ") == 3);
    const std::string line = "\t  key=value  ";
    assert(practical_ltrim(line) == "key=value  ");
    assert(practical_ltrim(" \n\t").empty());
    std::cout << "find_first_not_of: tests passed\n";
}

/*
 * 【選擇表】find_first_of 找「第一個在集合內」；find_first_not_of 找「第一個在集合外」。
 * 【陷阱】npos 不能直接拿來 substr；本例全空白時回空 view。
 * 【生命週期】ltrim 回 view，不擁有資料；不要傳 temporary std::string 並把 view 保存。
 * 【面試】為何不用 std::isspace？集合版本規格固定、無 locale；isspace 要處理 unsigned char。
 * 【練習】讓 whitespace 集合可由呼叫者傳入，並測試只 trim 空格、不 trim tab。
 */

/*
 * 【四個集合搜尋 API 對照】
 * - find_first_of：左起第一個在 set 內。
 * - find_last_of：右起第一個在 set 內。
 * - find_first_not_of：左起第一個不在 set 內。
 * - find_last_not_of：右起第一個不在 set 內。
 * 它們都把參數當「char 集合」，不是 regex character class，也不是 substring。
 *
 * 【trim 左側的安全寫法】
 * 若結果 npos，代表全部需移除，回空；否則 substr(pos) 或 remove_prefix(pos)。
 * owning substr 配置，view remove_prefix 不配置但依賴來源生命週期。
 *
 * 【面試題】傳入空 set 會如何？沒有字元屬於空集合，因此 first_not_of 從 pos 即成功
 * （只要 pos 在範圍）；產品程式若不允許空 set 要先驗證。
 * 【練習】用回傳位置直接 erase 前綴，並對全排除字元輸入明確 clear。
 */
