/*
 * front()：取得第一個字元
 *
 * 非空字串的 front() 等價於 [0]，為 O(1)，可讀也可寫。空字串呼叫 front() 是
 * 未定義行為，因此「先 empty()」不是可選風格，而是必要前置條件。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string token = "json";
    assert(token.front() == 'j');
    token.front() = 'J';
    assert(token == "Json");
}

// LeetCode 20（Valid Parentheses）：以 string 當 stack，top 對應 back；front 用於
// 快速拒絕第一個字元就是 closing bracket 的情況。
bool leetcode_valid_parentheses(const std::string& text) {
    if (!text.empty() && (text.front() == ')' || text.front() == ']' || text.front() == '}')) {
        return false;
    }
    std::string stack;
    for (const char ch : text) {
        if (ch == '(' || ch == '[' || ch == '{') {
            stack.push_back(ch);
        } else {
            if (stack.empty()) {
                return false;
            }
            const char open = stack.back();
            stack.pop_back();
            if ((ch == ')' && open != '(') || (ch == ']' && open != '[') ||
                (ch == '}' && open != '{')) {
                return false;
            }
        }
    }
    return stack.empty();
}

// 實務：設定檔以 # 開頭表示註解；空行不可呼叫 front。
bool practical_is_comment_line(const std::string& line) {
    return !line.empty() && line.front() == '#';
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_valid_parentheses("()[]{}"));
    assert(!leetcode_valid_parentheses("(]"));
    assert(!leetcode_valid_parentheses("]"));
    assert(practical_is_comment_line("# timeout=10"));
    assert(!practical_is_comment_line(""));
    std::cout << "front: tests passed\n";
}

/*
 * 【面試】front 回傳副本嗎？不是；非 const string 回傳 char&，修改它會改原字串。
 * 【陷阱】只判 `line.front() == '#'` 會讓空行觸發 UB。
 * 【練習】寫 trim_left_view，再以結果的 front 判斷前面有空白的註解行。
 */

/*
 * 【面試速查】front 是 O(1) reference access；空字串呼叫 UB，不會丟 out_of_range。
 * 與 at(0) 相比，front 語意更清楚但沒有檢查。若先 trim 成 string_view，也必須先
 * 確認 view 非空才 front。修改 string 的 front 不改 size/capacity。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'front.cpp' -o '/tmp/codex_cpp_C_String_front' && '/tmp/codex_cpp_C_String_front'
//
// === 預期輸出（節錄）===
// front: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
