// ============================================================================
// mutex：用 mutual exclusion 保護「不變量」，不只是單一變數
// ============================================================================
// std::mutex 同一時間只允許一個擁有者。lock_guard 適合整個 scope 鎖住；
// unique_lock 可延後 lock/unlock、搭配 condition_variable；scoped_lock 可一次鎖多把
// mutex 並使用避免死鎖的演算法。mutex 的 unlock synchronizes-with 後續成功 lock，
// 使 critical section 內寫入對下一持鎖者可見。
//
// 鎖應保護 invariant，例如 balance 與 ledger 必須一致，而不是各自隨便一把鎖。
// critical section 要短；不要在持鎖時做未知 callback、網路 I/O 或長時間運算。

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void basic_demo()
{
    int counter = 0;
    std::mutex mutex;
    auto increment = [&] {
        for (int i = 0; i < 1'000; ++i) {
            const std::lock_guard<std::mutex> lock(mutex);
            ++counter;
        }
    };
    std::thread first(increment);
    std::thread second(increment);
    first.join();
    second.join();
    expect(counter == 2'000, "mutex counter mismatch");
}

// ----------------------------------------------------------------------------
// LeetCode 1114：Print in Order（mutex 保護 state 的教學版）
// ----------------------------------------------------------------------------
// 真題三函式可能由不同 thread 任意呼叫。這裡以 mutex+condition_variable 等待 stage；
// wait 使用 predicate，避免 spurious wakeup。每一步完成後更新 stage 再 notify_all。
#include <condition_variable>

class Foo {
public:
    void first(const std::string& text)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        output_ += text;
        stage_ = 1;
        condition_.notify_all();
    }

    void second(const std::string& text)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return stage_ >= 1; });
        output_ += text;
        stage_ = 2;
        lock.unlock();
        condition_.notify_all();
    }

    void third(const std::string& text)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return stage_ >= 2; });
        output_ += text;
    }

    std::string output() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return output_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    int stage_ = 0;
    std::string output_;
};

void leetcode_demo()
{
    Foo foo;
    std::thread third([&] { foo.third("third"); });
    std::thread second([&] { foo.second("second"); });
    std::thread first([&] { foo.first("first"); });
    first.join();
    second.join();
    third.join();
    expect(foo.output() == "firstsecondthird", "Foo execution order mismatch");
}

// ----------------------------------------------------------------------------
// 實務：兩帳戶轉帳，scoped_lock 同時鎖兩把且避免 AB/BA deadlock
// ----------------------------------------------------------------------------
struct Account {
    mutable std::mutex mutex;
    int balance;
};

bool transfer(Account& from, Account& to, int amount)
{
    // scoped_lock 的 mutex 參數必須代表不同 mutex。assert 不能當契約守衛：
    // -DNDEBUG 會移除它，之後把同一把 non-recursive mutex 傳入兩次可能永久等待。
    // 本 API 選擇把「自己轉給自己」視為無效請求並回 false，caller 可明確處理。
    if (&from == &to || amount < 0) return false;

    std::scoped_lock lock(from.mutex, to.mutex);
    if (from.balance < amount) return false;
    from.balance -= amount;
    to.balance += amount;
    return true;
}

void practical_demo()
{
    Account a{{}, 100};
    Account b{{}, 50};
    bool first_succeeded = false;
    bool second_succeeded = false;
    // 不可把 transfer 放進 assert：release build 會連函式呼叫本身一起刪除。
    std::thread one([&] { first_succeeded = transfer(a, b, 10); });
    std::thread two([&] { second_succeeded = transfer(b, a, 20); });
    one.join();
    two.join();
    expect(first_succeeded && second_succeeded, "valid transfer was rejected");

    // alias 路徑必須快速拒絕，而不是在 release build 自鎖。
    expect(!transfer(a, a, 1), "alias transfer must be rejected");
    {
        const std::scoped_lock lock(a.mutex, b.mutex);
        expect(a.balance + b.balance == 150, "transfer broke total-balance invariant");
    }
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "mutex：counter、ordered execution 與轉帳 invariant 測試通過\n";
}

// 【陷阱】手寫 mutex.lock(); ...; mutex.unlock(); 遇 exception 會漏 unlock；用 RAII。
// 【陷阱】assert 只供偵錯且可能整段消失；不可在 assert expression 內放必要副作用。
// 【陷阱】recursive_mutex 常掩蓋 ownership/design 問題，不是一般 deadlock 解法。
// 【面試】鎖保護的是什麼？答 invariant 與所有存取路徑，而非只說「一個 int」。
// 【練習】為 Account 加 snapshot()，確保讀 balance 也走同一 mutex。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_mutex.cpp' -o '/tmp/codex_cpp_C_MultiThread_03_mutex' && '/tmp/codex_cpp_C_MultiThread_03_mutex'
//
// === 預期輸出（節錄）===
// mutex：counter、ordered execution 與轉帳 invariant 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
