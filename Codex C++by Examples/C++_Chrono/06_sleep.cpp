// ============================================================================
// 課題 6：std::this_thread::sleep_for / sleep_until
// ============================================================================
//
// sleep_for(relative duration) 與 sleep_until(absolute time point) 會讓目前 thread 暫停，
// 但 OS scheduler 只保證大致「不早於條件」恢復，可能晚很多；它不是 real-time timer。
// repeated periodic work 若每輪 sleep_for(period)，工作時間會累積 drift；用 steady
// `next += period; sleep_until(next)` 可維持 schedule（若落後仍需定義 catch-up policy）。
//
// sleep 無法被一般方式喚醒；需要 cancel/notify 時用 condition_variable::wait_for/until。
// unit tests 不應真的睡數秒，將 sleeper/clock 注入或用短時間且只驗邏輯。
// ============================================================================

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

void basic_example()
{
    const auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(1ms);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    // 不檢查「剛好 1ms」；scheduler 與 clock granularity 使 elapsed 通常更長。
    assert(elapsed >= std::chrono::steady_clock::duration::zero());
    std::cout << "[基礎] requested 1ms, actual "
              << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()
              << " us\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 359. Logger Rate Limiter（日誌速率限制器）
// 題目：依單調遞增 timestamp 判斷同一訊息是否距上次輸出至少 10 秒；例如 foo 在 1、2、11 秒依序為允許、拒絕、允許。
// 為何使用本章主題：這是反例式教學，解題應推進題目提供的邏輯時間，不應真的 sleep 10 秒，否則測試緩慢且不具決定性。
// 思路：1. 查訊息下一次可印時間。2. timestamp 未達門檻就拒絕。3. 允許時將新門檻設為 timestamp+10。
// 複雜度：unordered_map 平均單次時間 O(1)、空間 O(M)，M 為不同訊息數。
// 易錯點：等於門檻時可輸出；sleep 不是資料狀態，也不能取代每個 message 各自保存的期限。
// -----------------------------------------------------------------------------
class Logger {
public:
    bool should_print(int timestamp, const std::string& message)
    {
        const auto found = next_.find(message);
        if (found != next_.end() && timestamp < found->second) return false;
        next_[message] = timestamp + 10;
        return true;
    }
private:
    std::unordered_map<std::string, int> next_;
};

void leetcode_359_example()
{
    Logger logger;
    assert(logger.should_print(1, "foo"));
    assert(!logger.should_print(2, "foo"));
    assert(logger.should_print(11, "foo"));
    std::cout << "[LeetCode 359] tests simulate time; no 10-second sleep\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】避免漂移的週期工作期限表
// 情境：監控工作從固定起點每 100ms 執行一次，需要得到 100、200、300ms 的絕對期限，而不是把工作時間累加進週期。
// 為何使用本章主題：以 steady time_point 反覆加 period，可交給 sleep_until；相較每輪 sleep_for，更不會累積處理時間造成的 drift。
// 設計：1. 從 start 建立 next。2. 每輪加固定 period。3. 保存 absolute deadline。4. 呼叫端依期限等待或測試。
// 成本：產生 C 個期限需時間 O(C)、額外空間 O(C)；真正 sleep 會受 scheduler 延遲，本例未執行等待。
// 上線注意：需拒絕負 count/非正 period、檢查 time_point 溢位，並明訂工作落後時要補跑、跳過或施加 backpressure。
// -----------------------------------------------------------------------------
std::vector<std::chrono::steady_clock::time_point> schedule(
    std::chrono::steady_clock::time_point start,
    std::chrono::milliseconds period,
    int count)
{
    std::vector<std::chrono::steady_clock::time_point> deadlines;
    auto next = start;
    for (int index = 0; index < count; ++index) {
        next += period;
        deadlines.push_back(next);
    }
    return deadlines;
}

void practical_example()
{
    const auto origin = std::chrono::steady_clock::time_point{};
    const auto deadlines = schedule(origin, 100ms, 3);
    assert(deadlines.at(0) == origin + 100ms);
    assert(deadlines.at(2) == origin + 300ms);
    std::cout << "[實務] absolute periodic deadlines=100/200/300ms\n";
}

int main()
{
    basic_example();
    leetcode_359_example();
    practical_example();
}

// 易錯與面試：sleep_for/sleep_until 只保證「不早於」期限醒來，scheduler 可能延遲；
// sleep_for 週期 loop 會累積 drift，定期工作應從固定 origin 算下一個 absolute deadline。
// 練習：以 condition_variable 寫可由 stop flag 提前喚醒的 wait_for。
// 複雜度：安排一次 sleep 是 O(1) API 呼叫，但 wall latency 至少 requested duration 且可更久。
// 生命週期：sleep 不替 callback/object 保活；跨等待保存 raw this pointer 仍需 owner/cancellation 設計。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_sleep.cpp' -o '/tmp/codex_cpp_C_Chrono_06_sleep' && '/tmp/codex_cpp_C_Chrono_06_sleep'
//
// === 預期輸出（節錄）===
// [實務] absolute periodic deadlines=100/200/300ms
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
