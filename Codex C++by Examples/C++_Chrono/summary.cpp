// ============================================================================
// C++ <chrono> 總複習：型別化時間、正確計時、deadline 與 C++20 calendar
// ============================================================================
//
// 【本章地圖：對應 01～09】
//   overview / clocks / duration / time_point / benchmark / sleep /
//   C++20 calendar / practical timer / Unix timestamp
//
// 【三個核心型別】
//   duration<Rep,Period>：一段時間，例如 milliseconds；Rep 是數字，Period 是 tick 單位。
//   time_point<Clock,Duration>：某個 clock epoch 之後多少 duration。
//   Clock：定義 now()、epoch 與是否 monotonic；不同 clock 的 time_point 不可直接相減。
//
// 【clock 選型】
//   steady_clock：is_steady=true，不會倒退；elapsed、timeout、deadline、benchmark。
//   system_clock：可對應民用/Unix 時間；會被 NTP/管理員調整，不拿來量 elapsed。
//   high_resolution_clock：只是實作提供的 alias，未保證 steady；不要憑名字選它。
//
// 【duration 轉換規則】
//   seconds -> milliseconds 是精度增加，可隱式轉；milliseconds -> seconds 會丟資訊，
//   必須 duration_cast。若 policy 不是 toward-zero，C++17 起使用 floor/ceil/round。
//   `.count()` 只有配合原 duration 型別才有意義；API 優先直接收 duration。
//
// 【常見 API / 複雜度】
//   Clock::now()                         通常 O(1)，成本與 OS/clock source 有關
//   duration_cast/floor/ceil/round       O(1)
//   sleep_for(d)                         至少睡 d 左右，不承諾準時醒來
//   sleep_until(tp)                      適合週期排程，避免每輪工作時間累積 drift
//   sys_days <-> year_month_day          C++20 calendar，先用 `.ok()` 驗合法日期
//   system_clock::to_time_t/from_time_t  與 C time 互通，精度/範圍由實作決定
//
// 【benchmark 正確性清單】
//   1. 用 steady_clock；先 warm-up，再多次量測並看 median/percentiles。
//   2. 防止 dead-code elimination，但不要用 I/O 污染被測區間。
//   3. Release optimization、固定資料與環境；microbenchmark 優先專門框架。
//   4. 一次量測差異可能來自 turbo、scheduler、page fault、cache，不是演算法本身。
//
// 【deadline 與週期排程】
//   deadline = steady_clock::now() + timeout；每次比較 now >= deadline。
//   週期 task 用 `next += period; sleep_until(next)`，不是每輪 `sleep_for(period)`。
//   若工作已落後，要明訂 catch-up、skip 或 backpressure policy。
//
// 【C++20 calendar 與時區邊界】
//   year/month/day 是 calendar field；sys_days 是 UTC-like day point。
//   `2025y/February/29d` 可建構但 `.ok()==false`，不能假設 constructor 會 throw。
//   時區/DST 需要 chrono tzdb 且 library 支援度可能不同；持久化通常存 UTC timestamp
//   加原時區 ID，而不是只存本地顯示字串。
//
// 【面試快問快答】
//   Q: system_clock 為何可能算出負 elapsed？ A: wall clock 可被校時向後跳。
//   Q: sleep_for(10ms) 是否剛好 10ms？ A: 否，只承諾不早於請求時間（時鐘/中斷亦影響）。
//   Q: Unix timestamp 是什麼？ A: 慣例為 Unix epoch 起秒數；C++ standard 對歷史版本 epoch
//      保證有限，C++20 system_clock 已規範 Unix Time，但資料格式仍應寫明單位與範圍。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using namespace std::chrono_literals;

// time_point + duration 最終仍是底層 Rep 算術；超過範圍不會自動飽和。
// 這個 helper 先檢查 duration 轉換，再以不會自身溢位的邊界式檢查加法。
template <class Clock, class Rep, class Period>
typename Clock::time_point checked_add_time_point(
    typename Clock::time_point base,
    std::chrono::duration<Rep, Period> delta)
{
    using ClockDuration = typename Clock::duration;
    using ClockRep = typename ClockDuration::rep;
    static_assert(std::is_integral_v<ClockRep> && std::is_signed_v<ClockRep>,
                  "此教學 helper 專為 signed integral clock representation");

    const long double converted_ticks =
        std::chrono::duration<long double, typename ClockDuration::period>{delta}.count();
    const long double lowest =
        static_cast<long double>(std::numeric_limits<ClockRep>::lowest());
    const long double highest =
        static_cast<long double>(std::numeric_limits<ClockRep>::max());
    // 浮點精度不足的平台採保守拒絕邊界值，不讓 duration_cast 先溢位。
    if (!std::isfinite(converted_ticks) ||
        converted_ticks <= lowest || converted_ticks >= highest) {
        throw std::overflow_error("duration cannot be represented by clock");
    }

    const ClockDuration converted = std::chrono::duration_cast<ClockDuration>(delta);
    const ClockRep base_count = base.time_since_epoch().count();
    const ClockRep delta_count = converted.count();
    const ClockRep min_count = std::numeric_limits<ClockRep>::lowest();
    const ClockRep max_count = std::numeric_limits<ClockRep>::max();
    if ((delta_count > 0 && base_count > max_count - delta_count) ||
        (delta_count < 0 && base_count < min_count - delta_count)) {
        throw std::overflow_error("time point overflow");
    }
    return typename Clock::time_point{ClockDuration{base_count + delta_count}};
}

template <class Clock>
typename Clock::duration checked_time_point_difference(
    typename Clock::time_point left,
    typename Clock::time_point right)
{
    using Rep = typename Clock::duration::rep;
    static_assert(std::is_integral_v<Rep> && std::is_signed_v<Rep>);
    const Rep lhs = left.time_since_epoch().count();
    const Rep rhs = right.time_since_epoch().count();
    const Rep min_count = std::numeric_limits<Rep>::lowest();
    const Rep max_count = std::numeric_limits<Rep>::max();
    if ((rhs > 0 && lhs < min_count + rhs) ||
        (rhs < 0 && lhs > max_count + rhs)) {
        throw std::overflow_error("time point difference overflow");
    }
    return typename Clock::duration{lhs - rhs};
}

class Stopwatch {
public:
    using Clock = std::chrono::steady_clock;

    explicit Stopwatch(Clock::time_point start) : start_(start) {}

    [[nodiscard]] std::chrono::nanoseconds elapsed(Clock::time_point now) const
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            checked_time_point_difference<Clock>(now, start_));
    }

private:
    Clock::time_point start_;
};

class Deadline {
public:
    using Clock = std::chrono::steady_clock;

    Deadline(Clock::time_point now, Clock::duration timeout)
        : deadline_(make_deadline(now, timeout)) {}

private:
    static Clock::time_point make_deadline(Clock::time_point now,
                                           Clock::duration timeout) {
        if (timeout < Clock::duration::zero()) {
            throw std::invalid_argument("negative timeout");
        }
        return checked_add_time_point<Clock>(now, timeout);
    }

public:

    [[nodiscard]] bool expired(Clock::time_point now) const { return now >= deadline_; }

    [[nodiscard]] Clock::duration remaining(Clock::time_point now) const
    {
        return expired(now) ? Clock::duration::zero()
                            : checked_time_point_difference<Clock>(deadline_, now);
    }

private:
    Clock::time_point deadline_;
};

void basic_duration_and_calendar_demo()
{
    const auto precise = 1'550ms;
    assert(std::chrono::duration_cast<std::chrono::seconds>(precise) == 1s);
    assert(std::chrono::floor<std::chrono::seconds>(precise) == 1s);
    assert(std::chrono::ceil<std::chrono::seconds>(precise) == 2s);
    assert(std::chrono::round<std::chrono::seconds>(1'500ms) == 2s);
    assert(std::chrono::round<std::chrono::seconds>(2'500ms) == 2s); // ties-to-even

    const std::chrono::year_month_day leap_day{
        std::chrono::year{2024}, std::chrono::February, std::chrono::day{29}};
    const std::chrono::year_month_day invalid{
        std::chrono::year{2025}, std::chrono::February, std::chrono::day{29}};
    assert(leap_day.ok());
    assert(!invalid.ok());

    const auto origin = Deadline::Clock::time_point{};
    const Stopwatch stopwatch(origin);
    const Deadline deadline(origin, 5s);
    assert(stopwatch.elapsed(origin + 2ms) == 2ms);
    assert(deadline.remaining(origin + 2s) == 3s);
    assert(deadline.expired(origin + 5s));
    bool overflow_rejected = false;
    try {
        static_cast<void>(Deadline(Deadline::Clock::time_point::max(), 1ns));
    } catch (const std::overflow_error&) {
        overflow_rejected = true;
    }
    assert(overflow_rejected);
}

// ---------------------------------------------------------------------------
// LeetCode 933：Number of Recent Calls
// timestamp 在題目中是遞增整數；用 milliseconds 表達後，窗口語意更清楚。
// 每個 ping 最多進出 queue 一次，因此 amortized O(1) time，O(w) space。
// ---------------------------------------------------------------------------
class RecentCounter {
public:
    int ping(std::chrono::milliseconds now)
    {
        if (now < 0ms) {
            throw std::invalid_argument("timestamp must be non-negative");
        }
        if (last_.has_value() && now < *last_) {
            throw std::invalid_argument("timestamps must be monotonic");
        }
        last_ = now;
        calls_.push(now);
        const auto earliest = now - 3'000ms;
        while (calls_.front() < earliest) {
            calls_.pop();
        }
        return static_cast<int>(calls_.size());
    }

private:
    std::queue<std::chrono::milliseconds> calls_;
    std::optional<std::chrono::milliseconds> last_;
};

void leetcode_demo()
{
    RecentCounter counter;
    const int at_1 = counter.ping(1ms);
    const int at_100 = counter.ping(100ms);
    const int at_3_001 = counter.ping(3'001ms);
    const int at_3_002 = counter.ping(3'002ms);
    assert(at_1 == 1);
    assert(at_100 == 2);
    assert(at_3_001 == 3); // 1ms 仍在 inclusive window
    assert(at_3_002 == 3); // 1ms 被移除
}

// ---------------------------------------------------------------------------
// 實務：可測試的 retry/backoff 排程。
// Production code 不把 now() 寫死在演算法裡，而是把 time_point 當輸入，單測無需真的 sleep。
// delays 採 saturating exponential backoff；每個結果都是 absolute steady deadline。
// ---------------------------------------------------------------------------
std::vector<std::chrono::steady_clock::time_point> retry_schedule(
    std::chrono::steady_clock::time_point start,
    std::chrono::milliseconds initial_delay,
    std::chrono::milliseconds maximum_delay,
    std::size_t attempts)
{
    if (initial_delay <= 0ms || maximum_delay < initial_delay) {
        throw std::invalid_argument("invalid backoff");
    }

    std::vector<std::chrono::steady_clock::time_point> result;
    result.reserve(attempts);
    auto deadline = start;
    auto delay = initial_delay;
    for (std::size_t attempt = 0U; attempt < attempts; ++attempt) {
        deadline = checked_add_time_point<std::chrono::steady_clock>(deadline, delay);
        result.push_back(deadline);
        if (delay <= maximum_delay / 2) {
            delay *= 2;
        } else {
            delay = maximum_delay;
        }
    }
    return result;
}

void practical_demo()
{
    const auto epoch = std::chrono::steady_clock::time_point{};
    const auto schedule = retry_schedule(epoch, 100ms, 500ms, 5U);
    assert(schedule.size() == 5U);
    assert(schedule[0] - epoch == 100ms);
    assert(schedule[1] - epoch == 300ms);
    assert(schedule[2] - epoch == 700ms);
    assert(schedule[3] - epoch == 1'200ms);
    assert(schedule[4] - epoch == 1'700ms);

    // Unix timestamp round-trip：資料格式要明寫「seconds」，不要只存裸 count。
    const std::int64_t unix_seconds = 1'700'000'000;
    const std::chrono::sys_seconds point{std::chrono::seconds{unix_seconds}};
    assert(point.time_since_epoch().count() == unix_seconds);
}

int main()
{
    basic_duration_and_calendar_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "Chrono summary: all assertions passed\n";
}
