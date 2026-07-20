// ============================================================================
// stack：LIFO 容器轉接器
// ============================================================================
// std::stack<T, Container> 不自行規定儲存結構，而是把底層容器限制成 top/push/pop
// 的後進先出介面。預設底層是 deque；也可用 vector/list，只要提供 back、push_back、
// pop_back。它沒有 iterator，刻意避免呼叫端繞過 LIFO 契約。
// top/push/pop 通常 O(1)。pop() 不回傳值：先讀/move top，再 pop，可維持介面與
// 例外安全；空 stack 呼叫 top/pop 是 UB。

#include <cassert>
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

void basic_demo()
{
    std::stack<int> history;
    history.push(10);
    history.emplace(20);
    assert(history.top() == 20);
    history.pop();
    assert(history.top() == 10 && history.size() == 1U);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses（有效的括號）
// 題目：字串只含 ()[]{}，判斷括號型別與巢狀次序是否正確；例如 "{[()]}" 為 true。
// 為何使用本章主題：stack 的 LIFO 次序恰好保存尚未閉合的最近一個左括號。
// 思路：左括號 push；右括號查對應左括號；驗證 top 後 pop；結束時 stack 必須為空。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為字元數。
// 易錯點：讀 top 前先檢查 empty；錯誤字元在此也判 false；掃完仍有左括號不能算有效。
// -----------------------------------------------------------------------------
bool valid_parentheses(const std::string& text)
{
    const std::unordered_map<char, char> opening_for{
        {')', '('}, {']', '['}, {'}', '{'}};
    std::stack<char> openings;

    for (const char token : text) {
        if (token == '(' || token == '[' || token == '{') {
            openings.push(token);
            continue;
        }
        const auto expected = opening_for.find(token);
        if (expected == opening_for.end() || openings.empty() ||
            openings.top() != expected->second) {
            return false;
        }
        openings.pop();
    }
    return openings.empty();
}

void leetcode_demo()
{
    assert(valid_parentheses("()[]{}"));
    assert(valid_parentheses("{[()]}"));
    assert(!valid_parentheses("([)]"));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】文字編輯器復原歷史
// 情境：文件每次 replace 後，使用者可依修改的反向次序逐步回到先前文字。
// 為何使用本章主題：Undo 天然是後做先復原的 LIFO；stack 限制介面可避免任意改動歷史中段。
// 設計：replace 前 push 舊文字；undo 時先 move top 回目前內容；再 pop；無歷史則回 false。
// 成本：每次替換/復原需 O(L) 搬移或複製文字，歷史空間 O(H*L)，H 為步數、L 為平均快照長度。
// 上線注意：需設定記憶體上限並考慮 delta/command；多執行緒編輯需同步，復原指令也要定義 redo 與例外策略。
// -----------------------------------------------------------------------------
class UndoableText {
public:
    explicit UndoableText(std::string text) : text_(std::move(text)) {}

    void replace(std::string next)
    {
        undo_.push(text_);
        text_ = std::move(next);
    }

    bool undo()
    {
        if (undo_.empty()) {
            return false;
        }
        text_ = std::move(undo_.top());
        undo_.pop();
        return true;
    }

    const std::string& value() const { return text_; }

private:
    std::string text_;
    std::stack<std::string, std::vector<std::string>> undo_;
};

void practical_demo()
{
    UndoableText document("draft");
    document.replace("reviewed");
    document.replace("published");
    const bool first_undo = document.undo();
    assert(first_undo && document.value() == "reviewed");
    const bool second_undo = document.undo();
    assert(second_undo && document.value() == "draft");
    const bool history_empty = !document.undo();
    assert(history_empty);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "stack：LIFO、括號驗證與 Undo 測試通過\n";
}

// 【陷阱】while (!stack.empty()) 才可讀 top；pop 後舊 top reference 失效。
// 【面試】DFS/遞迴、expression evaluation、monotonic stack 分別為何需要 stack？
// 【練習】實作 LeetCode 150 Evaluate Reverse Polish Notation。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'stack.cpp' -o '/tmp/codex_cpp_C_Container_stack' && '/tmp/codex_cpp_C_Container_stack'
//
// === 預期輸出（節錄）===
// stack：LIFO、括號驗證與 Undo 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
