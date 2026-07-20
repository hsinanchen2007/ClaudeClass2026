// ============================================================================
// 課題 5：noexcept contract、move optimization 與 terminate
// ============================================================================
//
// `noexcept` 是可由 type system/optimizer 查詢的承諾：若 exception 逃出 noexcept function，
// runtime 直接 std::terminate，外層 catch 接不到。只有真的能保證不丟才標；destructor、
// swap、move 常應 noexcept（其 members 也需支持）。
//
// vector reallocation 為 strong guarantee，若 T move 可能丟且 T 可 copy，通常改 copy 舊
// elements；noexcept move 讓 container 安全使用較便宜的 move。conditional noexcept 可用
// `noexcept(expression)` 或 type traits 傳遞 member 能力。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

class Buffer {
public:
    explicit Buffer(std::size_t size) : data_(size, 0) {}
    Buffer(const Buffer&) = default;
    Buffer& operator=(const Buffer&) = default;
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;
    void swap(Buffer& other) noexcept { data_.swap(other.data_); }
    std::size_t size() const noexcept { return data_.size(); }
private:
    std::vector<int> data_;
};

void basic_example()
{
    static_assert(std::is_nothrow_move_constructible_v<Buffer>);
    static_assert(noexcept(std::declval<Buffer&>().swap(std::declval<Buffer&>())));
    Buffer first(10U);
    Buffer second(20U);
    first.swap(second);
    assert(first.size() == 20U && second.size() == 10U);
    std::cout << "[基礎] Buffer move/swap/size have checked noexcept contracts\n";
}

// LeetCode 155：MinStack。查詢在題目保證 non-empty 時可做 O(1)，但 vector::push_back
// 可能配置失敗，不能標 noexcept；pop/top 若自行呼叫在空 stack 是前置條件違反。
class MinStack {
public:
    void push(int value)
    {
        values_.push_back(value);
        minimums_.push_back(minimums_.empty() ? value : std::min(value, minimums_.back()));
    }
    void pop() noexcept { values_.pop_back(); minimums_.pop_back(); }
    int top() const noexcept { return values_.back(); }
    int getMin() const noexcept { return minimums_.back(); }
private:
    std::vector<int> values_;
    std::vector<int> minimums_;
};

void leetcode_155_example()
{
    MinStack stack;
    stack.push(-2); stack.push(0); stack.push(-3);
    assert(stack.getMin() == -3);
    stack.pop();
    assert(stack.top() == 0 && stack.getMin() == -2);
    std::cout << "[LeetCode 155] only truly nonthrowing/preconditioned ops marked noexcept\n";
}

// 實務：free function swap 轉送 member noexcept，讓 standard algorithms 使用。
void swap(Buffer& left, Buffer& right) noexcept { left.swap(right); }

void practical_example()
{
    Buffer small(1U), large(100U);
    using std::swap;
    swap(small, large);
    assert(small.size() == 100U);
    std::cout << "[實務] ADL swap preserves noexcept\n";
}

int main()
{
    basic_example();
    leetcode_155_example();
    practical_example();
}

// 易錯與面試：noexcept 是契約，不是「請幫我忽略 exception」；若函式仍逸出 exception，
// 程式直接 std::terminate。move constructor 的 conditional noexcept 會影響 vector 選 move/copy。
// 練習：移除 move noexcept，以 type trait 觀察 vector 可能選 copy 的理由。
// 複雜度：noexcept 是 compile-time 契約/查詢，本身 O(1)；vector relocation 的總成本仍 O(N)。
// 生命週期：noexcept function 若讓 exception 逸出會 terminate，stack unwinding 不保證完成到 caller。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_noexcept.cpp' -o '/tmp/codex_cpp_C_Exception_05_noexcept' && '/tmp/codex_cpp_C_Exception_05_noexcept'
//
// === 預期輸出（節錄）===
// [實務] ADL swap preserves noexcept
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
