// ============================================================================
// 課題 7：Member initializer list（成員初始化列表）
// ============================================================================
//
// `Widget(...) : id_(id), name_(...) {}` 是「直接初始化 member」；若改在 constructor
// body 裡 assignment，member 會先 default construct，再被賦值，多做一步，而且 const、
// reference、沒有 default constructor 的 member 根本不能這樣處理。
//
// 真實初始化順序固定為：virtual base → direct base → member 宣告順序 → body。
// initializer list 的書寫順序不會改變它。讓列表與宣告順序相同，可避免 `-Wreorder`
// 與「用尚未初始化的 member 初始化另一 member」的 bug。
//
// `{}` 也叫 initializer list 的語法家族，但 `std::initializer_list<T>` 是另一個具體
// 型別；本檔主題是 constructor 冒號後的 member initialization。
//
// 【面試】reference member 必須在 initializer list 綁定，之後不能改綁其他物件。
// 【陷阱】reference member 可能活得比被參照物件久而 dangling。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Employee {
public:
    Employee(int id, std::string name, const std::string& department)
        : id_(id), name_(std::move(name)), department_(department) {}

    std::string label() const
    {
        return department_ + "/" + std::to_string(id_) + ":" + name_;
    }

private:
    const int id_;                  // const member 只能初始化，不能稍後 assignment。
    std::string name_;
    const std::string& department_; // non-owning reference；生命週期由呼叫端保證。
};

void basic_example()
{
    const std::string team = "compiler";
    const Employee employee(7, "Ada", team);
    assert(employee.label() == "compiler/7:Ada");
    std::cout << "[基礎] " << employee.label() << '\n';
}

// LeetCode 622：Design Circular Queue。
// initializer list 一次建立固定容量 vector 與所有 index/count invariant。
class MyCircularQueue {
public:
    explicit MyCircularQueue(int capacity)
        : data_(validated_capacity(capacity)), head_(0U), count_(0U) {}

    bool en_queue(int value)
    {
        if (is_full()) {
            return false;
        }
        const std::size_t tail = (head_ + count_) % data_.size();
        data_.at(tail) = value;
        ++count_;
        return true;
    }

    bool de_queue()
    {
        if (is_empty()) {
            return false;
        }
        head_ = (head_ + 1U) % data_.size();
        --count_;
        return true;
    }

    int front() const { return is_empty() ? -1 : data_.at(head_); }
    int rear() const
    {
        return is_empty() ? -1 : data_.at((head_ + count_ - 1U) % data_.size());
    }
    bool is_empty() const { return count_ == 0U; }
    bool is_full() const { return count_ == data_.size(); }

private:
    static std::size_t validated_capacity(int capacity)
    {
        if (capacity <= 0) {
            throw std::invalid_argument("capacity must be positive");
        }
        return static_cast<std::size_t>(capacity);
    }

    std::vector<int> data_;
    std::size_t head_;
    std::size_t count_;
};

void leetcode_622_example()
{
    MyCircularQueue queue(3);
    const bool queued_1 = queue.en_queue(1);
    const bool queued_2 = queue.en_queue(2);
    const bool queued_3 = queue.en_queue(3);
    const bool full = !queue.en_queue(4);
    assert(queued_1);
    assert(queued_2);
    assert(queued_3);
    assert(full);
    assert(queue.rear() == 3 && queue.is_full());
    const bool removed_front = queue.de_queue();
    const bool wrapped_insert = queue.en_queue(4);
    assert(removed_front && wrapped_insert);
    assert(queue.rear() == 4);
    std::cout << "[LeetCode 622] circular queue front=" << queue.front()
              << " rear=" << queue.rear() << '\n';
}

// 實務案例：Job 建構時就取得不可變 id，payload 則 move 進 member，避免多一次 copy。
class Job {
public:
    Job(unsigned long id, std::vector<int> payload)
        : id_(id), payload_(std::move(payload)) {}

    unsigned long id() const { return id_; }
    std::size_t payload_size() const { return payload_.size(); }

private:
    const unsigned long id_;
    std::vector<int> payload_;
};

void practical_example()
{
    Job job(42UL, {10, 20, 30});
    assert(job.id() == 42UL && job.payload_size() == 3U);
    std::cout << "[實務] job 42 payload size=3\n";
}

int main()
{
    basic_example();
    leetcode_622_example();
    practical_example();
}

// 練習：故意調換 member 宣告與 initializer 順序，以 -Wreorder 觀察 compiler 提醒。
// 複雜度：initializer list 可直接建構 member，避免「先 default 再 assignment」的額外成本。
// 生命週期：初始化永遠依 member 宣告順序，不依 list 書寫順序；reference member 的來源須更長壽。
