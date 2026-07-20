// ============================================================================
// 課題 18：Virtual destructor（虛擬解構子）
// ============================================================================
//
// 若 object 可能經 Base pointer 被 delete，Base destructor 必須 virtual；否則刪除
// derived object 是 undefined behavior，derived resources 可能不釋放。介面慣例：
// `virtual ~Interface() = default;`。
//
// 一旦 class 有任何 virtual function，它通常已是 polymorphic base，virtual destructor
// 的額外成本通常只是 vtable entry，不會再讓每 object 多一個 vptr。若 class 明確不允許
// polymorphic deletion，可讓 destructor protected non-virtual，阻止外界 delete Base*。
//
// 【面試】derived destructor 不必再寫 virtual；virtual 性質會沿繼承保留，可加 override。
// 【安全】本檔不以「真的執行 UB」教學，只展示正確寫法並用 counter 驗證 derived cleanup。
// ============================================================================

#include <cassert>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

class Worker {
public:
    virtual ~Worker() = default;
    virtual int run() const = 0;
};

class BufferWorker final : public Worker {
public:
    explicit BufferWorker(int& destroyed) : destroyed_(destroyed), buffer_(1'024, 7) {}
    ~BufferWorker() override { ++destroyed_; }
    int run() const override { return buffer_.front(); }

private:
    int& destroyed_;
    std::vector<int> buffer_;
};

void basic_example()
{
    int destroyed = 0;
    {
        std::unique_ptr<Worker> worker = std::make_unique<BufferWorker>(destroyed);
        assert(worker->run() == 7);
    } // unique_ptr<Worker> 執行 delete Worker*，virtual dispatch 到 ~BufferWorker。
    assert(destroyed == 1);
    std::cout << "[基礎] base pointer deletion 已執行 derived destructor\n";
}

// LeetCode 703：Kth Largest Element in a Stream。
// KthLargest 實作抽象 IntStream；經 base owner 銷毀仍能正確解構 priority_queue。
class IntStream {
public:
    virtual ~IntStream() = default;
    virtual int add(int value) = 0;
};

class KthLargest final : public IntStream {
public:
    KthLargest(std::size_t k, const std::vector<int>& nums) : k_(k)
    {
        for (const int value : nums) add(value);
    }

    int add(int value) override
    {
        smallest_.push(value);
        if (smallest_.size() > k_) smallest_.pop();
        return smallest_.top();
    }

private:
    std::size_t k_;
    std::priority_queue<int, std::vector<int>, std::greater<int>> smallest_;
};

void leetcode_703_example()
{
    std::unique_ptr<IntStream> stream =
        std::make_unique<KthLargest>(3U, std::vector<int>{4, 5, 8, 2});
    const int after_3 = stream->add(3);
    const int after_5 = stream->add(5);
    const int after_10 = stream->add(10);
    const int after_9 = stream->add(9);
    assert(after_3 == 4);
    assert(after_5 == 5);
    assert(after_10 == 5);
    assert(after_9 == 8);
    std::cout << "[LeetCode 703] virtual stream 回傳 kth largest=8\n";
}

// 實務案例：Plugin owner 只知道介面；plugin destructor 可釋放自有 connection/cache。
class Plugin {
public:
    virtual ~Plugin() = default;
    virtual std::string name() const = 0;
};

class MetricsPlugin final : public Plugin {
public:
    explicit MetricsPlugin(bool& stopped) : stopped_(stopped) {}
    ~MetricsPlugin() override { stopped_ = true; }
    std::string name() const override { return "metrics"; }

private:
    bool& stopped_;
};

void practical_example()
{
    bool stopped = false;
    {
        std::unique_ptr<Plugin> plugin = std::make_unique<MetricsPlugin>(stopped);
        assert(plugin->name() == "metrics");
    }
    assert(stopped);
    std::cout << "[實務] plugin 經 base owner 正確 shutdown\n";
}

int main()
{
    basic_example();
    leetcode_703_example();
    practical_example();
}

// 易錯與面試：只要可能透過 base pointer delete derived，base destructor 就必須 virtual；
// 否則行為未定義。若 class 根本不允許 polymorphic delete，可把 destructor protected nonvirtual。
// 練習：查看 `std::has_virtual_destructor_v<Worker>`，並解釋為何結果應為 true。
// 複雜度：polymorphic delete 的 dispatch 常數時間，總解構成本是 derived/base/member cleanup 總和。
// 生命週期：透過 Base* 刪除時必須由 virtual destructor 走完整 derived chain，否則是 UB/資源漏。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '18_VirtualDestructor.cpp' -o '/tmp/codex_cpp_C_OOP_18_VirtualDestructor' && '/tmp/codex_cpp_C_OOP_18_VirtualDestructor'
//
// === 預期輸出（節錄）===
// [實務] plugin 經 base owner 正確 shutdown
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
