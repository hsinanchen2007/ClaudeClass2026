// =============================================================================
//  03_duration.cpp  —  duration 與單位轉換
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/chrono/duration
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、duration 是「時間長度」                                │
//  └────────────────────────────────────────────────────────────┘
//
//  std::chrono::duration<Rep, Period> 兩個 template 參數：
//    Rep    — 用來存值的數值型別（int、int64_t、double ...）
//    Period — 一個 std::ratio<N,D>，表達「每 tick 多少秒」
//
//  例：
//    duration<int,        std::ratio<1, 1000>>   // 整數毫秒
//    duration<double,     std::ratio<1>>          // double 秒
//    duration<long long,  std::ratio<60>>         // 整數分鐘
//
//  標準別名（最常用）：
//      nanoseconds  microseconds  milliseconds  seconds  minutes  hours
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、四則運算 — 單位由型別系統管                            │
//  └────────────────────────────────────────────────────────────┘
//
//   * d1 + d2 / d1 - d2：兩個 duration 加減 → 結果單位是「最細的共同單位」
//      e.g. seconds + milliseconds → milliseconds
//   * d * scalar / d / scalar：duration 跟純量乘除
//   * d1 / d2：兩個 duration 相除 → 純量比值（無單位）
//   * d1 % d2：duration 取模
//
//  好處：「秒 + 毫秒」compiler 自動換算到統一單位，不用手動乘 1000，也不
//  會搞錯方向。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、duration_cast / floor / ceil / round （C++17）         │
//  └────────────────────────────────────────────────────────────┘
//
//   * duration_cast<T>(d) — 截斷往零取（trunc）
//   * floor<T>(d)         — 往下取（往負無窮）
//   * ceil<T>(d)          — 往上取
//   * round<T>(d)         — 四捨五入到偶數
//
//  例：1500ms 轉 seconds
//    duration_cast → 1
//    floor          → 1
//    ceil           → 2
//    round          → 2  (1.5 取最近偶數 → 2)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、字面量（C++14 chrono_literals）                       │
//  └────────────────────────────────────────────────────────────┘
//
//      using namespace std::chrono_literals;
//      auto a = 100ns;     // nanoseconds
//      auto b = 100us;     // microseconds
//      auto c = 100ms;     // milliseconds
//      auto d = 100s;      // seconds
//      auto e = 100min;    // minutes
//      auto f = 100h;      // hours
//
//  字面量讓 sleep_for(500ms)、timeout(2s) 這類呼叫一目了然。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：duration 加減乘除
//   * Demo 2：duration_cast / floor / ceil / round 差異
//   * Demo 3：用 double duration 做高精度時間
// =============================================================================

/*
補充筆記：std::duration
  - std::duration 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - duration 的型別同時包含數值型別和比例，例如 milliseconds 是 ratio<1,1000>。
  - duration_cast 會明確處理可能截斷的轉換，例如 microseconds 轉 milliseconds。
  - 用 auto 接 duration 運算結果時要知道單位可能被推導成共同型別，不一定是你肉眼看到的單位。
*/
#include <chrono>
#include <iostream>

// 前置宣告：附加範例
static void demo_human_readable_duration();
static void demo_retry_backoff();

int main() {
    using namespace std::chrono;
    using namespace std::chrono_literals;

    // ─────────────────────────────────────────────────────────
    // Demo 1：四則運算 — 單位自動處理
    // ─────────────────────────────────────────────────────────
    auto a = 1s + 500ms;            // 結果型別：milliseconds
    auto b = 2min - 30s;             // 結果型別：seconds
    auto ratio = 1min / 200ms;       // 兩個 duration 相除 → 純量
    auto half  = 1s / 2;             // duration / 純量 → 還是 duration
    std::cout << "[Demo1] 1s+500ms = " << a.count() << " ms\n";
    std::cout << "[Demo1] 2min-30s = " << b.count() << " s\n";
    std::cout << "[Demo1] 1min/200ms ratio = " << ratio << '\n';
    std::cout << "[Demo1] 1s/2 = " << half.count() << " ms\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：四種「轉粗單位」工具的差異
    // ─────────────────────────────────────────────────────────
    milliseconds d{1500};   // 1.5 秒
    auto cast  = duration_cast<seconds>(d);
    auto fl    = floor<seconds>(d);
    auto ce    = ceil<seconds>(d);
    auto rd    = round<seconds>(d);
    std::cout << "[Demo2] 1500ms → seconds:\n"
              << "         duration_cast = " << cast.count() << '\n'
              << "         floor          = " << fl.count()   << '\n'
              << "         ceil           = " << ce.count()   << '\n'
              << "         round          = " << rd.count()   << '\n';

    // 對負數時更明顯
    milliseconds neg{-1500};
    std::cout << "[Demo2] -1500ms → seconds:\n"
              << "         duration_cast = " << duration_cast<seconds>(neg).count() << '\n'
              << "         floor          = " << floor<seconds>(neg).count() << '\n'
              << "         ceil           = " << ceil<seconds>(neg).count()  << '\n'
              << "         round          = " << round<seconds>(neg).count() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：用 double 當 Rep 做高精度時間
    //   標準別名都是整數 Rep；要小數秒就自己定 alias：
    // ─────────────────────────────────────────────────────────
    using fseconds = duration<double>;
    fseconds elapsed = 1500ms;       // 自動換算 → 1.5（秒）
    std::cout << "[Demo3] 1500ms as double seconds = " << elapsed.count() << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：duration_cast 跟 static_cast<T>(d.count()) 差在哪？
    //    A：duration_cast 會處理「單位換算 + 截斷」一次完成，型別安全；
    //       手動拿 count() 再 static_cast 容易忘記乘 ratio，產生 bug。
    //
    //  Q2：怎麼比較兩個 duration 大小？
    //    A：直接 <、>、==。不同單位的 duration 比較會自動換到共同單位，
    //       很乾淨：if (elapsed > 100ms) ...
    //
    //  Q3：duration 可以 < 0 嗎？
    //    A：可以，整個 chrono 體系是有號的（int64_t 為主）。倒著計時、回頭
    //       走時都能正確表達。
    //
    demo_human_readable_duration();
    demo_retry_backoff();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 把 duration 切成「h:m:s」可讀形式
// =============================================================================
//  進度條 / log / UI 顯示常要把秒數拆成「2h 5m 3s」。chrono 有現成的 floor +
//  取餘做法，比手動除法易讀很多。
// =============================================================================
static void demo_human_readable_duration() {
    using namespace std::chrono;
    seconds total{2 * 3600 + 5 * 60 + 3};  // 2h 5m 3s
    auto h = duration_cast<hours>(total);
    auto m = duration_cast<minutes>(total - h);
    auto s = duration_cast<seconds>(total - h - m);
    std::cout << "[human] " << total.count() << "s -> "
              << h.count() << "h "
              << m.count() << "m "
              << s.count() << "s\n";
}

// =============================================================================
//  附加 2：實用範例 — 指數退避 (exponential backoff) 延時序列
// =============================================================================
//  網路重試、API rate-limit 常見的退避策略：每次失敗後等待時間翻倍。
//  duration 支援乘法，能優雅表達：100ms * (1 << attempt)。
// =============================================================================
static void demo_retry_backoff() {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    auto base = 100ms;
    std::cout << "[backoff] ";
    for (int attempt = 0; attempt < 5; ++attempt) {
        auto delay = base * (1 << attempt);  // 100, 200, 400, 800, 1600 ms
        std::cout << delay.count() << "ms ";
    }
    std::cout << '\n';
}
