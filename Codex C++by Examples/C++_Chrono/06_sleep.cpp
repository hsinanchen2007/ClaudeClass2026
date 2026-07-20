// ============================================================================
// 課題 6：std::this_thread::sleep_for / sleep_until
// ============================================================================
//
// sleep_for(relative duration) 與 sleep_until(absolute time point) 會讓目前 thread 暫停，
// 但 OS scheduler 只保證大致「不早於條件」恢復，可能晚很多；它不是 real-time timer。
// repeated periodic work 若每輪 sleep_for(period)，工作時間會累積 drift；用 steady
// `next += period; sleep_until(next)` 可維持 schedule（若落後仍需定義 catch-up policy）。
//
// sleep 無法被一般方式喚醒；需要 cancel/notify 時用 condition_variable::wait_for/until。
// unit tests 不應真的睡數秒，將 sleeper/clock 注入或用短時間且只驗邏輯。
// ============================================================================

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

void basic_example()
{
    const auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(1ms);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    // 不檢查「剛好 1ms」；scheduler 與 clock granularity 使 elapsed 通常更長。
    assert(elapsed >= std::chrono::steady_clock::duration::zero());
    std::cout << "[基礎] requested 1ms, actual "
              << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()
              << " us\n";
}

// LeetCode 359：Logger Rate Limiter 不該真的 sleep；用邏輯 timestamp 才 deterministic。
class Logger {
public:
    bool should_print(int timestamp, const std::string& message)
    {
        const auto found = next_.find(message);
        if (found != next_.end() && timestamp < found->second) return false;
        next_[message] = timestamp + 10;
        return true;
    }
private:
    std::unordered_map<std::string, int> next_;
};

void leetcode_359_example()
{
    Logger logger;
    assert(logger.should_print(1, "foo"));
    assert(!logger.should_print(2, "foo"));
    assert(logger.should_print(11, "foo"));
    std::cout << "[LeetCode 359] tests simulate time; no 10-second sleep\n";
}

// 實務：計算 periodic absolute deadlines；演示 schedule，不真的等待三個長 periods。
std::vector<std::chrono::steady_clock::time_point> schedule(
    std::chrono::steady_clock::time_point start,
    std::chrono::milliseconds period,
    int count)
{
    std::vector<std::chrono::steady_clock::time_point> deadlines;
    auto next = start;
    for (int index = 0; index < count; ++index) {
        next += period;
        deadlines.push_back(next);
    }
    return deadlines;
}

void practical_example()
{
    const auto origin = std::chrono::steady_clock::time_point{};
    const auto deadlines = schedule(origin, 100ms, 3);
    assert(deadlines.at(0) == origin + 100ms);
    assert(deadlines.at(2) == origin + 300ms);
    std::cout << "[實務] absolute periodic deadlines=100/200/300ms\n";
}

int main()
{
    basic_example();
    leetcode_359_example();
    practical_example();
}

// 易錯與面試：sleep_for/sleep_until 只保證「不早於」期限醒來，scheduler 可能延遲；
// sleep_for 週期 loop 會累積 drift，定期工作應從固定 origin 算下一個 absolute deadline。
// 練習：以 condition_variable 寫可由 stop flag 提前喚醒的 wait_for。
// 複雜度：安排一次 sleep 是 O(1) API 呼叫，但 wall latency 至少 requested duration 且可更久。
// 生命週期：sleep 不替 callback/object 保活；跨等待保存 raw this pointer 仍需 owner/cancellation 設計。
