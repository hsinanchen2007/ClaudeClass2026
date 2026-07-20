// ============================================================================
// 課題 8：實務 ScopeTimer 與可測量 instrumentation
// ============================================================================
//
// ScopeTimer 在 constructor 記 steady start、destructor 計算 elapsed 並送給 callback，
// 所有 return/exception 路徑都會記錄。destructor 必須 noexcept；callback 若可能 throw，
// 應捕捉或提供明確 stop()。Timer 自己的輸出 I/O 可能干擾很短 operation，microbenchmark
// 不應每輪直接印 log。
//
// 真實 tracing 常記 label、thread id、correlation id，批次送 metrics；本例以 callback
// 注入避免把 logger/global singleton 綁死。
// ============================================================================

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class ScopeTimer {
public:
    using Clock = std::chrono::steady_clock;
    using Callback = std::function<void(std::chrono::nanoseconds)>;

    explicit ScopeTimer(Callback callback)
        : callback_(std::move(callback)), start_(Clock::now()) {}

    ~ScopeTimer() noexcept
    {
        try {
            callback_(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start_));
        } catch (...) {
            // destructor 不讓 metrics failure 破壞主流程。
        }
    }

    ScopeTimer(const ScopeTimer&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;

private:
    Callback callback_;
    Clock::time_point start_;
};

void basic_example()
{
    std::chrono::nanoseconds observed{-1};
    {
        ScopeTimer timer([&](std::chrono::nanoseconds elapsed) { observed = elapsed; });
        int value = 1 + 1;
        assert(value == 2);
    }
    assert(observed.count() >= 0);
    std::cout << "[基礎] RAII timer observed " << observed.count() << " ns\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 70. Climbing Stairs（爬樓梯）
// 題目：每次可走 1 或 2 階，輸入 steps 後求抵達頂端的方法數；例如 20 階有 10946 種。
// 為何使用本章主題：chrono 不參與最佳解；本例只是用 ScopeTimer 包住迭代 DP，示範不改演算法即可收集整個 scope 的耗時。
// 思路：1. 以 previous/current 保存相鄰 Fibonacci 狀態。2. 每階更新 next。3. 回傳 current。4. scope 結束由 RAII callback 記錄時間。
// 複雜度：演算法時間 O(N)、額外空間 O(1)，N 為階數；計時另增加兩次 clock 讀取與一次 callback。
// 易錯點：steps=0 的定義是 1 種；計時值受 optimizer 與系統負載影響，不能用單次 elapsed 判斷演算法正確性。
// -----------------------------------------------------------------------------
int climb_stairs(int steps)
{
    int previous = 0;
    int current = 1;
    for (int step = 1; step <= steps; ++step) {
        const int next = previous + current;
        previous = current;
        current = next;
    }
    return current;
}

void leetcode_70_example()
{
    std::chrono::nanoseconds elapsed{0};
    int answer = 0;
    {
        ScopeTimer timer([&](std::chrono::nanoseconds value) { elapsed = value; });
        answer = climb_stairs(20);
    }
    assert(answer == 10'946 && elapsed.count() >= 0);
    std::cout << "[LeetCode 70] answer=10946, instrumentation=" << elapsed.count() << " ns\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】批次工作延遲樣本收集
// 情境：依序處理三個 job，無論正常 return 或丟例外，都要在各 job scope 結束時留下一筆 duration。
// 為何使用本章主題：RAII ScopeTimer 自動涵蓋所有離開 scope 的路徑；callback 注入比在計時器內綁死 logger 更容易替換 metrics backend。
// 設計：1. 每個 job 建立 timer。2. constructor 記 steady start。3. destructor 計算 elapsed。4. callback 將樣本加入 vector。
// 成本：每個 job 有兩次 now()、std::function 呼叫與 vector append；細粒度工作可能被 instrumentation 成本主導。
// 上線注意：callback 捕獲的 samples 必須比 timer 活得久；destructor 必須吞下 metrics 例外，並考慮多執行緒 backend 的同步。
// -----------------------------------------------------------------------------
void practical_example()
{
    std::vector<std::chrono::nanoseconds> samples;
    for (int job = 0; job < 3; ++job) {
        ScopeTimer timer([&](std::chrono::nanoseconds elapsed) { samples.push_back(elapsed); });
        assert(job >= 0);
    }
    assert(samples.size() == 3U);
    std::cout << "[實務] scope timer collected 3 job samples\n";
}

int main()
{
    basic_example();
    leetcode_70_example();
    practical_example();
}

// 易錯與面試：RAII timer destructor 不應讓 callback exception 逸出而導致 terminate；
// benchmark 要用 steady_clock、warm-up、多次樣本，並防止 compiler 消除待測工作。
// 練習：加入 stop()，保證 callback 最多一次，並測試 move semantics。
// 複雜度：timer 本體 start/stop O(1)，callback 成本另計；大量細粒度 scope 可能反客為主。
// 生命週期：RAII timer 在 scope exit 執行 callback；callback 捕獲的 reference 必須仍存活。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_practical_timer.cpp' -o '/tmp/codex_cpp_C_Chrono_08_practical_timer' && '/tmp/codex_cpp_C_Chrono_08_practical_timer'
//
// === 預期輸出（節錄）===
// [實務] scope timer collected 3 job samples
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
