// ============================================================================
// 多執行緒面試題：先說 invariant/progress，再寫 synchronization
// ============================================================================
// 面試答題順序：
// 1. 誰擁有資料、哪些 access 會同時發生？2. invariant/linearization point 是什麼？
// 3. blocking 或 lock-free？滿/空/取消/shutdown 怎麼辦？4. happens-before 在哪裡？
// 5. 例外與生命週期？6. 如何以壓力測試、TSan、timeout 驗證？
//
// 常見錯答：用 sleep 排序、用 volatile 修 race、只 notify 不存 state、持鎖 callback、
// 忘記 join、把 lock-free 說成 wait-free、宣稱測試跑過就證明沒有 race。

#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <semaphore>
#include <string>
#include <thread>

// ----------------------------------------------------------------------------
// 基礎題：thread-safe counter，linearization point 是持鎖下的 ++value_
// ----------------------------------------------------------------------------
class Counter {
public:
    void increment()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        ++value_;
    }
    int value() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }

private:
    mutable std::mutex mutex_;
    int value_ = 0;
};

void basic_demo()
{
    Counter counter;
    std::thread a([&] { for (int i = 0; i < 1'000; ++i) counter.increment(); });
    std::thread b([&] { for (int i = 0; i < 1'000; ++i) counter.increment(); });
    a.join();
    b.join();
    assert(counter.value() == 2'000);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1116. Print Zero Even Odd（列印零與奇偶數）
// 題目：三個 thread 分別印 0、偶數、奇數，輸出須為 010203...0n；本例 n=5 得 0102030405。
// 為何使用本章主題：三個 binary_semaphore 將 turn 編碼成 permit，早於 waiter 的 release 仍保留狀態，且每次只允許一個 output writer。
// 思路：1. zero 取得 permit 並印 0。2. 依下一數奇偶釋放對應 worker。3. odd/even 印數字。4. 再釋放 zero permit。
// 複雜度：共 2N 次輸出與同步，時間/輸出空間 O(N)，控制空間 O(1)。
// 易錯點：本例以 '0'+number 拼字元，只可靠支援 0..9，未完整涵蓋原題較大 n；任一 worker 缺席都會永久等待。
// -----------------------------------------------------------------------------
class ZeroEvenOdd {
public:
    explicit ZeroEvenOdd(int limit) : limit_(limit) {}

    void zero()
    {
        for (int number = 1; number <= limit_; ++number) {
            zero_turn_.acquire();
            output_ += '0';
            if (number % 2 == 0) even_turn_.release();
            else odd_turn_.release();
        }
    }

    void even()
    {
        for (int number = 2; number <= limit_; number += 2) {
            even_turn_.acquire();
            output_ += static_cast<char>('0' + number);
            zero_turn_.release();
        }
    }

    void odd()
    {
        for (int number = 1; number <= limit_; number += 2) {
            odd_turn_.acquire();
            output_ += static_cast<char>('0' + number);
            zero_turn_.release();
        }
    }

    const std::string& result() const { return output_; }

private:
    int limit_;
    std::binary_semaphore zero_turn_{1};
    std::binary_semaphore even_turn_{0};
    std::binary_semaphore odd_turn_{0};
    std::string output_;
};

void leetcode_demo()
{
    ZeroEvenOdd value(5);
    std::thread even([&] { value.even(); });
    std::thread odd([&] { value.odd(); });
    std::thread zero([&] { value.zero(); });
    zero.join();
    odd.join();
    even.join();
    assert(value.result() == "0102030405");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定發布的一次性啟動閘門
// 情境：consumer 啟動後必須等 config=42 發布才能繼續；publish 也可能早於 wait，不能靠一次 notification 當事實。
// 為何使用本章主題：mutex 同時保護 config/ready，condition_variable 等持久 predicate，避免 busy wait 並處理早到通知與 spurious wakeup。
// 設計：1. waiter 鎖住 state。2. wait 到 ready。3. publisher 鎖內寫 config 再設 ready。4. 解鎖後 notify_all。
// 成本：publish 與完成後 wait 為 O(1)，等待期間不持有 CPU；同步成本取決於 waiter 數量。
// 上線注意：目前允許重複 publish 覆寫值，若契約是一次性應拒絕第二次；析構前所有 waiter 必須被喚醒並 join。
// -----------------------------------------------------------------------------
class StartupGate {
public:
    void publish(int config)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            config_ = config;
            ready_ = true;
        }
        condition_.notify_all();
    }

    int wait() const
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return ready_; });
        return config_;
    }

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable condition_;
    bool ready_ = false;
    int config_ = 0;
};

void practical_demo()
{
    StartupGate gate;
    int observed = 0;
    std::thread consumer([&] { observed = gate.wait(); });
    gate.publish(42);
    consumer.join();
    assert(observed == 42);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "interview problems：counter、ZeroEvenOdd、startup gate 測試通過\n";
}

// 【面試 Q&A】
// Q: mutex 與 atomic 怎麼選？A: 單一簡單 state/RMW 可 atomic；多欄 invariant 用 mutex。
// Q: lock-free 等於快？A: 否，CAS contention/cache traffic/reclamation 都可能更慢。
// Q: 如何測 deadlock？A: timeout+stress 能發現但不能證明不存在；靠 lock hierarchy/design。
// Q: condition_variable 為何要 predicate？A: state 才是事實，notification 可早到/假醒。
// 【陷阱】用 char('0'+number) 只適用 0..9；本題 limit=5，泛化輸出應用 to_string。
// 【練習】實作 LeetCode 1195 Fizz Buzz Multithreaded，明確定義四 worker predicate。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '30_interview_problems.cpp' -o '/tmp/codex_cpp_C_MultiThread_30_interview_problems' && '/tmp/codex_cpp_C_MultiThread_30_interview_problems'
//
// === 預期輸出（節錄）===
// interview problems：counter、ZeroEvenOdd、startup gate 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
