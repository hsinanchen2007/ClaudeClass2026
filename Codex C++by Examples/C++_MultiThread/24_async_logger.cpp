// ============================================================================
// Async Logger：前景 enqueue、背景序列化、flush 與 shutdown durability
// ============================================================================
// producer 不直接做慢 I/O，而將完整 log record 放 queue；單 background worker 依序寫出。
// queue 需要 mutex/cv，close 時停止收件並 drain。多個外部 thread 可 concurrent close；
// 每個 close 都等到 drain+join 完成才返回。這降低 caller latency，但 unbounded queue
// 在磁碟慢時會吃光記憶體；production 要 bounded queue 與 drop/block policy。
// flush 的契約必須明確：只代表寫到 user-space stream、OS page cache，或持久媒體 fsync？

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <latch>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

class AsyncLogger {
public:
    AsyncLogger() : worker_([this] { run(); }) {}
    ~AsyncLogger() { close(); }

    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;

    void log(std::string record)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) throw std::runtime_error("log after close");
            pending_.push(std::move(record));
        }
        condition_.notify_one();
    }

    void close()
    {
        std::call_once(close_once_, [this] {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                // log 與 close 共用此 mutex：先取得鎖者決定該 record 被接受或拒絕。
                closed_ = true;
            }
            condition_.notify_all();
            if (worker_.joinable()) worker_.join();
        });
    }

    std::vector<std::string> records() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return written_;
    }

private:
    void run()
    {
        for (;;) {
            std::string record;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] { return closed_ || !pending_.empty(); });
                if (pending_.empty() && closed_) return;
                record = std::move(pending_.front());
                pending_.pop();
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                written_.push_back(std::move(record));
            }
        }
    }

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<std::string> pending_;
    std::vector<std::string> written_;  // 教材 sink；production 可換 file/socket。
    bool closed_ = false;
    std::once_flag close_once_;
    std::jthread worker_;
};

void basic_demo()
{
    AsyncLogger logger;
    logger.log("first");
    logger.log("second");
    std::thread first_closer([&] { logger.close(); });
    std::thread second_closer([&] { logger.close(); });
    first_closer.join();
    second_closer.join();
    expect((logger.records() == std::vector<std::string>{"first", "second"}),
           "logger close did not drain accepted records");

    bool rejected = false;
    try {
        logger.log("too late");
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    expect(rejected, "log after close must be rejected");
}

// ----------------------------------------------------------------------------
// LeetCode 359：Logger Rate Limiter
// ----------------------------------------------------------------------------
class RateLimiter {
public:
    bool should_print(int timestamp, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = next_allowed_.find(message);
        if (found != next_allowed_.end() && timestamp < found->second) return false;
        next_allowed_[message] = timestamp + 10;
        return true;
    }

private:
    std::mutex mutex_;
    std::unordered_map<std::string, int> next_allowed_;
};

void leetcode_demo()
{
    RateLimiter limiter;
    const bool first = limiter.should_print(1, "foo");
    const bool suppressed = limiter.should_print(2, "foo");
    const bool after_window = limiter.should_print(11, "foo");
    expect(first && !suppressed && after_window, "rate limiter window mismatch");
}

// ----------------------------------------------------------------------------
// 實務：兩 producer；每一 producer 內順序保留，全域 interleaving 不作假設
// ----------------------------------------------------------------------------
void practical_demo()
{
    AsyncLogger logger;
    std::latch start{1};
    std::atomic<std::size_t> accepted{0U};

    std::thread producer([&] {
        start.wait();
        for (int sequence = 0; sequence < 100; ++sequence) {
            try {
                logger.log("record-" + std::to_string(sequence));
                accepted.fetch_add(1U, std::memory_order_relaxed);
            } catch (const std::runtime_error&) {
                break;
            }
        }
    });
    std::thread first_closer([&] { start.wait(); logger.close(); });
    std::thread second_closer([&] { start.wait(); logger.close(); });
    start.count_down();

    producer.join();
    first_closer.join();
    second_closer.join();
    const auto records = logger.records();
    expect(records.size() == accepted.load(std::memory_order_relaxed),
           "close returned before every accepted record was written");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "async logger：queue、drain 與 rate limiter 測試通過\n";
}

// 【陷阱】寫入 queue 的 record 必須擁有字串；不可保存 caller 的 string_view 到背景。
// 【陷阱】process crash/斷電時背景 queue 未必落盤；close/join 也不等於 storage fsync。
// 【陷阱】joinable()+join 不是 concurrent close guard；同一 thread object 不可被同時 join。
// 【注意】close 可與 log/close 並行；destructor 與其他 member call 仍不可重疊。
// 【面試】滿 queue 策略：block、drop newest/oldest、sample，各自對 latency/durability 影響。
// 【練習】加入 monotonic sequence number，測試 written records 可依 sequence 重排。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '24_async_logger.cpp' -o '/tmp/codex_cpp_C_MultiThread_24_async_logger' && '/tmp/codex_cpp_C_MultiThread_24_async_logger'
//
// === 預期輸出（節錄）===
// async logger：queue、drain 與 rate limiter 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
