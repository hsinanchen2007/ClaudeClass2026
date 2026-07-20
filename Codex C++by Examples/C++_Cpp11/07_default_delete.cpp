/*
 * C++11 教科書：= default 與 = delete
 *
 * = default 明確要求 compiler 產生特殊成員函式；= delete 宣告某呼叫不合法，錯誤會在
 * compile time 發生。常見用途：禁止複製、禁止危險隱式轉換、保留 move-only 語意。
 *
 * 【特殊成員】destructor、copy/move constructor、copy/move assignment 彼此會影響隱式生成。
 * 最佳預設是 Rule of Zero：資源交給 vector/string/unique_ptr，類別不手寫五個函式。
 * 若真的管理 raw resource，遵守 Rule of Five 並明確定義 ownership。
 * 【陷阱】只宣告 destructor 可能抑制隱式 move；只 delete copy 不代表一定自動有 move。
 * 【面試題】為何 delete private copy constructor 優於舊式「只宣告不定義」？診斷更早更清楚。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stack>
#include <string>
#include <type_traits>
#include <utility>

namespace basic {
class Connection {
public:
    explicit Connection(int id) : id_(id) {}
    ~Connection() = default;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) noexcept = default;
    Connection& operator=(Connection&&) noexcept = default;

    int id() const noexcept { return id_; }

private:
    int id_;
};

void demo() {
    static_assert(!std::is_copy_constructible<Connection>::value,
                  "Connection 必須禁止複製");
    static_assert(std::is_move_constructible<Connection>::value,
                  "Connection 必須允許移動");
    Connection first(9);
    Connection second(std::move(first));
    assert(second.id() == 9);
}
}  // namespace basic

namespace leetcode {
// LeetCode 155：Min Stack。兩個 stack 讓 push/pop/top/getMin 都是 O(1)。
// 類別不管理 raw resource，因此特殊成員可全部交給 compiler（Rule of Zero）。
class MinStack {
public:
    void push(int value) {
        values_.push(value);
        minima_.push(minima_.empty() ? value : std::min(value, minima_.top()));
    }
    void pop() {
        values_.pop();
        minima_.pop();
    }
    int top() const { return values_.top(); }
    int get_min() const { return minima_.top(); }

private:
    std::stack<int> values_;
    std::stack<int> minima_;
};

void test() {
    MinStack stack;
    stack.push(-2);
    stack.push(0);
    stack.push(-3);
    assert(stack.get_min() == -3);
    stack.pop();
    assert(stack.top() == 0 && stack.get_min() == -2);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
// 實務：AuditToken 不允許從 bool/char 等型別隱式建立，避免 ID 混淆。
class AuditToken {
public:
    explicit AuditToken(std::string value) : value_(std::move(value)) {}
    AuditToken(int) = delete;
    const std::string& value() const noexcept { return value_; }

private:
    std::string value_;
};

void test() {
    const AuditToken token("request-42");
    assert(token.value() == "request-42");
    static_assert(!std::is_constructible<AuditToken, int>::value,
                  "AuditToken 必須拒絕 int");
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "default/delete：move-only、MinStack、型別防誤用測試通過\n";
}
