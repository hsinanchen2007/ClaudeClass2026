// =============================================================================
//  07_calendar_cpp20.cpp  —  C++20 日期、年月日 (year_month_day)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/chrono#Calendar
//
//  本檔需要 C++20 支援。如果編譯器舊（沒有 <chrono> 日曆部分），會在
//  編譯期跳過示範並印「skipped」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、C++20 之前處理日期有多痛苦？                          │
//  └────────────────────────────────────────────────────────────┘
//
//  C++17 之前要操作日期，幾乎只有兩條路：
//   (1) C 風格 std::tm 配 mktime / strftime — 介面老、易踩坑
//   (2) 第三方 Howard Hinnant 的 date library
//
//  C++20 把 Howard Hinnant 的 date 設計合進 <chrono>，提供：
//   * year, month, day                      — 三個輕量型別
//   * year_month_day                        — 完整日期
//   * year_month_weekday                    — 「2026 年 5 月的第 1 個週一」
//   * weekday, last_spec, weekday_indexed   — 各種週幾組合
//   * sys_days / local_days                 — 把日期轉成 time_point
//   * <chrono> 同樣支援 1y、2d、3m... 字面量
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、最常用的幾個操作                                       │
//  └────────────────────────────────────────────────────────────┘
//
//   * 建立日期           year{2026}/May/5 → year_month_day
//   * 取今天             floor<days>(system_clock::now())
//   * 加減               ymd + months{1}, ymd + days{30}
//   * 取週幾             year_month_weekday{ymd}
//   * 月底日期           year{2026}/May/last → year_month_day_last
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：建立 ymd、加減月與日
//   * Demo 2：今天是星期幾？
//   * Demo 3：本月最後一天是幾號
// =============================================================================

/*
補充筆記：std::calendar_cpp20
  - std::calendar_cpp20 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - C++20 calendar 型別把 year/month/day 拆成強型別，能避免把月份和日期順序寫反。
  - year_month_day 可表示不存在日期，使用 ok() 檢查例如 2023y/February/31 是否有效。
  - 時區和日曆不是同一件事；只處理年月日不代表已處理 daylight saving time。
*/
#include <chrono>
#include <iostream>

#if __cplusplus >= 202002L
// 前置宣告：附加範例
static void demo_days_between();
static void demo_first_monday_of_month();
#endif

int main() {
#if __cplusplus < 202002L
    std::cout << "[skipped] need C++20 calendar support\n";
    return 0;
#else
    using namespace std::chrono;

    // ─────────────────────────────────────────────────────────
    // Demo 1：建構 ymd、做日期算術
    // ─────────────────────────────────────────────────────────
    year_month_day birthday{2026y, May, 5d};
    std::cout << "[Demo1] birthday        = "
              << static_cast<int>(birthday.year()) << '-'
              << static_cast<unsigned>(birthday.month()) << '-'
              << static_cast<unsigned>(birthday.day()) << '\n';

    // 加 1 個月（注意：月份算術可能落在無效日期，例：1/31 + 1 month = 2/31，
    // 此時 ok() 會回傳 false，需用 sys_days 重新轉一次或自行修正。)
    auto next_month = birthday + months{1};
    std::cout << "[Demo1] +1 month        = "
              << static_cast<int>(next_month.year()) << '-'
              << static_cast<unsigned>(next_month.month()) << '-'
              << static_cast<unsigned>(next_month.day())
              << " (ok=" << next_month.ok() << ")\n";

    // 加 30 天：要先轉成 sys_days、加完再轉回
    auto plus30 = sys_days{birthday} + days{30};
    year_month_day after30 = plus30;
    std::cout << "[Demo1] +30 days        = "
              << static_cast<int>(after30.year()) << '-'
              << static_cast<unsigned>(after30.month()) << '-'
              << static_cast<unsigned>(after30.day()) << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：星期幾
    // ─────────────────────────────────────────────────────────
    weekday wd{sys_days{birthday}};
    std::cout << "[Demo2] birthday is weekday index "
              << wd.c_encoding()    // 0=Sun, 1=Mon, ...
              << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：那個月的最後一天
    // ─────────────────────────────────────────────────────────
    year_month_day_last lastDay{2026y/May/last};
    year_month_day endOfMay{lastDay};
    std::cout << "[Demo3] last day of May 2026 = "
              << static_cast<int>(endOfMay.year()) << '-'
              << static_cast<unsigned>(endOfMay.month()) << '-'
              << static_cast<unsigned>(endOfMay.day()) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼日期型別要拆成 year / month / day 三個？
    //    A：分開後可以做型別安全的字面量（2026y vs 2026d 不會搞混），且
    //       「2026/5/5」這種表達在語法上更清楚。
    //
    //  Q2：時區？
    //    A：C++20 也加了 time_zone / zoned_time。但很多 stdlib 實作仍未完
    //       整支援；GCC libstdc++ 13+ / clang libc++ 16+ 較新版本才有。
    //
    //  Q3：std::format 的日期格式化？
    //    A：std::format("{:%Y-%m-%d}", year_month_day{...}) 直接搞定，比
    //       手動 cast 整數印好太多。
    demo_days_between();
    demo_first_monday_of_month();
#endif
    return 0;
}

#if __cplusplus >= 202002L
// =============================================================================
//  附加 1：實用範例 — 算兩個日期之間相差幾天
// =============================================================================
//  訂閱到期、活動倒數、會員年資 — 都需要「兩個 yyyy-mm-dd 相減」。
//  做法：先把兩個 year_month_day 轉成 sys_days（本質就是 days 為單位的
//  time_point），相減即得 days。
// =============================================================================
static void demo_days_between() {
    using namespace std::chrono;
    year_month_day start{2026y, January, 1d};
    year_month_day end_{2026y, May, 18d};
    auto diff = sys_days{end_} - sys_days{start};
    std::cout << "[days_between] 2026/1/1 -> 2026/5/18 = "
              << diff.count() << " days\n";
}

// =============================================================================
//  附加 2：實用範例 — 取「某月的第一個週一」(月例會排程常用)
// =============================================================================
//  公司排程「每月第一個週一開會」這類需求，C++20 calendar 用 weekday_indexed
//  一行搞定：year/month/Monday[1]。
// =============================================================================
static void demo_first_monday_of_month() {
    using namespace std::chrono;
    auto firstMon = 2026y / May / Monday[1];   // 2026 年 5 月的第 1 個週一
    year_month_day ymd{sys_days{firstMon}};
    std::cout << "[first_monday] 2026 May = "
              << static_cast<int>(ymd.year()) << '-'
              << static_cast<unsigned>(ymd.month()) << '-'
              << static_cast<unsigned>(ymd.day()) << '\n';
}
#endif
