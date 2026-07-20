// ============================================================================
// 課題 1：<chrono> 全覽 - duration、time_point、clock
// ============================================================================
//
// chrono 用型別區分三件事：
//   duration<Rep,Period>：時間長度，例如 250ms；可加減/比較。
//   time_point<Clock,Duration>：某個 clock domain 上的一個時間點。
//   Clock：提供 now() 與 epoch/單調性語意。
//
// `deadline = now + timeout` 合法；`elapsed = end - start` 得 duration。不同 clocks 的
// time_points 不可直接相減，因 epoch/跳動語意不同。量 elapsed/deadline 用 steady_clock；
// 顯示/持久化 wall time 用 system_clock。
//
// chrono 的 type safety 可防「毫秒誤當秒」，但 `.count()` 會丟掉單位資訊，API boundary
// 應盡量直接接 duration，而非裸 integer。
// ============================================================================

#include <cassert>
#include <chrono>
#include <deque>
#include <iostream>

using namespace std::chrono_literals;

void basic_example()
{
    const auto timeout = 2s + 500ms;
    assert(timeout == 2'500ms);
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
    assert(seconds.count() == 2); // duration_cast 對整數 duration 截斷，不是四捨五入。

    const auto start = std::chrono::steady_clock::now();
    const auto deadline = start + timeout;
    assert(deadline - start == timeout);
    std::cout << "[基礎] timeout=2500ms，cast seconds=2\n";
}

// LeetCode 933：Number of Recent Calls。
// 題目用 int milliseconds；domain class 內改用 chrono::milliseconds，避免單位混淆。
class RecentCounter {
public:
    int ping(std::chrono::milliseconds now)
    {
        calls_.push_back(now);
        const auto earliest = now - 3'000ms;
        while (!calls_.empty() && calls_.front() < earliest) calls_.pop_front();
        return static_cast<int>(calls_.size());
    }
private:
    std::deque<std::chrono::milliseconds> calls_;
};

void leetcode_933_example()
{
    RecentCounter counter;
    const int at_1 = counter.ping(1ms);
    const int at_100 = counter.ping(100ms);
    const int at_3_001 = counter.ping(3'001ms);
    const int at_3_002 = counter.ping(3'002ms);
    assert(at_1 == 1);
    assert(at_100 == 2);
    assert(at_3_001 == 3);
    assert(at_3_002 == 3);
    std::cout << "[LeetCode 933] chrono window retains 3 recent calls\n";
}

// 實務：API 直接接受 duration，呼叫點帶單位，避免裸數字 `set_timeout(5000)` 歧義。
class ClientOptions {
public:
    void set_timeout(std::chrono::milliseconds timeout)
    {
        assert(timeout > 0ms);
        timeout_ = timeout;
    }
    std::chrono::milliseconds timeout() const { return timeout_; }
private:
    std::chrono::milliseconds timeout_{1s};
};

void practical_example()
{
    ClientOptions options;
    options.set_timeout(5s);
    assert(options.timeout() == 5'000ms);
    std::cout << "[實務] typed timeout=5000ms\n";
}

int main()
{
    basic_example();
    leetcode_933_example();
    practical_example();
}

// 練習：讓 set_timeout 接 template duration，內部統一轉 milliseconds 並檢查 overflow。
// 複雜度與生命週期：duration/time_point arithmetic 是 O(1) 值運算；ClientOptions 按值保存
// timeout，所以呼叫端的 temporary duration 結束後設定仍有效。
