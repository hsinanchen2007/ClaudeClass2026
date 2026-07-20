/*
 * C++17 教科書：std::string_view
 *
 * string_view 是 non-owning contiguous character view，通常只有 pointer+length。建構與
 * substr 都是 O(1)，不配置、不複製字元；可 view string、literal 或 char buffer 的一段。
 * 它不保證 null termination，`.data()` 不能無條件傳給只收 C string 的 API。
 *
 * 【生命週期】原字串必須活得比 view 久；回傳 local string 的 view、保存 temporary string
 * 的 view、原 string reallocation 後繼續使用舊 view，都會 dangling。view 是借用，不是 owner。
 * 【API】讀取且不保存內容的參數很適合 string_view；若需跨 call 保存，複製成 std::string。
 * 【常見陷阱】temporary/local/reallocated string 會讓 view dangling，且 data() 不保證 NUL 結尾。
 * 【複雜度】compare/find 視內容通常 O(n*m) 或實作演算法；view 只省 ownership/配置。
 * 【面試題】string_view::substr 與 string::substr 最大差異是什麼？前者 O(1) 且不配置。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace basic {
std::pair<std::string_view, std::string_view> split_once(std::string_view text, char delimiter) {
    const std::size_t position = text.find(delimiter);
    if (position == std::string_view::npos) return {text, {}};
    return {text.substr(0U, position), text.substr(position + 1U)};
}

void demo() {
    constexpr std::string_view text = "key=value";
    const auto [key, value] = split_once(text, '=');
    assert(key == "key" && value == "value");
}
}  // namespace basic

namespace leetcode {
// LeetCode 28：Find the Index of the First Occurrence in a String。
// std::string_view::find 直接搜尋 view；平均實務表現依標準庫，介面不保證特定演算法。
int leetcode_str_str(std::string_view haystack, std::string_view needle) {
    const std::size_t position = haystack.find(needle);
    return position == std::string_view::npos ? -1 : static_cast<int>(position);
}

void leetcode_test() {
    assert(leetcode_str_str("sadbutsad", "sad") == 0);
    assert(leetcode_str_str("leetcode", "leeto") == -1);
}
}  // namespace leetcode

// 【實務案例】HTTP header 切片：零配置回傳 name/value view，並把 owner lifetime 寫入契約。
namespace practical {
struct HeaderView {
    std::string_view name;
    std::string_view value;
};

HeaderView practical_parse_header(std::string_view line) {
    const auto [name, value] = basic::split_once(line, ':');
    return {name, value.empty() ? value : value.substr(value.front() == ' ' ? 1U : 0U)};
}

void practical_test() {
    const std::string owned = "Content-Type: text/plain";
    const HeaderView header = practical_parse_header(owned);
    assert(header.name == "Content-Type" && header.value == "text/plain");
    // owned 必須維持不變且存活；header 只借用其字元。
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "string_view：借用/lifetime、substring search、header parse 測試通過\n";
}

// 【延伸練習】寫會保存 header 的版本：先示範 dangling 風險，再改成 owning std::string。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '11_string_view.cpp' -o '/tmp/codex_cpp_C_Cpp17_11_string_view' && '/tmp/codex_cpp_C_Cpp17_11_string_view'
//
// === 預期輸出（節錄）===
// string_view：借用/lifetime、substring search、header parse 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
