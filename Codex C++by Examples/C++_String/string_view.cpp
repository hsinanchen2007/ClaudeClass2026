/*
 * std::string_view：不擁有資料的 `{pointer,length}` 唯讀視窗
 *
 * 建立、複製、remove_prefix/remove_suffix、substr 都是 O(1)；非常適合 parser 與只讀參數。
 * 它不保證 NUL 結尾，不能直接把 data() 當 C 字串；也不延長來源生命週期。來源 string
 * 銷毀或重新配置後，view 懸空。view 可含內嵌 NUL，size 仍精確。
 */

#include <cassert>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {

void basic_demo() {
    const std::string owner = "prefix:value:suffix";
    std::string_view view = owner;
    view.remove_prefix(7U);
    view.remove_suffix(7U);
    assert(view == "value");
    assert(view.data() == owner.data() + 7);
}

// LeetCode 125（Valid Palindrome）：零拷貝參數與雙索引。
bool leetcode_valid_palindrome(const std::string_view text) {
    std::size_t left = 0U;
    std::size_t right = text.size();
    while (left < right) {
        while (left < right &&
               std::isalnum(static_cast<unsigned char>(text[left])) == 0) ++left;
        while (left < right &&
               std::isalnum(static_cast<unsigned char>(text[right - 1U])) == 0) --right;
        if (left < right) {
            if (std::tolower(static_cast<unsigned char>(text[left])) !=
                std::tolower(static_cast<unsigned char>(text[right - 1U]))) return false;
            ++left;
            --right;
        }
    }
    return true;
}

// 實務：零配置切割第一個 delimiter；兩個 view 都依賴 input 來源生命週期。
std::optional<std::pair<std::string_view, std::string_view>>
practical_split_once(const std::string_view input, const char delimiter) {
    const std::size_t position = input.find(delimiter);
    if (position == std::string_view::npos) return std::nullopt;
    return std::pair{input.substr(0U, position), input.substr(position + 1U)};
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_valid_palindrome("A man, a plan, a canal: Panama"));
    assert(!leetcode_valid_palindrome("race a car"));
    const std::string config = "timeout=30";
    const auto fields = practical_split_once(config, '=');
    assert(fields.has_value());
    assert(fields->first == "timeout" && fields->second == "30");
    assert(!practical_split_once("missing", '=').has_value());
    std::cout << "string_view: tests passed\n";
}

/*
 * 【生命週期紅線】
 * 【陷阱】view 看起來像小 string，卻沒有 ownership；複製 view 只複製 pointer+length。
 * - `string_view v = string("tmp")`：完整運算式結束後 v 懸空。
 * - 回傳指向函式內 local string 的 view：函式返回即懸空。
 * - owner.append/reserve 造成重配後，舊 view 可能懸空。
 * - literal 建立的 view 指向 static storage，保存才安全。
 *
 * 【面試】string_view 參數一定更快嗎？避免複製且接受多來源，但若函式需 NUL 結尾或
 * 保存內容，仍應複製/使用 string。`data()` 不能假設 data()[size()] 是 NUL。
 * 【練習】寫 split_all 回傳 vector<string_view>，並在註解聲明 owner 契約。
 */

/*
 * 【教科書補充：借用範圍的完整契約】
 * - owner 被縮短、clear 或 erase 時，舊 view 可能指向已不屬於字串的區段；未重配不等於仍合法。
 * - remove_prefix/remove_suffix(n) 的前置條件是 n<=size()，違反時是未定義行為。
 * - vector<string_view> 只是很多借用；原始 string 集合搬移、重配或銷毀後，整批 view 都要重新取得。
 * - 禁止把 temporary string 傳入會保存/回傳 view 的介面；若無法表達借用關係就複製成 string。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'string_view.cpp' -o '/tmp/codex_cpp_C_String_string_view' && '/tmp/codex_cpp_C_String_string_view'
//
// === 預期輸出（節錄）===
// string_view: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
