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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses（有效括號）
// 題目：判斷三種括號是否依正確類型與順序配對；"()[]{}" 為 true，"(]" 為 false。
// 為何使用本章主題：front() 在非空 guard 後快速拒絕首字元為閉括號的輸入；主要配對仍以
//       string 的 back/push/pop 模擬 stack，front 是可省略但合法的提早判斷。
// 思路：1. 首字元為閉括號即失敗；2. 開括號入 stack；3. 閉括號取 stack 尾核對；4. 最後須為空。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 text 長度。
// 易錯點：空字串不能呼叫 front；原題保證只有括號，本版若出現其他字元會走閉括號分支而失敗。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔註解行辨識
// 情境：簡化設定格式規定只有第一個字元恰為 `#` 才是註解，空行或前有空格者不是。
// 為何使用本章主題：在 empty guard 後用 front() 直接讀首字元，比 substr(0,1) 無配置且意圖清楚。
// 設計：1. 先確認 line 非空；2. 讀取 front；3. 與 `#` 精確比較並回傳。
// 成本：時間 O(1)、額外空間 O(1)。
// 上線注意：若規格允許前導空白，必須先做 trim-left；多 byte BOM、Unicode 空白與 inline comment
//       也要另訂規則，不能在空字串上直接 front。
// -----------------------------------------------------------------------------
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
