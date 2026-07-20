/*
 * C++20 std::erase / std::erase_if：把 erase-remove 慣用法包成單一呼叫
 *
 * std::erase(string, value) 刪除所有等於 value 的字元；std::erase_if 依 predicate 刪除。
 * 回傳刪除數量。兩者皆線性 O(n)，會保持未刪元素的相對順序。它們是 <string>
 * 提供的非成員函式，不是 text.erase_if(...）。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "banana";
    const std::size_t removed = std::erase(text, 'a');
    assert(removed == 3U);
    assert(text == "bnn");
}

// LeetCode 27（Remove Element）的 string 對應版。
std::string leetcode_remove_character(std::string text, const char unwanted) {
    static_cast<void>(std::erase(text, unwanted));
    return text;
}

// 實務：log 單行協定拒絕 ASCII control chars，但保留 tab。
std::size_t practical_sanitize_log_line(std::string& text) {
    return std::erase_if(text, [](const char ch) {
        const unsigned char byte = static_cast<unsigned char>(ch);
        return byte < 0x20U && ch != '\t';
    });
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_remove_character("leetcode", 'e') == "ltcod");
    std::string log("ok\nnext\tfield", 13U);
    const std::size_t removed = practical_sanitize_log_line(log);
    assert(removed == 1U);
    assert(log == "oknext\tfield");
    std::cout << "std::erase: tests passed\n";
}

/*
 * 【面試】std::erase_if 會保留順序嗎？會，效果等同 erase(remove_if(...),end())。
 * 【陷阱】predicate 內把負的 char 直接交給 std::iscntrl 會 UB；先轉 unsigned char。
 * 【練習】刪除所有 ASCII whitespace，但保留一般 Unicode UTF-8 bytes 不動。
 */

/*
 * 【C++20 前後比較】
 * C++17 慣用法：
 *   s.erase(std::remove(s.begin(),s.end(),ch),s.end());
 * C++20：
 *   std::erase(s,ch);
 * 後者不會忘記第二步，也直接回傳刪除數量。
 *
 * 【predicate 規則】
 * - 不應修改元素或依賴呼叫次數/順序。
 * - 必須能接受所有 char 值；呼叫 cctype 前轉 unsigned char。
 * - erase_if 穩定保留未刪字元相對順序，但 iterator/reference 仍會失效。
 *
 * 【面試題】std::remove 是否真的縮短容器？不會，只把保留元素移到前面並回新 logical end。
 * 【練習】讓 sanitizer 回傳「刪除數 + 清理後內容」的結果 struct。
 * 【注意】刪除後任何保存的 element reference 都不應繼續使用。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'erase_free.cpp' -o '/tmp/codex_cpp_C_String_erase_free' && '/tmp/codex_cpp_C_String_erase_free'
//
// === 預期輸出（節錄）===
// std::erase: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
