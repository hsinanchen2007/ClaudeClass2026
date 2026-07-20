// ============================================================================
// condition_variable：等待「狀態成立」，不是等待 notification 本身
// ============================================================================
// condition_variable 搭配 unique_lock<mutex>。正確模式永遠是：
//   cv.wait(lock, predicate);
// predicate 在持鎖下讀共享狀態；wait 會原子地 unlock+睡眠，醒來後重新 lock 並
// 重查 predicate。這處理 spurious wakeup，也避免 notification 早於 wait 時遺失狀態。
// 更新狀態後可先 unlock 再 notify，減少被喚醒者立刻卡在同一 mutex。

#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

void basic_demo()
{
    std::mutex mutex;
    std::condition_variable condition;
    bool ready = false;
    int value = 0;
    std::thread consumer([&] {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [&] { return ready; });
        assert(value == 42);
    });
    {
        std::lock_guard<std::mutex> lock(mutex);
        value = 42;
        ready = true;
    }
    condition.notify_one();
    consumer.join();
}

// ----------------------------------------------------------------------------
// LeetCode 1115：Print FooBar Alternately
// ----------------------------------------------------------------------------
class FooBar {
public:
    explicit FooBar(int repetitions) : repetitions_(repetitions) {}

    void foo()
    {
        for (int i = 0; i < repetitions_; ++i) {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] { return foo_turn_; });
            output_ += "foo";
            foo_turn_ = false;
            lock.unlock();
            condition_.notify_one();
        }
    }

    void bar()
    {
        for (int i = 0; i < repetitions_; ++i) {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] { return !foo_turn_; });
            output_ += "bar";
            foo_turn_ = true;
            lock.unlock();
            condition_.notify_one();
        }
    }

    std::string result() const { return output_; }  // join 後呼叫，無同時寫入。

private:
    int repetitions_;
    bool foo_turn_ = true;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::string output_;
};

void leetcode_demo()
{
    FooBar value(3);
    std::thread bar([&] { value.bar(); });
    std::thread foo([&] { value.foo(); });
    foo.join();
    bar.join();
    assert(value.result() == "foobarfoobarfoobar");
}

// ----------------------------------------------------------------------------
// 實務：可關閉 blocking queue
// ----------------------------------------------------------------------------
class BlockingQueue {
public:
    void push(int value)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            assert(!closed_);
            values_.push(value);
        }
        condition_.notify_one();
    }

    bool pop(int& value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return closed_ || !values_.empty(); });
        if (values_.empty()) {
            return false;  // closed 且排空。
        }
        value = values_.front();
        values_.pop();
        return true;
    }

    void close()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        condition_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<int> values_;
    bool closed_ = false;
};

void practical_demo()
{
    BlockingQueue queue;
    int sum = 0;
    std::thread consumer([&] {
        int value = 0;
        while (queue.pop(value)) {
            sum += value;
        }
    });
    queue.push(10);
    queue.push(20);
    queue.close();
    consumer.join();
    assert(sum == 30);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "condition_variable：predicate、FooBar 與 blocking queue 測試通過\n";
}

// 【陷阱】只寫 if (!ready) cv.wait(lock) 會被 spurious wakeup 擊穿；用 predicate/while。
// 【陷阱】只 notify 不保存 state，早到的 notification 不會排隊等待未來 waiter。
// 【面試】為何 wait 需要 unique_lock 而非 lock_guard？必須暫時 unlock/relock。
// 【練習】為 BlockingQueue 加容量上限與 not_full condition。

/*
 * 【教科書補充：BlockingQueue 的關閉契約】
 * - push 與 close 在同一 mutex 下線性化：先取得鎖者決定該筆資料被接受或拒絕。
 * - 本例 close 後 push 只以 assert 阻擋；release 會失去檢查，production 應回 bool 或丟例外。
 * - close 應定義為冪等、喚醒所有 waiter，consumer 則 drain 已接受項目後回 false。
 * - 解構 condition/mutex/queue 前，所有 producer/consumer 必須停止並 join；仍在 wait 的 thread 會造成 UB。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_condition_variable.cpp' -o '/tmp/codex_cpp_C_MultiThread_05_condition_variable' && '/tmp/codex_cpp_C_MultiThread_05_condition_variable'
//
// === 預期輸出（節錄）===
// condition_variable：predicate、FooBar 與 blocking queue 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
