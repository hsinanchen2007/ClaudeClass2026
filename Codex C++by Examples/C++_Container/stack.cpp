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

// ----------------------------------------------------------------------------
// LeetCode 20：Valid Parentheses
// ----------------------------------------------------------------------------
// 左括號入 stack；右括號必須與 top 配對。每字元進出至多一次，O(n) 時間/O(n) 空間。
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

// ----------------------------------------------------------------------------
// 實務：文字編輯器 Undo
// ----------------------------------------------------------------------------
// 每次修改前保存舊值。真實系統可保存 command/delta 以降低複製成本，並設容量上限。
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
