// ============================================================================
// 課題 2：system_clock、steady_clock、high_resolution_clock
// ============================================================================
//
// system_clock 對應 civil/wall clock，可轉 time_t、顯示日期；NTP/管理員調時會向前或
// 向後跳，因此不可可靠量 timeout。steady_clock 保證單調，最適合 elapsed/deadline。
// high_resolution_clock 只是「最短 tick period」的 alias，可能就是 system/steady；
// 不保證 steady，應看 `is_steady` 而非名字猜。
//
// time_point 只能在同 clock domain 比較。若系統同時需要「可讀 wall timestamp」與
//「可靠 elapsed」，事件中通常各記 system_clock::now() 與 steady_clock::now()。
// ============================================================================

#include <cassert>
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <unordered_map>

void basic_example()
{
    static_assert(std::chrono::steady_clock::is_steady);
    const auto monotonic_start = std::chrono::steady_clock::now();
    const auto wall_now = std::chrono::system_clock::now();
    const std::time_t wall_seconds = std::chrono::system_clock::to_time_t(wall_now);
    assert(wall_seconds > 0);
    const auto monotonic_end = std::chrono::steady_clock::now();
    assert(monotonic_end >= monotonic_start);
    std::cout << "[基礎] steady is monotonic; system time_t=" << wall_seconds << '\n';
}

// LeetCode 359：Logger Rate Limiter。
// 題目 timestamp 單調遞增；以 seconds duration 表示，而非與 system_clock 綁定，
// 讓 unit test 完全 deterministic。
class Logger {
public:
    bool should_print_message(std::chrono::seconds timestamp, const std::string& message)
    {
        const auto found = next_allowed_.find(message);
        if (found != next_allowed_.end() && timestamp < found->second) return false;
        next_allowed_[message] = timestamp + std::chrono::seconds(10);
        return true;
    }
private:
    std::unordered_map<std::string, std::chrono::seconds> next_allowed_;
};

void leetcode_359_example()
{
    Logger logger;
    assert(logger.should_print_message(std::chrono::seconds(1), "foo"));
    assert(!logger.should_print_message(std::chrono::seconds(2), "foo"));
    assert(logger.should_print_message(std::chrono::seconds(11), "foo"));
    std::cout << "[LeetCode 359] deterministic 10-second rate limit\n";
}

// 實務：同一事件同時保留 wall/monotonic；log 顯示 wall，SLA 計算 monotonic。
struct EventStamp {
    std::chrono::system_clock::time_point wall;
    std::chrono::steady_clock::time_point monotonic;
};

EventStamp stamp_event()
{
    return {std::chrono::system_clock::now(), std::chrono::steady_clock::now()};
}

void practical_example()
{
    const EventStamp first = stamp_event();
    const EventStamp second = stamp_event();
    assert(second.monotonic >= first.monotonic);
    std::cout << "[實務] wall stamp for logs + steady stamp for elapsed\n";
}

int main()
{
    basic_example();
    leetcode_359_example();
    practical_example();
}

// 練習：印出三個 clock 的 period 與 is_steady，觀察本機 high_resolution_clock alias。
// 複雜度：now() 與 time_point subtraction 通常 O(1)，但實際 syscall/vDSO 成本依平台。
// 生命週期：time_point 是值 snapshot；system_clock 之後被校時不會回頭修改已取得的值。
