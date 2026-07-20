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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack（最小棧）
// 題目：push、pop、top、getMin 都在 O(1) 完成；例如 push -2,0,-3，最小為 -3，pop 後為 -2。
// 為何使用本章主題：GenericMinStack<T> 將相同雙 stack 技巧套到可比較型別；題目實際 instantiate int。
// 思路：每次 push 同步保存 value 與截至該層的 minimum；pop 同時移除；兩種查詢讀各自 back。
// 複雜度：每個操作 O(1)，額外空間 O(N)，N 為元素數。
// 易錯點：空 stack 不可 pop/top/getMin；T 必須可比較與複製；第二次 push 若拋例外需回滾第一個 vector 才能守 invariant。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】固定容量的工作環狀佇列
// 情境：最多同時排兩個工作，滿載時拒絕 deploy；pop 後要能在環狀槽位再次加入並維持 FIFO。
// 為何使用本章主題：T/Capacity 模板讓容量進入型別，std::array 將槽位內嵌；元素 T 自身仍可能另行配置資源。
// 設計：head_ 指 front、size_ 計使用量；push 寫 (head+size)%Capacity；pop 推進 head；滿/空明確回報。
// 成本：push/front/pop O(1)，inline 槽位空間 O(Capacity*sizeof(T))。
// 上線注意：Capacity 編譯期必須大於零；T 需可預設建構/指定；多 producer/consumer 要同步與明確關閉協定。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '24_ClassTemplate.cpp' -o '/tmp/codex_cpp_C_OOP_24_ClassTemplate' && '/tmp/codex_cpp_C_OOP_24_ClassTemplate'
//
// === 預期輸出（節錄）===
// [實務] fixed-capacity job queue 不做 heap allocation
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
