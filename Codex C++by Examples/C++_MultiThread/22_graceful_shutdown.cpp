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

// -----------------------------------------------------------------------------
// 【日常實務範例】接受工作後保證排空的背景處理器
// 情境：producer 與兩個 closer 同時競跑；submit 在線性化點成功的每一筆都必須平方並寫入結果，所有 close caller 返回時 worker 已 join。
// 為何使用本章主題：mutex/cv 定義接受與 drain 狀態，call_once 讓 concurrent close 冪等且只有一個 joiner，比 joinable check-then-join 安全。
// 設計：1. submit 鎖內接受或拒絕。2. worker 等 closed 或 queue 非空。3. close 設 closed、notify 並 join。4. worker 排空後才退出。
// 成本：每筆 queue/結果各有 mutex 操作；空間 O(Q+R)，close latency 包含所有已接受工作與 join。
// 上線注意：worker 計算/結果配置若丟例外不能逃出 thread entry；永久阻塞工作需 deadline/取消，平方也要先防 int overflow。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array（一維陣列的動態和）
// 題目：回傳每個 prefix 的累加值；例如 [1,2,3,4] 得 [1,3,6,10]。
// 為何使用本章主題：prefix 本身仍在 caller 順序計算；本教學另把每個 prefix 送入 DrainingWorker 做平方稽核，只為驗證 close 前工作全數 drain，不是平行 scan。
// 思路：1. 逐值更新 total。2. append 正規 prefix 結果。3. submit 同一 total 作背景 audit。4. close 並確認 audit 筆數後回傳。
// 複雜度：前景與背景總工作 O(N)、prefix/audit 結果空間 O(N)，另有每筆 queue 同步與最終 join。
// 易錯點：不能把 audit 平方結果誤當題目答案；total 與平方都可能 signed overflow，close 失敗/worker 例外也需回報。
// -----------------------------------------------------------------------------
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

/*
 * 【教科書補充：graceful 需要定義 failure，而不只是 drain】
 * - worker 內 sink/push_back 若拋例外且逃出 thread entry，程序會 terminate；需捕捉並發布錯誤/取消。
 * - 已接受工作若處理失敗，要定義 retry、dead-letter、部分結果與 close() 回報方式。
 * - 任務永久阻塞會讓 close/join 永久等待；production 要有 stop token、deadline 或可中斷 I/O。
 * - 教學中的 int 平方也需限制範圍，signed overflow 是 UB，catch 不到。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '22_graceful_shutdown.cpp' -o '/tmp/codex_cpp_C_MultiThread_22_graceful_shutdown' && '/tmp/codex_cpp_C_MultiThread_22_graceful_shutdown'
//
// === 預期輸出（節錄）===
// graceful shutdown：拒新件、drain 與 idempotent close 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
