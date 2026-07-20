// ============================================================================
// 課題 14：Inheritance（繼承）與 is-a 關係
// ============================================================================
//
// `class Derived : public Base` 表示 Derived 是一種 Base，會先建構 Base subobject，再
// 建構 Derived members；解構順序相反。Derived 可重用 Base 的 public/protected API，
// 但 Base private state 仍只能透過 Base operations 維護。
//
// 繼承最重要的是可替換語意（Liskov Substitution）：任何接受 Base 的地方若換成
// Derived，不應破壞原契約。單純「想重用幾行 code」通常優先 composition；繼承會
// 建立較強耦合，也可能引入 slicing、virtual dispatch 與 fragile base class 問題。
//
// 【面試】Base constructor 一定早於 Derived constructor；Derived initializer list 要用
// `Base(args...)` 選擇 base constructor。
// 【陷阱】按值接收 Base 會把 Derived-specific 部分切掉，稱 object slicing。
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
    Employee(int id, std::string name) : id_(id), name_(std::move(name)) {}
    int id() const { return id_; }
    const std::string& name() const { return name_; }

private:
    int id_;
    std::string name_;
};

class Engineer : public Employee {
public:
    Engineer(int id, std::string name, std::string language)
        : Employee(id, std::move(name)), language_(std::move(language)) {}
    const std::string& language() const { return language_; }

private:
    std::string language_;
};

void basic_example()
{
    const Engineer engineer(7, "Ada", "C++");
    const Employee& employee_view = engineer; // reference 不會 slicing。
    assert(employee_view.name() == "Ada" && engineer.language() == "C++");
    std::cout << "[基礎] Engineer is-an Employee: " << employee_view.name() << '\n';
}

// LeetCode 303：Range Sum Query - Immutable。
// PrefixSum1D 是可重用的 base abstraction；NumArray 保留題目要求的名稱與 sumRange API。
class PrefixSum1D {
protected:
    explicit PrefixSum1D(const std::vector<int>& values) : prefix_(values.size() + 1U, 0)
    {
        for (std::size_t index = 0U; index < values.size(); ++index) {
            prefix_.at(index + 1U) = prefix_.at(index) + values.at(index);
        }
    }

    int range_sum(std::size_t left, std::size_t right) const
    {
        return prefix_.at(right + 1U) - prefix_.at(left);
    }

private:
    std::vector<int> prefix_;
};

class NumArray : public PrefixSum1D {
public:
    explicit NumArray(const std::vector<int>& values) : PrefixSum1D(values) {}
    int sumRange(int left, int right) const
    {
        if (left < 0 || right < left) {
            throw std::out_of_range("invalid range");
        }
        return range_sum(static_cast<std::size_t>(left), static_cast<std::size_t>(right));
    }
};

void leetcode_303_example()
{
    const NumArray sums({-2, 0, 3, -5, 2, -1});
    assert(sums.sumRange(0, 2) == 1);
    assert(sums.sumRange(2, 5) == -1);
    std::cout << "[LeetCode 303] inherited prefix-sum implementation 正確\n";
}

// 實務案例：所有事件都有 immutable metadata，ErrorEvent 再增加 severity。
class Event {
public:
    Event(unsigned long sequence, std::string message)
        : sequence_(sequence), message_(std::move(message)) {}
    unsigned long sequence() const { return sequence_; }
    const std::string& message() const { return message_; }

private:
    unsigned long sequence_;
    std::string message_;
};

class ErrorEvent : public Event {
public:
    ErrorEvent(unsigned long sequence, std::string message, int severity)
        : Event(sequence, std::move(message)), severity_(severity) {}
    int severity() const { return severity_; }

private:
    int severity_;
};

void practical_example()
{
    const ErrorEvent event(42UL, "GPU unavailable", 3);
    assert(event.sequence() == 42UL && event.severity() == 3);
    std::cout << "[實務] event #42 severity=3 message=" << event.message() << '\n';
}

int main()
{
    basic_example();
    leetcode_303_example();
    practical_example();
}

// 練習：嘗試寫 `Employee sliced = engineer;`，列出會遺失哪些 state。
// 複雜度：繼承本身無演算法 Big-O；virtual dispatch 常是常數開銷但標準不保證實作方式。
// 生命週期：derived 建構先 base、解構先 derived；base reference 只是 alias，不延長 derived 存活。
