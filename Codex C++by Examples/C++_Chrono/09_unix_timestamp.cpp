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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1154. Day of the Year（一年中的第幾天）
// 題目：輸入 YYYY-MM-DD，回傳該日期在當年的序號；例如 2019-02-10 是第 41 天，2000-12-31 是第 366 天。
// 為何使用本章主題：C++20 calendar 將日期轉為 sys_days，再與當年 1 月 1 日相減，自動涵蓋月份長度與閏年。
// 思路：1. 解析 year/month/day。2. 用 ok() 驗證。3. 建立同年首日 sys_days。4. 日期差加 1 後回傳。
// 複雜度：固定長度日期解析與 calendar 算術皆為 O(1)，額外空間 O(1)。
// 易錯點：序號從 1 開始所以差值要加 1；stoi 前仍應完整檢查分隔符與數字，且不可把無效日期直接轉換。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】稽核紀錄的 Unix 秒序列化
// 情境：稽核事件要以可跨程序保存的 signed 64-bit Unix 秒寫入 API record，並能由時間點往返轉換。
// 為何使用本章主題：system_clock time_point 可映射 wall timestamp；明列 `_unix_seconds` 比裸 count 或 steady_clock 值更適合互通。
// 設計：1. 外部秒數轉 system_clock::time_point。2. 對 epoch duration 做 floor<seconds>。3. 寫入具單位名稱的 int64 欄位。
// 成本：轉換是 O(1) 算術與一個 64-bit 欄位，未涉及時區資料庫或 I/O。
// 上線注意：協定需固定 UTC、秒單位與合法範圍；epoch 前時間必須 floor 而非 toward-zero cast，顯示本地時間時另處理時區。
// -----------------------------------------------------------------------------
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
