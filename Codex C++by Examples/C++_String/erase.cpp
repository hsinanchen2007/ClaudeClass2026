/*
 * std::string::erase：依索引或 iterator 刪除一段內容
 *
 * erase(pos,count) 刪 [pos,pos+count)；count 超過尾端會自動截到尾端，pos 超界則
 * 丟 out_of_range。iterator 版回傳「刪除區間之後的新 iterator」，適合安全迴圈。
 * 中間刪除需搬移後段，最壞 O(size)。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "012345";
    text.erase(2U, 2U);
    assert(text == "0145");
    text.erase(text.begin());
    assert(text == "145");
}

// LeetCode 1910（Remove All Occurrences of a Substring）。
std::string leetcode_remove_occurrences(std::string text, const std::string& part) {
    if (part.empty()) return text;
    std::size_t position = text.find(part);
    while (position != std::string::npos) {
        text.erase(position, part.size());
        position = text.find(part);
    }
    return text;
}

// 實務：從 query string 移除 token 欄位；簡化假設每欄以 & 分隔。
std::string practical_remove_token_field(std::string query) {
    const std::string key = "token=";
    const std::size_t begin = query.find(key);
    if (begin == std::string::npos) return query;
    std::size_t end = query.find('&', begin);
    if (end == std::string::npos) end = query.size();
    else ++end;  // 一併刪右側 &。
    query.erase(begin, end - begin);
    if (!query.empty() && query.back() == '&') query.pop_back();
    return query;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_remove_occurrences("daabcbaabcbc", "abc") == "dab");
    assert(leetcode_remove_occurrences("axxxxyyyyb", "xy") == "ab");
    assert(practical_remove_token_field("user=ada&token=secret&mode=1") == "user=ada&mode=1");
    assert(practical_remove_token_field("token=secret") == "");
    std::cout << "erase: tests passed\n";
}

/*
 * 【陷阱】range-for 中直接 erase 會讓迭代器失效；改用回傳的新 iterator。
 * 【面試】erase-remove idiom 為何有兩步？algorithm remove 只分區，container erase 才縮 size。
 * 【練習】讓 practical_remove_token_field 正確處理 token 在最後且前方有 & 的情況。
 */

/*
 * 【erase overload 速查】
 * - erase()：全部清空，類似 clear。
 * - erase(pos,count)：索引區段，count 可超過尾端。
 * - erase(iterator)：回下一個有效 iterator。
 * - erase(first,last)：刪半開區間並回新 iterator。
 * 刪中段需要左移尾端，通常 O(n)。刪除迴圈必須使用回傳 iterator；索引迴圈則要
 * 決定刪後是否遞增，否則會跳過相鄰匹配。
 * 【面試題】刪完會自動縮 capacity 嗎？不保證；size 與 capacity 分離。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'erase.cpp' -o '/tmp/codex_cpp_C_String_erase' && '/tmp/codex_cpp_C_String_erase'
//
// === 預期輸出（節錄）===
// erase: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
