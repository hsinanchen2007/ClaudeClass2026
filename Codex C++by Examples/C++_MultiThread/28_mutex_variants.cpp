// ============================================================================
// Mutex 家族與 lock wrapper：按語意選，不按名稱堆功能
// ============================================================================
// mutex：基本獨占；timed_mutex：try_lock_for/until；recursive_mutex：同 thread 可重入；
// shared_mutex：shared readers/exclusive writer。recursive 常掩蓋設計問題，timed lock
// 逾時也不修正 invariant，只提供 fail/退避路徑。
//
// lock_guard：最小 scope RAII；unique_lock：可 defer/unlock/transfer，供 cv 使用；
// scoped_lock：一次鎖多把、避免 AB/BA；adopt_lock 表示「已持鎖」且用錯就是 UB。

#include <array>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

void recursive_increment(std::recursive_mutex& mutex, int depth, int& count)
{
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    ++count;
    if (depth > 0) recursive_increment(mutex, depth - 1, count);
}

void basic_demo()
{
    std::recursive_mutex recursive;
    int count = 0;
    recursive_increment(recursive, 2, count);
    assert(count == 3);

    std::timed_mutex timed;
    timed.lock();
    bool acquired = true;
    std::thread contender([&] { acquired = timed.try_lock(); });
    contender.join();
    timed.unlock();
    assert(!acquired);
}

// ----------------------------------------------------------------------------
// LeetCode 1114：unique_lock + condition_variable
// ----------------------------------------------------------------------------
class Ordered {
public:
    void first() { advance(0, "first"); }
    void second() { advance(1, "second"); }
    void third() { advance(2, "third"); }
    std::string result() const { return output_; }

private:
    void advance(int expected, const std::string& text)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [&] { return stage_ == expected; });
        output_ += text;
        ++stage_;
        lock.unlock();
        condition_.notify_all();
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    int stage_ = 0;
    std::string output_;
};

void leetcode_demo()
{
    Ordered ordered;
    std::thread c([&] { ordered.third(); });
    std::thread b([&] { ordered.second(); });
    std::thread a([&] { ordered.first(); });
    a.join();
    b.join();
    c.join();
    assert(ordered.result() == "firstsecondthird");
}

// ----------------------------------------------------------------------------
// 實務：striped counters，每個 key 對應一把 mutex，降低無關 key contention
// ----------------------------------------------------------------------------
class StripedCounters {
public:
    void increment(std::size_t key)
    {
        const std::size_t stripe = key % values_.size();
        const std::lock_guard<std::mutex> lock(mutexes_[stripe]);
        ++values_[stripe];
    }

    int value(std::size_t stripe) const
    {
        const std::lock_guard<std::mutex> lock(mutexes_.at(stripe));
        return values_.at(stripe);
    }

private:
    mutable std::array<std::mutex, 4> mutexes_;
    std::array<int, 4> values_{};
};

void practical_demo()
{
    StripedCounters counters;
    std::thread a([&] { for (int i = 0; i < 100; ++i) counters.increment(0U); });
    std::thread b([&] { for (int i = 0; i < 100; ++i) counters.increment(1U); });
    a.join();
    b.join();
    assert(counters.value(0U) == 100 && counters.value(1U) == 100);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "mutex variants：recursive/timed/unique/striped 測試通過\n";
}

// 【陷阱】try_lock 失敗後不可 unlock；只有成功取得者可 unlock。
// 【陷阱】recursive_mutex 讓深層 API 隱藏 lock dependency，難測且可能讓 critical section 過大。
// 【面試】unique_lock 比 lock_guard 多了什麼成本/能力？狀態、可移動、defer、unlock、cv。
// 【練習】用 defer_lock + std::lock 安全取得兩個 unique_lock。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '28_mutex_variants.cpp' -o '/tmp/codex_cpp_C_MultiThread_28_mutex_variants' && '/tmp/codex_cpp_C_MultiThread_28_mutex_variants'
//
// === 預期輸出（節錄）===
// mutex variants：recursive/timed/unique/striped 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
