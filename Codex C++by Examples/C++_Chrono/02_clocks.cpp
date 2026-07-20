// ============================================================================
// 課題 2：system_clock、steady_clock、high_resolution_clock
// ============================================================================
//
// system_clock 對應 civil/wall clock，可轉 time_t、顯示日期；NTP/管理員調時會向前或
// 向後跳，因此不可可靠量 timeout。steady_clock 保證單調，最適合 elapsed/deadline。
// high_resolution_clock 只是「最短 tick period」的 alias，可能就是 system/steady；
// 不保證 steady，應看 `is_steady` 而非名字猜。
//
// time_point 只能在同 clock domain 比較。若系統同時需要「可讀 wall timestamp」與
//「可靠 elapsed」，事件中通常各記 system_clock::now() 與 steady_clock::now()。
// ============================================================================

#include <cassert>
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <unordered_map>

void basic_example()
{
    static_assert(std::chrono::steady_clock::is_steady);
    const auto monotonic_start = std::chrono::steady_clock::now();
    const auto wall_now = std::chrono::system_clock::now();
    const std::time_t wall_seconds = std::chrono::system_clock::to_time_t(wall_now);
    assert(wall_seconds > 0);
    const auto monotonic_end = std::chrono::steady_clock::now();
    assert(monotonic_end >= monotonic_start);
    std::cout << "[基礎] steady is monotonic; system time_t=" << wall_seconds << '\n';
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 359. Logger Rate Limiter（日誌速率限制器）
// 題目：依時間戳與訊息決定是否輸出；同一訊息每 10 秒最多一次，例如 foo 在 1 秒可印、2 秒拒絕、11 秒再允許。
// 為何使用本章主題：題目給的是邏輯秒數，使用 chrono::seconds 而非讀 system_clock，既保留單位又讓測試不受校時影響。
// 思路：1. 查訊息的 next-allowed 時間。2. 未到期限便拒絕。3. 允許時把期限更新為 timestamp+10s。
// 複雜度：unordered_map 平均每次時間 O(1)、空間 O(M)，M 為出現過的不同訊息數。
// 易錯點：timestamp 恰等於期限時必須允許；若用 system_clock 控制間隔，NTP 向後校時可能破壞限制語意。
// -----------------------------------------------------------------------------
class Logger {
public:
    bool should_print_message(std::chrono::seconds timestamp, const std::string& message)
    {
        const auto found = next_allowed_.find(message);
        if (found != next_allowed_.end() && timestamp < found->second) return false;
        next_allowed_[message] = timestamp + std::chrono::seconds(10);
        return true;
    }
private:
    std::unordered_map<std::string, std::chrono::seconds> next_allowed_;
};

void leetcode_359_example()
{
    Logger logger;
    assert(logger.should_print_message(std::chrono::seconds(1), "foo"));
    assert(!logger.should_print_message(std::chrono::seconds(2), "foo"));
    assert(logger.should_print_message(std::chrono::seconds(11), "foo"));
    std::cout << "[LeetCode 359] deterministic 10-second rate limit\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】同時記錄稽核時間與 SLA 起點
// 情境：服務事件既要在日誌顯示可讀的牆鐘時間，也要可靠計算兩事件間耗時。
// 為何使用本章主題：system_clock 可轉民用時間但可能跳動；steady_clock 單調，適合 elapsed，兩種語意不能由單一 clock 兼任。
// 設計：1. 事件發生時依序擷取 wall 與 monotonic。2. 將兩個 time_point 放入同一 EventStamp。3. 顯示與 SLA 各用對應欄位。
// 成本：每次事件有兩次 now() 呼叫與兩個 time_point 的儲存成本；實際延遲依平台 clock source 而定。
// 上線注意：兩次取樣並非同一瞬間，跨主機 monotonic 值也不可比較或持久化；分散式追蹤需另用 correlation 資訊。
// -----------------------------------------------------------------------------
struct EventStamp {
    std::chrono::system_clock::time_point wall;
    std::chrono::steady_clock::time_point monotonic;
};

EventStamp stamp_event()
{
    return {std::chrono::system_clock::now(), std::chrono::steady_clock::now()};
}

void practical_example()
{
    const EventStamp first = stamp_event();
    const EventStamp second = stamp_event();
    assert(second.monotonic >= first.monotonic);
    std::cout << "[實務] wall stamp for logs + steady stamp for elapsed\n";
}

int main()
{
    basic_example();
    leetcode_359_example();
    practical_example();
}

// 練習：印出三個 clock 的 period 與 is_steady，觀察本機 high_resolution_clock alias。
// 複雜度：now() 與 time_point subtraction 通常 O(1)，但實際 syscall/vDSO 成本依平台。
// 生命週期：time_point 是值 snapshot；system_clock 之後被校時不會回頭修改已取得的值。

/*
【本課面試問答】
Q1：三種標準 clock 如何選？
A：system_clock 對應民用/日曆時間，可轉 time_t 但會調整；steady_clock 保證不倒退，適合 elapsed/deadline；
high_resolution_clock 是實作提供的最高解析別名，不保證 steady，也不保證比前兩者更好。

Q2：系統時間向後跳會怎樣影響 rate limiter？
A：若用 system_clock 計間隔，request 可能被錯誤延後或提前；用 steady_clock 保存 monotonic deadline。
需要 audit log 時另外記 system_clock stamp，不要把展示時間與控制流程混成一個值。

Q3：`now()` 是不是免費的 O(1) 操作？
A：語意上為常數次操作，但實際可能走 vDSO、system call 或平台 clock source；解析度也不等於精確度。
熱路徑應 profile，不要每個元素都讀 clock 後再宣稱演算法成本只有 O(N)。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_clocks.cpp' -o '/tmp/codex_cpp_C_Chrono_02_clocks' && '/tmp/codex_cpp_C_Chrono_02_clocks'
//
// === 預期輸出（節錄）===
// [實務] wall stamp for logs + steady stamp for elapsed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
