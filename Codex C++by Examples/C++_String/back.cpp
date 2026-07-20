/*
 * back()：取得最後一個字元
 *
 * 非空字串的 back() 等價於 [size()-1]，為 O(1)，常與 push_back/pop_back 組成
 * stack。空字串呼叫 back() 是未定義行為；pop_back() 也有同樣前置條件。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string path = "/tmp/";
    assert(path.back() == '/');
    path.back() = ':';
    assert(path == "/tmp:");
}

// LeetCode 844（Backspace String Compare）：建構最終文字的直觀解法。
std::string apply_backspaces(const std::string& input) {
    std::string output;
    for (const char ch : input) {
        if (ch == '#') {
            if (!output.empty()) {
                output.pop_back();
            }
        } else {
            output.push_back(ch);
        }
    }
    return output;
}

bool leetcode_backspace_compare(const std::string& first, const std::string& second) {
    return apply_backspaces(first) == apply_backspaces(second);
}

// 實務：路徑 join 的簡化版；真實跨平台程式優先用 std::filesystem::path。
std::string practical_join_unix_path(std::string base, const std::string& child) {
    if (!base.empty() && base.back() != '/') {
        base.push_back('/');
    }
    base += child;
    return base;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_backspace_compare("ab#c", "ad#c"));
    assert(!leetcode_backspace_compare("a#c", "b"));
    assert(practical_join_unix_path("/srv/data", "report.txt") == "/srv/data/report.txt");
    assert(practical_join_unix_path("/srv/data/", "report.txt") == "/srv/data/report.txt");
    std::cout << "back: tests passed\n";
}

/*
 * 【面試】為何用 string 當 stack 很方便？連續儲存、back/push_back/pop_back 均簡單；
 * 但只適用元素就是 char 的情境。
 * 【練習】practical_join_unix_path 應如何處理 child 以 '/' 開頭？先定義規格再加測試。
 */

/*
 * 【考前速查】
 * - back() 回 reference，不是副本；const string 則回 const reference。
 * - 非空前提同時適用 back 與 pop_back，兩者均為 O(1)。
 * - 若要移除並取值：先複製 `char ch=s.back()`，再 `s.pop_back()`。
 * - append/resize 後舊的 back reference 可能因重配失效。
 * 【陷阱】即使上一行剛判非空，若中間呼叫可能 clear 字串，也要重新確認前置條件。
 * 【面試題】為何不寫 `s[s.size()-1]`？back 意圖清楚，但仍必須先判空。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'back.cpp' -o '/tmp/codex_cpp_C_String_back' && '/tmp/codex_cpp_C_String_back'
//
// === 預期輸出（節錄）===
// back: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
