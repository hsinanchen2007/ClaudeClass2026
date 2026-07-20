// ============================================================================
// 課題 11：static data member 與 static member function
// ============================================================================
//
// 一般 member 每個 object 各有一份；static data member 屬於 class，全體 object 共用。
// static member function 沒有 this，只能直接存取 static members。C++17 的
// `inline static` 可在 class 內定義並避免另外提供一個 translation-unit definition。
//
// static 適合 class-wide counter、factory、常數；但可變 global-like state 會造成測試
// 互相污染、初始化順序與多執行緒同步問題。跨 thread 修改 counter 應用 atomic/mutex。
//
// 【面試】function-local static 自 C++11 起初始化是 thread-safe，但後續修改不自動安全。
// 【陷阱】static object 的解構順序跨 translation units 不明確，稱 static destruction
// order fiasco；避免在 shutdown 時讓 global objects 互相依賴。
// ============================================================================

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

class Ticket {
public:
    Ticket() : id_(next_id_++) { ++live_count_; }
    ~Ticket() noexcept { --live_count_; }
    Ticket(const Ticket&) = delete;
    Ticket& operator=(const Ticket&) = delete;

    int id() const { return id_; }
    static int live_count() { return live_count_; }

private:
    inline static int next_id_ = 1;
    inline static int live_count_ = 0;
    int id_;
};

void basic_example()
{
    assert(Ticket::live_count() == 0);
    {
        Ticket first;
        Ticket second;
        assert(first.id() == 1 && second.id() == 2);
        assert(Ticket::live_count() == 2);
    }
    assert(Ticket::live_count() == 0);
    std::cout << "[基礎] static counter 由所有 Ticket 共用\n";
}

// LeetCode 535：Encode and Decode TinyURL。
// 不同 Codec instance 共用 static storage，模擬同一服務的 process-wide URL registry。
class Codec {
public:
    std::string encode(const std::string& long_url)
    {
        const std::string key = std::to_string(next_key_++);
        urls_[key] = long_url;
        return "https://tiny/" + key;
    }

    static std::string decode(const std::string& short_url)
    {
        const std::size_t slash = short_url.find_last_of('/');
        return urls_.at(short_url.substr(slash + 1U));
    }

private:
    inline static unsigned long next_key_ = 1UL;
    inline static std::unordered_map<std::string, std::string> urls_;
};

void leetcode_535_example()
{
    Codec encoder;
    Codec another_instance;
    const std::string short_url = encoder.encode("https://leetcode.com/problems/design-tinyurl");
    assert(another_instance.decode(short_url) ==
           "https://leetcode.com/problems/design-tinyurl");
    std::cout << "[LeetCode 535] " << short_url << " 可由另一 instance decode\n";
}

// 實務案例：factory 名稱用 static function，因建立物件前根本沒有 this 可用。
class RetryPolicy {
public:
    static RetryPolicy interactive() { return RetryPolicy(3, 100); }
    static RetryPolicy batch() { return RetryPolicy(10, 1'000); }
    int attempts() const { return attempts_; }
    int delay_ms() const { return delay_ms_; }

private:
    RetryPolicy(int attempts, int delay_ms) : attempts_(attempts), delay_ms_(delay_ms) {}
    int attempts_;
    int delay_ms_;
};

void practical_example()
{
    const RetryPolicy policy = RetryPolicy::batch();
    assert(policy.attempts() == 10 && policy.delay_ms() == 1'000);
    std::cout << "[實務] static named factory 建立 batch retry policy\n";
}

int main()
{
    basic_example();
    leetcode_535_example();
    practical_example();
}

// 練習：把 next_id_ 改成 std::atomic<int>，思考 relaxed ordering 是否足夠。
// 複雜度：static counter increment 通常 O(1)；atomic 版仍 O(1) API 但有同步常數成本。
// 生命週期：inline static member 具 static storage duration，所有 instances 共用且活到程序結束。
