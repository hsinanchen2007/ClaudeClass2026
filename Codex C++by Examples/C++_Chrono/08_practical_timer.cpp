// ============================================================================
// 課題 8：實務 ScopeTimer 與可測量 instrumentation
// ============================================================================
//
// ScopeTimer 在 constructor 記 steady start、destructor 計算 elapsed 並送給 callback，
// 所有 return/exception 路徑都會記錄。destructor 必須 noexcept；callback 若可能 throw，
// 應捕捉或提供明確 stop()。Timer 自己的輸出 I/O 可能干擾很短 operation，microbenchmark
// 不應每輪直接印 log。
//
// 真實 tracing 常記 label、thread id、correlation id，批次送 metrics；本例以 callback
// 注入避免把 logger/global singleton 綁死。
// ============================================================================

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class ScopeTimer {
public:
    using Clock = std::chrono::steady_clock;
    using Callback = std::function<void(std::chrono::nanoseconds)>;

    explicit ScopeTimer(Callback callback)
        : callback_(std::move(callback)), start_(Clock::now()) {}

    ~ScopeTimer() noexcept
    {
        try {
            callback_(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start_));
        } catch (...) {
            // destructor 不讓 metrics failure 破壞主流程。
        }
    }

    ScopeTimer(const ScopeTimer&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;

private:
    Callback callback_;
    Clock::time_point start_;
};

void basic_example()
{
    std::chrono::nanoseconds observed{-1};
    {
        ScopeTimer timer([&](std::chrono::nanoseconds elapsed) { observed = elapsed; });
        int value = 1 + 1;
        assert(value == 2);
    }
    assert(observed.count() >= 0);
    std::cout << "[基礎] RAII timer observed " << observed.count() << " ns\n";
}

// LeetCode 70：Climbing Stairs。Timer 不改演算法，只在 scope exit 收集 elapsed。
int climb_stairs(int steps)
{
    int previous = 0;
    int current = 1;
    for (int step = 1; step <= steps; ++step) {
        const int next = previous + current;
        previous = current;
        current = next;
    }
    return current;
}

void leetcode_70_example()
{
    std::chrono::nanoseconds elapsed{0};
    int answer = 0;
    {
        ScopeTimer timer([&](std::chrono::nanoseconds value) { elapsed = value; });
        answer = climb_stairs(20);
    }
    assert(answer == 10'946 && elapsed.count() >= 0);
    std::cout << "[LeetCode 70] answer=10946, instrumentation=" << elapsed.count() << " ns\n";
}

// 實務：收集多個 jobs 的 durations，計算總和；callback 可替換成 metrics backend。
void practical_example()
{
    std::vector<std::chrono::nanoseconds> samples;
    for (int job = 0; job < 3; ++job) {
        ScopeTimer timer([&](std::chrono::nanoseconds elapsed) { samples.push_back(elapsed); });
        assert(job >= 0);
    }
    assert(samples.size() == 3U);
    std::cout << "[實務] scope timer collected 3 job samples\n";
}

int main()
{
    basic_example();
    leetcode_70_example();
    practical_example();
}

// 易錯與面試：RAII timer destructor 不應讓 callback exception 逸出而導致 terminate；
// benchmark 要用 steady_clock、warm-up、多次樣本，並防止 compiler 消除待測工作。
// 練習：加入 stop()，保證 callback 最多一次，並測試 move semantics。
// 複雜度：timer 本體 start/stop O(1)，callback 成本另計；大量細粒度 scope 可能反客為主。
// 生命週期：RAII timer 在 scope exit 執行 callback；callback 捕獲的 reference 必須仍存活。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_practical_timer.cpp' -o '/tmp/codex_cpp_C_Chrono_08_practical_timer' && '/tmp/codex_cpp_C_Chrono_08_practical_timer'
//
// === 預期輸出（節錄）===
// [實務] scope timer collected 3 job samples
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
