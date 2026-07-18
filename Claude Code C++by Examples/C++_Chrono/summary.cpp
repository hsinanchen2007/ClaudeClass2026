/*
================================================================================
【C++_Chrono/summary.cpp】

本目錄主題：<chrono> 時間處理（duration / time_point / clock，課件版）

核心三件事：
  1) std::chrono::duration   ：「時間長度」(例如 10ms、3s、2min)
  2) std::chrono::time_point ：「時間點」(例如 某個 clock 的 now())
  3) std::chrono::clock      ：提供 now() 的來源（system_clock/steady_clock 等）

你在工作上最常用 chrono 做三件事：
  - 正確計時（benchmark / timeout / retry backoff）：用 steady_clock
  - 取得「真實世界時間」：用 system_clock（可轉成 time_t）
  - 用強型別避免單位錯誤：duration_cast / literals（50ms, 2s）

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯；C++20 calendar 只提示不實作

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Chrono/C++_Chrono summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Chrono/C++_Chrono summary 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Chrono/C++_Chrono summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>

using namespace std::chrono;

static void header(const char* title) { std::cout << "\n[" << title << "]\n"; }

// -----------------------------------------------------------------------------
// 【重點 1】duration：單位 + 數值（強型別，避免「秒/毫秒」搞混）
// -----------------------------------------------------------------------------
static void demo_duration() {
    header("demo_duration");

    // (1) 建立：明確型別（秒、毫秒…）
    seconds s{3};
    milliseconds ms{1500};

    // (2) duration_cast：做單位轉換（可能截斷）
    auto ms_from_s = duration_cast<milliseconds>(s);
    auto s_from_ms = duration_cast<seconds>(ms); // 1500ms -> 1s（截斷）

    std::cout << "  3s -> " << ms_from_s.count() << "ms\n";
    std::cout << "  1500ms -> " << s_from_ms.count() << "s (trunc)\n";

    // (3) 浮點 duration：可表達 1.5s（注意精度與輸出）
    duration<double> ds = 1.25s;
    std::cout << "  duration<double>(1.25s) = " << ds.count() << " seconds\n";

    // (4) 常用觀念：count() 回傳的是「以該 duration 的 period 表示的數值」
    //     例：milliseconds{1500}.count() 是 1500（不是 1.5）
    std::cout << "  ms.count()=" << ms.count() << " (unit: ms)\n";
}

// -----------------------------------------------------------------------------
// 【重點 2】clocks：system_clock vs steady_clock
// -----------------------------------------------------------------------------
// - system_clock：代表「系統時間」（可轉成 calendar time），但可能被 NTP/使用者調整而跳動
// - steady_clock：單調遞增（適合做計時/benchmark），不可轉成真實日期時間
static void demo_clocks() {
    header("demo_clocks");

    std::cout << "  system_clock is_steady = " << system_clock::is_steady << "\n";
    std::cout << "  steady_clock is_steady = " << steady_clock::is_steady << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】time_point + now() + 計時（benchmark 的正確姿勢）
// -----------------------------------------------------------------------------
static void demo_benchmark() {
    header("demo_benchmark");

    // 計時請用 steady_clock（避免系統時間跳動）
    auto t0 = steady_clock::now();
    volatile std::uint64_t acc = 0;
    for (std::uint64_t i = 0; i < 5'000'00ULL; ++i) acc += i;
    auto t1 = steady_clock::now();

    auto elapsed = duration_cast<microseconds>(t1 - t0);
    std::cout << "  elapsed = " << elapsed.count() << " us (acc=" << acc << ")\n";
}

// -----------------------------------------------------------------------------
// 【重點 3.5】time_point 的基本操作：加減 duration、比較大小
// -----------------------------------------------------------------------------
static void demo_time_point_ops() {
    header("demo_time_point_ops");

    auto t0 = steady_clock::now();
    auto deadline = t0 + 200ms; // time_point + duration = time_point
    std::cout << "  deadline created\n";

    auto t1 = steady_clock::now();
    std::cout << "  t1 < deadline ? " << (t1 < deadline) << "\n";

    // time_point - time_point = duration
    auto dt = t1 - t0;
    std::cout << "  dt(us)=" << duration_cast<microseconds>(dt).count() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 4】sleep_for / sleep_until：簡單延遲（注意：不是精準計時器）
// -----------------------------------------------------------------------------
static void demo_sleep() {
    header("demo_sleep");
    auto t0 = steady_clock::now();
    std::this_thread::sleep_for(50ms);
    auto t1 = steady_clock::now();
    std::cout << "  slept about " << duration_cast<milliseconds>(t1 - t0).count() << " ms\n";

    // sleep_until：睡到某個時間點（通常用 steady_clock + deadline）
    auto deadline = steady_clock::now() + 20ms;
    std::this_thread::sleep_until(deadline);
    std::cout << "  sleep_until(deadline) done\n";
}

// -----------------------------------------------------------------------------
// 【重點 5】system_clock -> 顯示成日期時間（本地時間）
// -----------------------------------------------------------------------------
static void demo_print_system_time() {
    header("demo_print_system_time");

    auto now = system_clock::now();
    std::time_t tt = system_clock::to_time_t(now);

    // localtime 不是 thread-safe；單執行緒 demo OK。
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    tm = *std::localtime(&tt);
#endif

    std::cout << "  now = " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 6】Unix timestamp（秒 / 毫秒）
// -----------------------------------------------------------------------------
static void demo_unix_timestamp() {
    header("demo_unix_timestamp");

    auto now = system_clock::now();
    auto sec = duration_cast<seconds>(now.time_since_epoch()).count();
    auto milli = duration_cast<milliseconds>(now.time_since_epoch()).count();
    std::cout << "  epoch seconds      = " << sec << "\n";
    std::cout << "  epoch milliseconds = " << milli << "\n";
}

static void demo_cpp20_calendar_note() {
    header("demo_cpp20_calendar_note");
#if __cplusplus >= 202002L
    std::cout << "  C++20: chrono 增加 calendar/timezone API（year_month_day, zoned_time...）\n";
#else
    std::cout << "  (C++17：若要 calendar/timezone，需自行處理或使用第三方庫)\n";
#endif
}

int main() {
    demo_duration();
    demo_clocks();
    demo_benchmark();
    demo_time_point_ops();
    demo_sleep();
    demo_print_system_time();
    demo_unix_timestamp();
    demo_cpp20_calendar_note();

    std::cout << "\n[done]\n";
    return 0;
}

/*
================================================================================
【附錄：cppreference 風格速查（常用）】

duration
  - count()
  - duration_cast<To>(from)
  - 常用 typedef：nanoseconds/microseconds/milliseconds/seconds/minutes/hours

time_point
  - clock::now()
  - time_since_epoch()
  - + duration / - duration / 比較

clocks
  - system_clock：可轉 time_t（to_time_t / from_time_t）
  - steady_clock：單調遞增（計時首選）
  - high_resolution_clock：實作定義（很多平台只是別名，不要迷信）

thread helpers
  - this_thread::sleep_for / sleep_until

C++20 延伸（本檔不強推）
  - calendar/timezone：year_month_day, zoned_time, current_zone...
================================================================================
*/
