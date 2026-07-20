// =============================================================================
//  02_clocks.cpp  —  三大時鐘 system_clock / steady_clock / high_resolution_clock
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/chrono#Clocks
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼要分這麼多種 clock？                            │
//  └────────────────────────────────────────────────────────────┘
//
//  時間在電腦裡有兩個面向：
//
//    A) 「現在是幾點幾分」 — 牆上時間 (wall clock)。要對應日曆與時區，
//       受 NTP 校正、夏令時、使用者手動調表的影響。
//    B) 「過了多少時間」 — 單調流逝的時間 (monotonic)。不受外界調整影響，
//       只會往前走，適合量測「執行了幾秒」「過了多久要 timeout」。
//
//  C++ 把這兩件事分成不同 clock 型別：
//
//      std::chrono::system_clock          → 牆上時間 (A)
//      std::chrono::steady_clock          → 單調流逝時間 (B)
//      std::chrono::high_resolution_clock → 高解析度時鐘 (大多等同 steady_clock)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、各 clock 的差異與選擇                                 │
//  └────────────────────────────────────────────────────────────┘
//
//  ┌─────────────────────┬─────────┬─────────┬───────────────────────────┐
//  │  clock              │ steady? │ epoch   │ 主要用途                   │
//  ├─────────────────────┼─────────┼─────────┼───────────────────────────┤
//  │ system_clock        │  否      │ 通常為   │ 紀錄真實時間、寫 log、     │
//  │                     │          │ 1970-01 │ 跟 time_t / strftime 互轉  │
//  │ steady_clock        │  是      │ 未指定   │ 量測經過時間、timeout、    │
//  │                     │          │ 任意起點 │ benchmark                  │
//  │ high_resolution_clock│ 視實作  │ 視實作   │ 多數實作 == steady_clock   │
//  └─────────────────────┴─────────┴─────────┴───────────────────────────┘
//
//  經驗法則：
//   * 「測一段程式跑多久」→ steady_clock
//   * 「現在幾點？要存成 log」→ system_clock
//   * high_resolution_clock 別用 — 各 platform 別名不同，可移植性差。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、is_steady 與 epoch                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  每個 clock 都有 static constexpr bool is_steady — 編譯期就能知道它是否
//  單調。你可以用 static_assert 強制檢查：
//
//      static_assert(std::chrono::steady_clock::is_steady,
//                    "we require monotonic clock");
//
//  system_clock 自 C++20 起明確規定 epoch 為 1970-01-01 UTC，這以前是「實
//  作定義」，但 99% 的實作早就是這個值。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：印出三個 clock 的 is_steady 與目前時間
//   * Demo 2：用 system_clock 拿「現在幾點」並轉成可印字串
//   * Demo 3：用 steady_clock 量測 sleep 是否真的睡到位
// =============================================================================

/*
補充筆記：std::clocks
  - std::clocks 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - system_clock 對應牆上時間，可能被 NTP 或使用者調整；steady_clock 保證單調，適合計時。
  - high_resolution_clock 可能只是 system_clock 或 steady_clock 的別名，不能只看名字就假設最準。
  - 跨平台程式應明確寫出選哪個 clock，因為不同標準庫實作的 clock typedef 可能不同。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】三大時鐘：system_clock / steady_clock / high_resolution_clock
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 三個時鐘差在哪？
//     答：system_clock 是牆上時間，對應日曆，可被 NTP 校正、使用者手動調整、夏令時
//         影響，因此「可能倒退」，is_steady 為 false；它是唯一能與 time_t／日曆互轉的
//         時鐘。steady_clock 是單調時鐘，is_steady 為 true，保證只會前進且速率固定，
//         epoch 通常是開機時間（沒有意義），只能用來量測「經過了多久」。
//         high_resolution_clock 只是「別名」，實作可自由選擇指向哪一個。
//     追問：is_steady 保證什麼？（tick 之間單調不減且前進速率恆定）
//
// 🔥 Q2. 什麼時候用哪一個？
//     答：量測時間間隔、逾時、retry backoff 一律用 steady_clock；顯示「現在幾點」、
//         寫 log 時間戳、與 DB／API 交換 timestamp 用 system_clock。兩者用途互斥，
//         不可混用——尤其不要拿 steady_clock 的 time_since_epoch() 去算日期，它的
//         epoch 沒有定義意義。
//     追問：wait_until 傳 system_clock 的 time_point 有什麼風險？（會受調時影響，
//           系統時間被往前調就可能提早或延後醒來；相對逾時請用 wait_for）
//
// ⚠️ 陷阱. 用 high_resolution_clock 量測時間有什麼風險？
//     答：它只是 typedef，實作可自由決定指向哪個時鐘。在某些標準函式庫上它是
//         system_clock——也就是可被 NTP 調整、可能倒退的牆上時鐘，用它 benchmark 會
//         偶發出現負數或荒謬結果；在另一些實作上它是 steady_clock，行為正確。
//         同一份程式碼在不同平台語意不同，這是最惡劣的可攜性陷阱。
//     為什麼會錯：名字裡的 "high_resolution" 讓人以為它是「最精準的那個」。實際上
//         現代平台上 steady_clock 的解析度通常相同。結論：不要用 high_resolution_clock，
//         量測用 steady_clock、顯示時間用 system_clock。要驗證你的平台可用
//         static_assert(std::is_same_v<high_resolution_clock, steady_clock>)。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

// 前置宣告：附加範例
static void demo_log_with_timestamp();
static void demo_wall_vs_steady_diff();

int main() {
    using namespace std::chrono;
    using namespace std::chrono_literals;

    // ─────────────────────────────────────────────────────────
    // Demo 1：is_steady 比較
    // ─────────────────────────────────────────────────────────
    std::cout << std::boolalpha;
    std::cout << "[Demo1] system_clock::is_steady          = "
              << system_clock::is_steady << '\n';
    std::cout << "[Demo1] steady_clock::is_steady          = "
              << steady_clock::is_steady << '\n';
    std::cout << "[Demo1] high_resolution_clock::is_steady = "
              << high_resolution_clock::is_steady << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：用 system_clock 列印現在時間（牆上時間）
    //   流程：system_clock::now() → time_t → struct tm → strftime
    //   C++20 後可直接用 std::format("{:%F %T}", now)，本檔保持 C++17。
    // ─────────────────────────────────────────────────────────
    auto now = system_clock::now();
    std::time_t tt = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::cout << "[Demo2] now = "
              << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：steady_clock 量測 sleep 經過了多久
    //   注意：絕對不要用 system_clock 做這件事 — 萬一系統時間被 NTP 拉
    //   回去，差值會變成負的（甚至大幅錯誤）。
    // ─────────────────────────────────────────────────────────
    auto t1 = steady_clock::now();
    std::this_thread::sleep_for(120ms);
    auto t2 = steady_clock::now();
    auto elapsed = duration_cast<milliseconds>(t2 - t1);
    std::cout << "[Demo3] sleep_for(120ms) actually took "
              << elapsed.count() << " ms\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 system_clock 不單調？
    //    A：因為它就是 OS 對外暴露的牆上時間 — 一旦使用者把表調回 5 秒前，
    //       或 NTP daemon 把時鐘拉一下，它真的會「倒退」。所以拿 system_clock
    //       做 timeout 會出 bug。
    //
    //  Q2：高解析度時鐘到底有多細？
    //    A：在 Linux x86_64 上 steady_clock::period 通常是 ns（10^-9 秒）。
    //       但「實際解析度」受作業系統 / 硬體 timer 限制 — 量到的 jitter 可
    //       能落在數百 ns 到 µs 級。
    //
    //  Q3：怎麼把 system_clock::time_point 印成 ISO 8601？
    //    A：C++17 用 put_time（如本範例）。C++20 用 std::format："{:%FT%TZ}"
    //       一行解決。
    //
    demo_log_with_timestamp();
    demo_wall_vs_steady_diff();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 帶毫秒的 log timestamp
// =============================================================================
//  Server log 常見格式：「2026-05-18 12:34:56.789 INFO ...」
//  做法：先用 system_clock::now() 取得目前時間，秒級用 put_time 印；毫秒從
//  time_since_epoch() 取餘數補在後面。
// =============================================================================
static std::string nowLogTimestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto tt  = system_clock::to_time_t(now);
    auto ms  = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}
static void demo_log_with_timestamp() {
    std::cout << "[log_ts] " << nowLogTimestamp() << " INFO server start\n";
}

// =============================================================================
//  附加 2：實用範例 — 偵測系統時間被「拉走」(NTP / 手動調整)
// =============================================================================
//  steady_clock 是單調的，永遠往前走。
//  比較「steady_clock 過了多久」與「system_clock 過了多久」的差距，可以察覺
//  系統時間是否被外力調整 (NTP 校正、使用者改時間)。
// =============================================================================
static void demo_wall_vs_steady_diff() {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    auto s1 = steady_clock::now();
    auto w1 = system_clock::now();
    std::this_thread::sleep_for(50ms);
    auto s2 = steady_clock::now();
    auto w2 = system_clock::now();

    auto steadyDelta = duration_cast<milliseconds>(s2 - s1).count();
    auto wallDelta   = duration_cast<milliseconds>(w2 - w1).count();
    std::cout << "[drift] steady=" << steadyDelta
              << " ms, wall=" << wallDelta << " ms"
              << " (差距很大表示 wall clock 被調過)\n";
}
