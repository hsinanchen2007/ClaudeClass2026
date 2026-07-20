// ============================================================================
// Thread Pool：重用 worker、bounded concurrency 與 future 結果
// ============================================================================
// 每個小 task 都建立 thread 很昂貴。pool 維持固定 worker，task 放進受 mutex 保護的
// queue，condition_variable 喚醒 worker。submit 以 packaged_task 把 callable 的
// return/exception 放入 future。shutdown 必須定義：停止收件、排空既有任務、喚醒並 join。
//
// 本例是教學用 unbounded queue；production 還需 backpressure、priority、metrics、
// task cancellation、worker exception 邊界與避免 pool worker 等同 pool future 的死鎖。

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

class ThreadPool {
public:
    // WorkerStarter 是可注入的啟動介面：一般 caller 使用單參數 constructor；
    // 教材測試則可穩定模擬「第 N 個 OS thread 建立失敗」，不用真的耗盡系統資源。
    using WorkerStarter = std::function<std::jthread(std::function<void()>)>;

    explicit ThreadPool(std::size_t worker_count)
        : ThreadPool(worker_count, [](std::function<void()> worker) {
              return std::jthread(std::move(worker));
          })
    {
    }

    ThreadPool(std::size_t worker_count, WorkerStarter start_worker)
    {
        if (worker_count == 0U) {
            throw std::invalid_argument("ThreadPool requires at least one worker");
        }
        if (!start_worker) throw std::invalid_argument("WorkerStarter must be callable");

        workers_.reserve(worker_count);
        try {
            for (std::size_t index = 0; index < worker_count; ++index) {
                static_cast<void>(index);
                std::jthread worker = start_worker([this] { worker_loop(); });
                if (!worker.joinable()) {
                    throw std::invalid_argument("WorkerStarter returned no worker");
                }
                workers_.emplace_back(std::move(worker));
            }
        } catch (...) {
            // Constructor 尚未成功時 ~ThreadPool 不會執行。若前幾個 worker 已在 cv
            // 等待，直接讓 vector<jthread> 解構會 join 永不退出的 worker，因此必須先
            // 發布 stopping、喚醒並 join，完成 rollback 後才重拋原始建構例外。
            stop_and_join();
            throw;
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() { stop_and_join(); }

    template <class Function>
    auto submit(Function function) -> std::future<std::invoke_result_t<Function>>
    {
        using Result = std::invoke_result_t<Function>;
        auto task = std::make_shared<std::packaged_task<Result()>>(std::move(function));
        std::future<Result> future = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                throw std::runtime_error("submit after shutdown");
            }
            tasks_.push([task] { (*task)(); });
        }
        condition_.notify_one();
        return future;
    }

private:
    void stop_and_join() noexcept
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        condition_.notify_all();
        for (std::jthread& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }

    void worker_loop()
    {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
                if (stopping_ && tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();  // packaged_task 捕捉 callable exception 到 future。
        }
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<std::function<void()>> tasks_;
    bool stopping_ = false;
    std::vector<std::jthread> workers_;
};

void basic_demo()
{
    ThreadPool pool(2U);
    auto first = pool.submit([] { return 20; });
    auto second = pool.submit([] { return 22; });
    expect(first.get() + second.get() == 42, "thread-pool result mismatch");

    bool zero_rejected = false;
    try {
        ThreadPool invalid(0U);
    } catch (const std::invalid_argument&) {
        zero_rejected = true;
    }
    expect(zero_rejected, "zero-worker pool must be rejected in release builds too");
}

// ----------------------------------------------------------------------------
// LeetCode 2235：Add Two Integers（透過 pool 執行）
// ----------------------------------------------------------------------------
int add_two_integers(ThreadPool& pool, int left, int right)
{
    return pool.submit([left, right] { return left + right; }).get();
}

void leetcode_demo()
{
    ThreadPool pool(2U);
    const int positive = add_two_integers(pool, 12, 5);
    const int mixed = add_two_integers(pool, -10, 4);
    expect(positive == 17 && mixed == -6, "LeetCode addition result mismatch");
}

// ----------------------------------------------------------------------------
// 實務：批次轉換，共用固定兩個 worker 而非建立 N 個 thread
// ----------------------------------------------------------------------------
std::vector<int> square_batch(ThreadPool& pool, const std::vector<int>& values)
{
    std::vector<std::future<int>> futures;
    futures.reserve(values.size());
    for (const int value : values) {
        futures.push_back(pool.submit([value] { return value * value; }));
    }
    std::vector<int> result;
    result.reserve(values.size());
    for (auto& future : futures) {
        result.push_back(future.get());
    }
    return result;
}

void practical_demo()
{
    ThreadPool pool(2U);
    const auto squares = square_batch(pool, {1, 2, 3, 4});
    expect((squares == std::vector<int>{1, 4, 9, 16}), "batch result mismatch");

    std::size_t starts = 0U;
    std::atomic<int> exited{0};
    bool startup_failed = false;
    try {
        ThreadPool partially_started(
            3U, [&](std::function<void()> worker) -> std::jthread {
                if (starts++ == 1U) {
                    throw std::system_error(
                        std::make_error_code(std::errc::resource_unavailable_try_again));
                }
                return std::jthread([worker = std::move(worker), &exited]() mutable {
                    worker();
                    exited.fetch_add(1, std::memory_order_relaxed);
                });
            });
    } catch (const std::system_error&) {
        startup_failed = true;
    }
    expect(startup_failed && starts == 2U && exited.load(std::memory_order_relaxed) == 1,
           "partial construction did not stop and join started workers");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "thread pool：submit、future 與 shutdown drain 測試通過\n";
}

// 【陷阱】pool 只有一個 worker 時，task A submit task B 並同步 get B 會自我死鎖。
// 【陷阱】constructor failure 不會呼叫 class destructor；已啟動 worker 要在 catch 內 rollback。
// 【陷阱】unbounded queue 在 producer 過快時耗盡記憶體；production 要 backpressure。
// 【面試】為何 task 在鎖外執行？否則 submit/其他 worker 全被長任務阻塞。
// 【練習】加入 bounded queue 與 submit timeout，定義拒絕策略。
