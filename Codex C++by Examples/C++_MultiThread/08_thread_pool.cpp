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

// -----------------------------------------------------------------------------
// 【日常實務範例】可排空且可回復部分建構的固定工作池
// 情境：服務建立固定 worker pool；若第 N 個 OS thread 啟動失敗，要停止並 join 已啟動者，正常析構時則先排空已接受任務。
// 為何使用本章主題：jthread、condition_variable 與 protected queue 組成長生命週期 executor，攤銷逐 task 建 thread 的成本。
// 設計：1. constructor 逐一啟動 worker。2. 失敗時發布 stopping、喚醒並 join 後重拋。3. worker 取鎖出列、鎖外執行。4. destructor drain/join。
// 成本：建立 W 個 workers；每次 submit 有配置、一次 queue lock 與喚醒，unbounded queue 空間 O(Q)。
// 上線注意：constructor 失敗不會呼叫 destructor；production 還需 bounded queue、拒絕/取消政策、metrics，並避免 worker 同步等待同 pool 任務。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 2235. Add Two Integers（兩整數相加）
// 題目：輸入 num1、num2 並回傳其和；例如 12+5=17、-10+4=-6。
// 為何使用本章主題：加法本身完全不需要多執行緒；這是刻意的 pool 教學改寫，用 future 展示 submit、結果等待與例外通道。
// 思路：1. 以 value capture 建加法 task。2. submit 到 pool。3. worker 執行 packaged_task。4. 呼叫端 get 結果。
// 複雜度：算術時間/空間 O(1)，但實際成本由 task 配置、queue 同步、排程與 future 等待主導。
// 易錯點：有號加法超出 int 是 UB；在 pool worker 內向單 worker pool submit 後立即 get 可能自我 deadlock。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】固定 Worker 的批次平方轉換
// 情境：四個值要平行平方，重用既有兩個 worker 而非為每個元素建立一條 OS thread，並保持輸出順序與輸入一致。
// 為何使用本章主題：每個 packaged task 回傳 future，pool 限制同時執行數；依 futures 原順序 get 可組回穩定結果。
// 設計：1. 每值 submit 一個 value-captured task。2. 保存 futures。3. 依提交順序逐一 get。4. append 到輸出。
// 成本：N 個 task 的總計算 O(N)、future/queue/輸出空間 O(N)，同步與配置成本可能大於簡單平方。
// 上線注意：平方要防 int overflow；任一 future 丟例外時仍須消費/管理其餘工作，且大量 producer 需 backpressure 防止 queue 耗盡記憶體。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_thread_pool.cpp' -o '/tmp/codex_cpp_C_MultiThread_08_thread_pool' && '/tmp/codex_cpp_C_MultiThread_08_thread_pool'
//
// === 預期輸出（節錄）===
// thread pool：submit、future 與 shutdown drain 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
