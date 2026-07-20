// ============================================================================
// 課題 24：Class template（類別模板）
// ============================================================================
//
// `template<class T>` 讓一份 class definition 依型別產生不同 specialization，例如
// Stack<int> 與 Stack<string> 是不同型別。template 通常需把完整定義放 header，因為
// compiler 在使用點才知道要 instantiate 哪個 T。
//
// template parameter 也可為 value（`std::size_t Capacity`）或另一 template。操作只有在
// 被 instantiate/使用時才需合法；C++20 concepts 可把需求提早寫成清楚 constraint。
//
// 【面試】template 不是 runtime polymorphism；它在 compile time 產生型別/程式碼，
// 通常可 inline，但過多 instantiations 可能增加 binary size。
// 【陷阱】dependent name 可能需要 typename/template 關鍵字，會在 template 章深入。
// ============================================================================

#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

// 與 assert 不同，expect 在 -DNDEBUG 建置中仍會評估條件。
void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

template<class T>
class Stack {
public:
    void push(T value) { values_.push_back(std::move(value)); }
    void pop()
    {
        if (values_.empty()) throw std::underflow_error("empty stack");
        values_.pop_back();
    }
    const T& top() const
    {
        if (values_.empty()) throw std::underflow_error("empty stack");
        return values_.back();
    }
    bool empty() const { return values_.empty(); }

private:
    std::vector<T> values_;
};

void basic_example()
{
    Stack<int> numbers;
    numbers.push(10);
    numbers.push(20);
    expect(numbers.top() == 20, "Stack<int> 頂端元素錯誤");
    Stack<std::string> words;
    words.push("CUDA");
    expect(words.top() == "CUDA", "Stack<string> 頂端元素錯誤");
    std::cout << "[基礎] 同一模板產生 Stack<int> 與 Stack<string>\n";
}

// LeetCode 155：Min Stack。
// GenericMinStack<T> 可處理任何有 operator< 的 value type；題目以 int instance 執行。
template<class T>
class GenericMinStack {
public:
    void push(T value)
    {
        const T minimum = minimums_.empty() || value < minimums_.back()
            ? value : minimums_.back();
        values_.push_back(std::move(value));
        minimums_.push_back(minimum);
    }
    void pop()
    {
        values_.pop_back();
        minimums_.pop_back();
    }
    const T& top() const { return values_.back(); }
    const T& getMin() const { return minimums_.back(); }

private:
    std::vector<T> values_;
    std::vector<T> minimums_;
};

void leetcode_155_example()
{
    GenericMinStack<int> stack;
    stack.push(-2);
    stack.push(0);
    stack.push(-3);
    expect(stack.getMin() == -3, "最小值應為 -3");
    stack.pop();
    expect(stack.top() == 0 && stack.getMin() == -2,
           "pop 後的頂端或最小值錯誤");
    std::cout << "[LeetCode 155] GenericMinStack<int> 通過\n";
}

// 實務案例：Capacity 是 compile-time value；不同容量是不同型別且不需 heap allocation。
template<class T, std::size_t Capacity>
class BoundedQueue {
public:
    bool push(T value)
    {
        if (size_ == Capacity) return false;
        data_.at((head_ + size_) % Capacity) = std::move(value);
        ++size_;
        return true;
    }
    const T& front() const
    {
        if (size_ == 0U) throw std::underflow_error("empty queue");
        return data_.at(head_);
    }
    void pop()
    {
        if (size_ == 0U) throw std::underflow_error("empty queue");
        head_ = (head_ + 1U) % Capacity;
        --size_;
    }
    std::size_t size() const { return size_; }

private:
    static_assert(Capacity > 0U, "queue capacity must be positive");
    std::array<T, Capacity> data_{};
    std::size_t head_ = 0U;
    std::size_t size_ = 0U;
};

void practical_example()
{
    BoundedQueue<std::string, 2U> queue;

    // 【契約】push 是必要副作用：成功回 true，滿載時回 false 且內容不變。
    const bool compile_queued = queue.push("compile");
    const bool test_queued = queue.push("test");
    const bool deploy_queued_while_full = queue.push("deploy");
    expect(compile_queued && test_queued, "容量內的工作應成功入列");
    expect(!deploy_queued_while_full, "滿載 queue 必須拒絕新工作");
    expect(queue.front() == "compile" && queue.size() == 2U,
           "滿載 queue 的內容或大小錯誤");

    queue.pop();
    expect(queue.front() == "test", "pop 後應輪到第二個工作");

    const bool deploy_queued_after_pop = queue.push("deploy");
    expect(deploy_queued_after_pop, "釋出空間後應可再次入列");
    queue.pop();
    expect(queue.front() == "deploy", "環狀索引繞回後順序錯誤");
    queue.pop();
    expect(queue.size() == 0U, "移除全部工作後 queue 應為空");

    // 【契約】front/pop 對空 queue 會在所有建置模式丟 underflow_error。
    bool front_rejected = false;
    try {
        static_cast<void>(queue.front());
    } catch (const std::underflow_error&) {
        front_rejected = true;
    }
    expect(front_rejected, "空 queue 的 front 必須被拒絕");

    bool pop_rejected = false;
    try {
        queue.pop();
    } catch (const std::underflow_error&) {
        pop_rejected = true;
    }
    expect(pop_rejected, "空 queue 的 pop 必須被拒絕");
    std::cout << "[實務] fixed-capacity job queue 不做 heap allocation\n";
}

int main()
{
    basic_example();
    leetcode_155_example();
    practical_example();
}

// 練習：用 concept 限制 GenericMinStack 的 T 必須可用 `<` 比較。
// 複雜度：template 不決定 Big-O；本例 stack push/pop 依底層 vector，MinStack 操作 O(1)。
// 生命週期：每個 specialization 是獨立型別；其 objects 仍依 members 的 ownership 規則解構。
