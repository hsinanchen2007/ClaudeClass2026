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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack（可常數時間取最小值的堆疊）
// 題目：實作 push、pop、top 與 getMin，所有操作皆須 O(1)；例如壓入 -2,0,-3 後最小值是 -3。
// 為何使用本章主題：兩個 std::stack 已管理資源，MinStack 採 Rule of Zero，不必手寫或 delete/default 特殊成員。
// 思路：1. values_ 保存原值；2. minima_ 每層保存截至該層的最小值；3. push/pop 同步操作兩個 stack。
// 複雜度：每個操作時間 O(1)；N 個元素需要 O(N) 額外空間存 minima_。
// 易錯點：兩個 stack 必須同步；題目保證非空才 pop/top/getMin，實務 API 應自行檢查空堆疊。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】稽核請求權杖強型別
// 情境：稽核 API 只接受字串 request token，不能讓整數 ID 因隱式轉換混入相同入口。
// 為何使用本章主題：將 AuditToken(int) 標為 = delete，讓錯誤型別在 overload resolution 階段得到明確診斷。
// 設計：1. 只提供 explicit string constructor；2. 刪除 int constructor；3. 以 const reference 暴露已持有字串。
// 成本：建構成本 O(L)，L 為 token 長度；讀取 value 為 O(1)，物件持有 O(L) 空間。
// 上線注意：= delete 只擋 int 路徑，仍要驗 token 格式、長度與敏感資訊記錄政策；回傳 reference 不得活過 token。
// -----------------------------------------------------------------------------
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

// 【延伸練習】建立只可 move 的 FileHandle，並以 type traits 驗證 copy 被刪除、move 可用。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_default_delete.cpp' -o '/tmp/codex_cpp_C_Cpp11_07_default_delete' && '/tmp/codex_cpp_C_Cpp11_07_default_delete'
//
// === 預期輸出（節錄）===
// default/delete：move-only、MinStack、型別防誤用測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
