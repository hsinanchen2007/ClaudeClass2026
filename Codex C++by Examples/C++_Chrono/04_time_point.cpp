// ============================================================================
// 課題 4：time_point、deadline 與 time_since_epoch
// ============================================================================
//
// time_point 是「Clock epoch + duration」。同 clock 的 time_points 可相減得 duration，
// time_point 可加減 duration。`time_since_epoch()` 主要用於 serialization/interoperability；
// steady_clock epoch 未指定且跨 process/boot 無意義，絕不可持久化成未來可比較 timestamp。
//
// timeout 應先算 absolute steady deadline，再在 loop 比 `now>=deadline`；若每次 operation
// 都重新 sleep relative timeout，spurious wakeup/retry 會讓總等待無限延長。
// ============================================================================

#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std::chrono_literals;

bool expired(std::chrono::steady_clock::time_point now,
             std::chrono::steady_clock::time_point deadline)
{
    return now >= deadline;
}

void basic_example()
{
    const auto start = std::chrono::steady_clock::time_point{1'000ms};
    const auto deadline = start + 250ms;
    assert(!expired(start + 249ms, deadline));
    assert(expired(start + 250ms, deadline));
    std::cout << "[基礎] absolute deadline expires exactly at 1250ms\n";
}

// LeetCode 636：Exclusive Time of Functions。
// 題目 timestamp 是 discrete seconds；用 duration 讓 `end` log 的 inclusive +1 清楚。
struct LogRecord {
    int function_id;
    bool start;
    std::chrono::seconds timestamp;
};

std::vector<int> exclusive_time(int functions, const std::vector<LogRecord>& logs)
{
    std::vector<int> result(static_cast<std::size_t>(functions), 0);
    std::vector<int> stack;
    std::chrono::seconds previous{0};
    for (const LogRecord& log : logs) {
        if (log.start) {
            if (!stack.empty()) {
                result.at(static_cast<std::size_t>(stack.back())) +=
                    static_cast<int>((log.timestamp - previous).count());
            }
            stack.push_back(log.function_id);
            previous = log.timestamp;
        } else {
            const auto elapsed = log.timestamp - previous + 1s;
            result.at(static_cast<std::size_t>(stack.back())) +=
                static_cast<int>(elapsed.count());
            stack.pop_back();
            previous = log.timestamp + 1s;
        }
    }
    return result;
}

void leetcode_636_example()
{
    const std::vector<LogRecord> logs{
        {0, true, 0s}, {1, true, 2s}, {1, false, 5s}, {0, false, 6s}};
    assert((exclusive_time(2, logs) == std::vector<int>{3, 4}));
    std::cout << "[LeetCode 636] exclusive times=3,4 seconds\n";
}

// 實務：testable deadline API 接收 now，不在邏輯深處硬呼叫 clock::now()。
class Lease {
public:
    Lease(std::chrono::steady_clock::time_point issued,
          std::chrono::steady_clock::duration lifetime)
        : expires_at_(issued + lifetime) {}
    bool valid_at(std::chrono::steady_clock::time_point now) const { return now < expires_at_; }
private:
    std::chrono::steady_clock::time_point expires_at_;
};

void practical_example()
{
    const auto issued = std::chrono::steady_clock::time_point{5s};
    const Lease lease(issued, 30s);
    assert(lease.valid_at(issued + 29s));
    assert(!lease.valid_at(issued + 30s));
    std::cout << "[實務] injected time makes lease boundary deterministic\n";
}

int main()
{
    basic_example();
    leetcode_636_example();
    practical_example();
}

// 練習：讓 Lease 支援 renew(now,lifetime)，並拒絕非正 duration。
// 複雜度：Lease 到期比較 O(1)；exclusive-time stack 演算法 O(N) 且 stack 空間 O(depth)。
// 生命週期：Lease 按值保存 deadline；不可保存指向 caller local time_point 的 reference。

/*
【本課面試問答】
Q1：不同 clock 的 `time_point` 可以直接相減嗎？
A：不行。time_point 的 clock 是型別的一部分，system_clock 與 steady_clock 沒有通用、固定 offset；
一個適合牆鐘/日誌，一個適合 deadline。需要兩種語意就像本課一樣各自記錄。

Q2：API 接 deadline 還是 timeout duration 較好？
A：多層重試通常把 duration 在入口轉成 steady deadline，後續每層比較同一 deadline，避免每層重新
給完整 timeout 而超時。測試時注入 `now`/clock，可精確驗 boundary 而不 sleep。

Q3：`time_point + duration` 一定不 overflow 嗎？
A：不保證；底層 `rep` 仍是有限寬度。外部輸入的巨大 timeout 要先 range-check/saturate，不能因 chrono
是強型別就忽略整數 overflow。系統 API 的最大可表示 deadline 也可能更小。
*/
