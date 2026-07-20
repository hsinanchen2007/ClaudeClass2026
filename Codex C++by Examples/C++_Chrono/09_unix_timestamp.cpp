// ============================================================================
// 課題 9：Unix timestamp、system_clock 與 serialization
// ============================================================================
//
// Unix timestamp 通常指 1970-01-01T00:00:00Z 起的秒數，但 C++ standard 對舊版本
// system_clock epoch 沒完全寫死；現代主流實作與 C++20 sys_time 以 Unix epoch 操作。
// 明定 unit、signed width、UTC；不要用裸 int（2038 問題）。負 timestamp 表 epoch 前。
//
// `duration_cast<seconds>` 對負值 toward zero；若要時間桶通常應 floor<seconds>，否則
// epoch 前 -0.5s 會錯分到 0。wall timestamp 可持久化，steady timestamp 不可。
// ============================================================================

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

std::int64_t unix_seconds(std::chrono::system_clock::time_point point)
{
    return std::chrono::floor<std::chrono::seconds>(point.time_since_epoch()).count();
}

std::chrono::system_clock::time_point from_unix_seconds(std::int64_t seconds)
{
    return std::chrono::system_clock::time_point{std::chrono::seconds{seconds}};
}

void basic_example()
{
    const auto epoch = from_unix_seconds(0);
    assert(unix_seconds(epoch) == 0);
    assert(unix_seconds(epoch + std::chrono::milliseconds{1'500}) == 1);
    assert(unix_seconds(epoch - std::chrono::milliseconds{500}) == -1); // floor，不是 0。
    std::cout << "[基礎] epoch=0, +1.5s=1, -0.5s floors to -1\n";
}

// LeetCode 1154：Day of the Year。以 C++20 calendar 算該日與當年 1/1 的差 +1。
int day_of_year(const std::string& text)
{
    if (text.size() != 10U) throw std::invalid_argument("YYYY-MM-DD required");
    using namespace std::chrono;
    const year_month_day date{year{std::stoi(text.substr(0U, 4U))},
        month{static_cast<unsigned>(std::stoi(text.substr(5U, 2U)))},
        day{static_cast<unsigned>(std::stoi(text.substr(8U, 2U)))}};
    if (!date.ok()) throw std::invalid_argument("invalid date");
    const sys_days first_day{date.year() / January / day{1}};
    return static_cast<int>((sys_days{date} - first_day).count()) + 1;
}

void leetcode_1154_example()
{
    assert(day_of_year("2019-01-09") == 9);
    assert(day_of_year("2019-02-10") == 41);
    assert(day_of_year("2000-12-31") == 366);
    std::cout << "[LeetCode 1154] day numbers 9,41,366\n";
}

// 實務：API record 明列欄位名稱 `_unix_seconds`，避免 consumer 猜 milliseconds。
struct AuditRecord {
    std::int64_t created_unix_seconds;
};

void practical_example()
{
    const AuditRecord record{unix_seconds(from_unix_seconds(1'700'000'000LL))};
    assert(record.created_unix_seconds == 1'700'000'000LL);
    std::cout << "[實務] serialized signed 64-bit unix seconds=1700000000\n";
}

int main()
{
    basic_example();
    leetcode_1154_example();
    practical_example();
}

// 複雜度：time_point/duration 的單位轉換是固定大小 arithmetic，通常 O(1)，不查時區資料庫。
// 生命週期：序列化的是值，不是 clock object/reference；steady_clock time_point 不可跨程序保存。
// 易錯與面試：timestamp 不含時區名稱，顯示成 local civil time 時仍需要明確 timezone policy。
// 練習：定義 milliseconds wire format，測試負 timestamp 與 int64 range。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_unix_timestamp.cpp' -o '/tmp/codex_cpp_C_Chrono_09_unix_timestamp' && '/tmp/codex_cpp_C_Chrono_09_unix_timestamp'
//
// === 預期輸出（節錄）===
// [實務] serialized signed 64-bit unix seconds=1700000000
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
