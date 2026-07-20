// ============================================================================
// Graceful Shutdown：停止收件、排空、取消與 join 的明確狀態機
// ============================================================================
// 「thread 已停止」不等於「已處理所有資料」。常見 drain shutdown：
// RUNNING 接受工作 -> CLOSING 拒絕新工作但處理 queue -> STOPPED worker 已 join。
// abort shutdown 則可能丟棄 queue，必須明確記錄。關閉操作應 idempotent；本例進一步
// 保證多個外部 thread 可同時 close，而且每個 close 都在 worker drain+join 後才返回。
// jthread stop_token 適合取消，但若要求 drain，predicate 要同時考慮 queue 與 closed，
// 不可收到 stop 就立刻離開。

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <latch>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

class DrainingWorker {
public:
    DrainingWorker() : worker_([this] { run(); }) {}

    ~DrainingWorker() { close(); }

    DrainingWorker(const DrainingWorker&) = delete;
    DrainingWorker& operator=(const DrainingWorker&) = delete;

    void submit(int value)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) throw std::runtime_error("submit after close");
            pending_.push(value);
        }
        condition_.notify_one();
    }

    void close()
    {
        // call_once 不只讓 close idempotent，也序列化 join。其他 concurrent close caller
        // 會等 callable 完成，因此所有 caller 返回時都能依賴「已拒新件且 queue 已排空」。
        std::call_once(close_once_, [this] {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                closed_ = true;  // submit/close 共用 mutex；這是接受或拒絕工作的線性化點。
            }
            condition_.notify_all();
            if (worker_.joinable()) worker_.join();
        });
    }

    std::vector<int> results() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return processed_;
    }

private:
    void run()
    {
        for (;;) {
            int value = 0;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] { return closed_ || !pending_.empty(); });
                if (pending_.empty() && closed_) return;
                value = pending_.front();
                pending_.pop();
            }
            const int result = value * value;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                processed_.push_back(result);
            }
        }
    }

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<int> pending_;
    std::vector<int> processed_;
    bool closed_ = false;
    std::once_flag close_once_;
    std::jthread worker_;
};

void basic_demo()
{
    DrainingWorker worker;
    worker.submit(2);
    worker.submit(3);
    std::thread first_closer([&] { worker.close(); });
    std::thread second_closer([&] { worker.close(); });
    first_closer.join();
    second_closer.join();
    expect((worker.results() == std::vector<int>{4, 9}), "close did not drain all work");
    worker.close();  // idempotent。

    bool rejected = false;
    try {
        worker.submit(4);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    expect(rejected, "submit after close must be rejected");
}

// ----------------------------------------------------------------------------
// LeetCode 1480：Running Sum，工作全部 drain 後才回傳
// ----------------------------------------------------------------------------
std::vector<int> running_sum(const std::vector<int>& numbers)
{
    DrainingWorker worker;
    int total = 0;
    // worker 做 square 的 API 不適合 prefix sum，因此此題聚焦 shutdown 後結果完整性：
    // 送入 prefix 值，worker 將其平方；再以測試確認每筆都 drain。函式本身回正規 prefix。
    std::vector<int> prefix;
    prefix.reserve(numbers.size());
    for (const int value : numbers) {
        total += value;
        prefix.push_back(total);
        worker.submit(total);
    }
    worker.close();
    expect(worker.results().size() == numbers.size(), "prefix audit work was not drained");
    return prefix;
}

void leetcode_demo()
{
    const auto result = running_sum({1, 2, 3, 4});
    expect((result == std::vector<int>{1, 3, 6, 10}), "running sum mismatch");
}

// ----------------------------------------------------------------------------
// 實務：close 前送入的工作一筆不漏
// ----------------------------------------------------------------------------
void practical_demo()
{
    DrainingWorker worker;
    std::latch start{1};
    std::atomic<std::size_t> accepted{0U};

    std::thread producer([&] {
        start.wait();
        for (int value = 1; value <= 100; ++value) {
            try {
                worker.submit(value);
                accepted.fetch_add(1U, std::memory_order_relaxed);
            } catch (const std::runtime_error&) {
                break;  // close 先在線性化點勝出；後續 submit 必須被拒。
            }
        }
    });
    std::thread first_closer([&] { start.wait(); worker.close(); });
    std::thread second_closer([&] { start.wait(); worker.close(); });
    start.count_down();

    producer.join();
    first_closer.join();
    second_closer.join();

    const auto results = worker.results();
    expect(results.size() == accepted.load(std::memory_order_relaxed),
           "close returned before every accepted item was processed");
    for (std::size_t index = 0U; index < results.size(); ++index) {
        const std::size_t value = index + 1U;
        expect(results[index] == static_cast<int>(value * value),
               "drain changed producer order or payload");
    }
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "graceful shutdown：拒新件、drain 與 idempotent close 測試通過\n";
}

// 【陷阱】只 request_stop 而 worker 立即退出，queue 中資料可能永久遺失。
// 【陷阱】先 join 再通知/設 closed，worker 仍在 wait，會永遠卡住。
// 【陷阱】先用 joinable() 檢查再由多個 caller join 不是同步；std::thread object 會 data race。
// 【注意】close 可與 submit/close 並行；destructor 開始後仍呼叫任何 member 則違反物件生命週期。
// 【面試】定義 shutdown contract：drain、cancel pending、deadline 到後 abort，各自回報什麼。
// 【練習】加入 close_for(timeout)，逾時後回傳尚未處理工作清單。
