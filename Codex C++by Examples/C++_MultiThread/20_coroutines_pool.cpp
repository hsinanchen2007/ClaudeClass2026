// ============================================================================
// Coroutine + Executor：co_await 只描述暫停點，executor 才決定在哪裡恢復
// ============================================================================
// coroutine 不是 thread。呼叫 coroutine function 會建立 frame；co_await 可暫停並把
// coroutine_handle 交給 scheduler，worker resume 後從暫停點繼續。promise_type 控制
// return value、exception、initial/final suspend；frame 必須最後 destroy，不能重複 resume。
//
// 最容易漏掉的 lifetime 規則是：到達 final_suspend 不代表另一個 thread 的 resume()
// 已經返回。若 final_suspend 先通知等待者，等待者可能立刻 destroy frame，而 worker 仍在
// resume 呼叫中，形成 use-after-free/UB。本例以 active_resumes 記帳；final suspend 只記錄
// 「已到終點」，真正的 done 由最後一個 resume 返回後發布。
//
// 本例是單 worker Executor。IntTask::get 是 blocking bridge；若 worker 等待同 executor
// queue 內的子 task，單純 cv.wait 會自我死鎖。因此 worker 會協助執行 ready queue，直到
// 目標完成。這會引入 reentrancy；大型 async 系統通常改用 co_await continuation。

#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>

class Executor;

struct IntTaskState {
    mutable std::mutex mutex;
    std::condition_variable condition;
    bool final_reached = false;
    std::size_t active_resumes = 0U;
    bool done = false;
    int value = 0;
    std::exception_ptr error;
    Executor* executor = nullptr;  // non-owning；pending task 的 executor 必須仍存活。
};

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

class Executor {
public:
    Executor() : worker_([this] { run(); }) {}

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    ~Executor()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        condition_.notify_all();
        if (worker_.joinable()) worker_.join();
    }

    void post(std::coroutine_handle<> handle, std::shared_ptr<IntTaskState> state)
    {
        // 先把 scheduler 身分寫入共享 state，再把 handle 發布到 ready queue。worker 可能在
        // post 返回前就 resume；await_suspend 因此不得在發布後再碰 coroutine frame。
        {
            std::lock_guard<std::mutex> state_lock(state->mutex);
            if (state->executor != nullptr && state->executor != this) {
                throw std::logic_error("one IntTask cannot migrate between executors");
            }
            state->executor = this;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) throw std::runtime_error("post after executor shutdown");
            ready_.push(ReadyTask{handle, std::move(state)});
        }
        condition_.notify_one();
    }

    bool runs_in_this_thread() const noexcept { return current_executor_ == this; }

    // worker 不能睡著等自己的 queue。協助執行 work 能解除單 worker nested get 的環；
    // 若目標本身已在目前 call stack 執行，則是真正的 cycle，明確丟例外而非永久等待。
    void help_until(const std::shared_ptr<IntTaskState>& target)
    {
        for (;;) {
            {
                std::lock_guard<std::mutex> state_lock(target->mutex);
                if (target->done) return;
                if (target->active_resumes != 0U) {
                    throw std::logic_error("executor task cannot wait for itself");
                }
            }

            ReadyTask next;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (ready_.empty()) {
                    throw std::logic_error(
                        "executor worker would block without runnable work");
                }
                next = std::move(ready_.front());
                ready_.pop();
            }
            execute(std::move(next));
        }
    }

private:
    struct ReadyTask {
        std::coroutine_handle<> handle{};
        std::shared_ptr<IntTaskState> state;
    };

    static void execute(ReadyTask task)
    {
        {
            std::lock_guard<std::mutex> lock(task.state->mutex);
            ++task.state->active_resumes;
        }

        task.handle.resume();

        bool publish_done = false;
        {
            std::lock_guard<std::mutex> lock(task.state->mutex);
            --task.state->active_resumes;
            if (task.state->final_reached && task.state->active_resumes == 0U &&
                !task.state->done) {
                task.state->done = true;
                publish_done = true;
            }
        }
        // 從此行開始不再解參考 coroutine_handle；被喚醒的 owner 可安全 destroy frame。
        if (publish_done) task.state->condition.notify_all();
    }

    void run()
    {
        current_executor_ = this;
        for (;;) {
            ReadyTask task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] { return stopping_ || !ready_.empty(); });
                if (stopping_ && ready_.empty()) break;
                task = std::move(ready_.front());
                ready_.pop();
            }
            execute(std::move(task));
        }
        current_executor_ = nullptr;
    }

    static inline thread_local const Executor* current_executor_ = nullptr;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<ReadyTask> ready_;
    bool stopping_ = false;
    std::jthread worker_;
};

struct ScheduleOn {
    Executor& executor;

    bool await_ready() const noexcept { return false; }

    template <class Promise>
    void await_suspend(std::coroutine_handle<Promise> handle) const
    {
        // completion_state() 的 shared_ptr 在發布前取出；post 後不再讀 promise/frame。
        executor.post(handle, handle.promise().completion_state());
    }

    void await_resume() const noexcept {}
};

class IntTask {
public:
    struct promise_type {
        std::shared_ptr<IntTaskState> state = std::make_shared<IntTaskState>();

        IntTask get_return_object()
        {
            return IntTask{std::coroutine_handle<promise_type>::from_promise(*this), state};
        }

        std::suspend_never initial_suspend() const noexcept { return {}; }

        struct FinalNotifier {
            std::shared_ptr<IntTaskState> state;

            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<>) const noexcept
            {
                bool publish_done = false;
                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->final_reached = true;
                    // 同步完成（沒有外部 resume）可在此發布；worker resume 中則必須等
                    // Executor::execute 看見 resume 已返回後再發布。
                    if (state->active_resumes == 0U && !state->done) {
                        state->done = true;
                        publish_done = true;
                    }
                }
                if (publish_done) state->condition.notify_all();
            }

            void await_resume() const noexcept {}
        };

        FinalNotifier final_suspend() const noexcept { return {state}; }
        void return_value(int result) const noexcept { state->value = result; }
        void unhandled_exception() const noexcept { state->error = std::current_exception(); }

        std::shared_ptr<IntTaskState> completion_state() const noexcept { return state; }
    };

    IntTask(IntTask&& other) noexcept
        : handle_(std::exchange(other.handle_, {})), state_(std::move(other.state_))
    {
    }

    IntTask(const IntTask&) = delete;
    IntTask& operator=(const IntTask&) = delete;
    IntTask& operator=(IntTask&&) = delete;

    ~IntTask()
    {
        if (handle_) {
            wait();
            handle_.destroy();
        }
    }

    int get()
    {
        wait();
        std::exception_ptr error;
        int result = 0;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            error = state_->error;
            result = state_->value;
        }
        if (error) std::rethrow_exception(error);
        return result;
    }

private:
    IntTask(std::coroutine_handle<promise_type> handle, std::shared_ptr<IntTaskState> state)
        : handle_(handle), state_(std::move(state))
    {
    }

    void wait() const
    {
        Executor* executor = nullptr;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->done) return;
            executor = state_->executor;
        }

        if (executor != nullptr && executor->runs_in_this_thread()) {
            executor->help_until(state_);
            return;
        }

        std::unique_lock<std::mutex> lock(state_->mutex);
        state_->condition.wait(lock, [this] { return state_->done; });
    }

    std::coroutine_handle<promise_type> handle_;
    std::shared_ptr<IntTaskState> state_;
};

IntTask async_add(Executor& executor, int left, int right)
{
    co_await ScheduleOn{executor};  // 之後的程式在 executor worker 繼續。
    co_return left + right;
}

IntTask add_from_worker(Executor& executor)
{
    co_await ScheduleOn{executor};
    IntTask child = async_add(executor, 19, 23);
    co_return child.get();  // worker 會 help queue，不會睡著等待自己。
}

void basic_demo()
{
    Executor executor;
    IntTask task = async_add(executor, 20, 22);
    expect(task.get() == 42, "coroutine result mismatch");
}

// ----------------------------------------------------------------------------
// LeetCode 2235：Add Two Integers
// ----------------------------------------------------------------------------
void leetcode_demo()
{
    Executor executor;
    const int positive = async_add(executor, 12, 5).get();
    const int mixed = async_add(executor, -10, 4).get();
    expect(positive == 17 && mixed == -6, "LeetCode addition result mismatch");
}

// ----------------------------------------------------------------------------
// 實務：blocking bridge、worker nested wait，以及 frame 立即回收壓力測試
// ----------------------------------------------------------------------------
void practical_demo()
{
    Executor executor;
    IntTask parse = async_add(executor, 100, 20);
    IntTask validate = async_add(executor, 7, 8);
    expect(parse.get() == 120, "parse task result mismatch");
    expect(validate.get() == 15, "validation task result mismatch");
    expect(add_from_worker(executor).get() == 42, "worker queue helping failed");

    // get 返回後 temporary 立即 destroy frame；重複執行可讓 sanitizer/TSan 壓到完成發布邊界。
    for (int value = 0; value < 500; ++value) {
        expect(async_add(executor, value, 1).get() == value + 1,
               "frame publication stress test failed");
    }
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "coroutine executor：schedule、resume、nested wait 與 frame 回收測試通過\n";
}

// 【陷阱】coroutine_handle 是 non-owning；destroy 後再 resume/destroy 是 UB。
// 【陷阱】final_suspend 的通知不能早於外部 resume 返回，否則 owner 可能過早 destroy frame。
// 【陷阱】executor 必須活得比所有 pending IntTask 久；State 內的 executor pointer 不擁有它。
// 【面試】coroutine 解決 control-flow suspension，不自動提供 parallelism 或 thread safety。
// 【練習】把 IntTask 擴成 template<T>，並以 continuation 取代 reentrant queue helping。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '20_coroutines_pool.cpp' -o '/tmp/codex_cpp_C_MultiThread_20_coroutines_pool' && '/tmp/codex_cpp_C_MultiThread_20_coroutines_pool'
//
// === 預期輸出（節錄）===
// coroutine executor：schedule、resume、nested wait 與 frame 回收測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
