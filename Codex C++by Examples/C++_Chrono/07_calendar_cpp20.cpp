// ============================================================================
// 課題 7：C++20 Calendar - year_month_day、sys_days
// ============================================================================
//
// C++20 calendar types 能以年/月/日建模，不再手刻 leap-year/day-count。year_month_day 是
// civil fields；轉成 sys_days 後可排序/相減。建構後要 `.ok()`，因 2025-02-31 仍可被
// 表示但不是有效日期。
//
// timezone (`zoned_time`, tzdb) 的 library/platform 支援可能落後；本檔只用 calendar+
// sys_days，GCC/Clang+libstdc++ 可攜性較高。日期不等於 24 小時 duration：DST 地區一天
// 可能 23/25 小時，排程要先決定 civil-time 或 elapsed-time 語意。
// ============================================================================

#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std::chrono;

year_month_day parse_date(const std::string& text)
{
    if (text.size() != 10U || text.at(4) != '-' || text.at(7) != '-') {
        throw std::invalid_argument("date must be YYYY-MM-DD");
    }
    const year_month_day date{year{std::stoi(text.substr(0U, 4U))},
                              month{static_cast<unsigned>(std::stoi(text.substr(5U, 2U)))},
                              day{static_cast<unsigned>(std::stoi(text.substr(8U, 2U)))}};
    if (!date.ok()) throw std::invalid_argument("invalid calendar date");
    return date;
}

void basic_example()
{
    const year_month_day leap_day{year{2024}, month{2}, day{29}};
    const year_month_day invalid{year{2023}, month{2}, day{29}};
    assert(leap_day.ok() && !invalid.ok());
    const year_month_day next = year_month_day{sys_days{leap_day} + days{1}};
    assert(next == year_month_day(year{2024}, month{3}, day{1}));
    std::cout << "[基礎] 2024-02-29 + 1 day = 2024-03-01\n";
}

// LeetCode 1360：Number of Days Between Two Dates。
int days_between_dates(const std::string& first, const std::string& second)
{
    const sys_days left{parse_date(first)};
    const sys_days right{parse_date(second)};
    const days difference = right >= left ? right - left : left - right;
    return static_cast<int>(difference.count());
}

void leetcode_1360_example()
{
    assert(days_between_dates("2019-06-29", "2019-06-30") == 1);
    assert(days_between_dates("2020-01-15", "2019-12-31") == 15);
    std::cout << "[LeetCode 1360] date gaps=1 and 15 days\n";
}

// 實務：月底 billing。year_month_day + months 對 1/31 + 1 month 可能產生 invalid 2/31；
// policy 必須明定。此函式選「clamp 到目標月份最後一天」。
year_month_day add_month_clamped(year_month_day date)
{
    const year_month target = date.year() / date.month() + months{1};
    const year_month_day candidate = target / date.day();
    if (candidate.ok()) return candidate;
    return year_month_day{target / last};
}

void practical_example()
{
    const auto result = add_month_clamped(year{2025} / January / day{31});
    assert(result == year_month_day(year{2025}, February, day{28}));
    std::cout << "[實務] monthly billing clamps Jan 31 -> Feb 28\n";
}

int main()
{
    basic_example();
    leetcode_1360_example();
    practical_example();
}

// 易錯與面試：year_month_day 可表示 invalid date，建構後要 `.ok()`；calendar date 不含
// timezone/DST。帳務「加一月」沒有唯一答案，clamp、reject、roll-over 都須寫成政策。
// 練習：測試 leap year 的 Jan 31 -> Feb 29，以及 Dec -> next January。
// 複雜度與生命週期：固定日期的 calendar/sys_days 轉換是 O(1) 值運算；year_month_day 可
// 獨立保存但可能是 invalid value，所以跨 API 邊界仍要以 ok() 驗證。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_calendar_cpp20.cpp' -o '/tmp/codex_cpp_C_Chrono_07_calendar_cpp20' && '/tmp/codex_cpp_C_Chrono_07_calendar_cpp20'
//
// === 預期輸出（節錄）===
// [實務] monthly billing clamps Jan 31 -> Feb 28
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
