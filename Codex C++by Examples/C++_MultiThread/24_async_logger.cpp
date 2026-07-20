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

// -----------------------------------------------------------------------------
// 【日常實務範例】前景低延遲、關閉時排空的非同步日誌
// 情境：producer 依序提交 record-0..99，同時兩個 closer 可能先關閉；所有 log 回成功的 records 必須在每個 close 返回前寫入 sink。
// 為何使用本章主題：producer 只在 mutex 下 enqueue，單背景 worker 序列化 sink；call_once 讓 concurrent close 共享同一次 drain/join。
// 設計：1. log 鎖內接受完整 owning string。2. worker cv 等待並出列。3. close 停止收件、notify。4. worker 排空後退出並由 closer join。
// 成本：每筆有字串 ownership、queue/written mutex 與配置；空間 O(Q+R)，close latency 取決於 backlog/sink。
// 上線注意：unbounded queue 需 block/drop policy；close/join 不代表 fsync，process crash 仍可能遺失，worker I/O 例外也需傳回 owner。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 359. Logger Rate Limiter（日誌速率限制器）
// 題目：同一 message 每 10 秒最多輸出一次；例如 foo 在 timestamp 1 允許、2 抑制、11 再允許。
// 為何使用本章主題：mutex 讓多 thread 對 next_allowed_ 的查詢與更新成為單一原子操作；原題時間遞增，這裡額外提供 thread-safe wrapper。
// 思路：1. 鎖住 map。2. 查 message 的門檻。3. 未到時間回 false。4. 允許時寫 timestamp+10 並回 true。
// 複雜度：unordered_map 平均每次 O(1)、空間 O(M)，M 為不同訊息數；所有 caller 仍競爭同一 mutex。
// 易錯點：timestamp 等於門檻時要允許；timestamp+10 需防 int overflow，長期服務也要淘汰舊 message 避免 map 無界成長。
// -----------------------------------------------------------------------------
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
