// =============================================================================
//  09_unix_timestamp.cpp  —  與 Unix epoch / time_t / strftime 互轉
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/chrono/system_clock/to_time_t
//    https://en.cppreference.com/w/cpp/chrono/system_clock/from_time_t
//    https://en.cppreference.com/w/cpp/io/manip/put_time
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼這檔需要存在？                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  工作上極常見的需求：
//    * 拿到一個 Unix timestamp (來自 DB / API / log)，要轉成可讀字串
//    * 拿到一個本地時間字串，要轉成 timestamp
//    * 寫 log 時帶上 ISO 8601 時間
//
//  C++17 工具就足以解決。C++20 / 23 有更乾淨的 std::format / chrono::parse，
//  但本檔保持 C++17 相容。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 路線圖                                                     │
//  └────────────────────────────────────────────────────────────┘
//
//  system_clock::time_point  ←─to_time_t─→  std::time_t
//                                                │
//                                                ↓ localtime_r / gmtime_r
//                                            std::tm
//                                                │
//                                                ↓ strftime / put_time
//                                              字串
//
//  反方向：字串 → strptime / get_time → tm → mktime → time_t → from_time_t →
//  time_point
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：time_point → Unix timestamp（秒）
//   * Demo 2：time_point → 本地時間字串
//   * Demo 3：time_point → UTC ISO 8601 字串
//   * Demo 4：字串解析 → time_point
// =============================================================================

/*
補充筆記：std::unix_timestamp
  - std::unix_timestamp 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - Unix timestamp 通常是從 1970-01-01 UTC 起算的秒數，但實際型別和精度要看 API。
  - system_clock::to_time_t 常以秒為單位，會丟失 sub-second 精度。
  - 本地時間顯示會牽涉時區；timestamp 本身是時間點，不包含使用者所在地格式。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Unix timestamp 與 time_t 互轉
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 如何取得與還原 Unix timestamp？
//     答：取得：對 system_clock::now().time_since_epoch() 做
//         duration_cast<seconds>(...).count()；還原：用
//         system_clock::time_point(seconds(secs))。也可以用 system_clock::to_time_t()
//         與 from_time_t()。必須用 system_clock——steady_clock 的 epoch 沒有定義意義
//         （常是開機時間），拿它算 timestamp 會得到完全錯誤的日期。
//     追問：毫秒級的 timestamp 怎麼取？（duration_cast<milliseconds>，注意目標型別的
//           位寬要足夠，秒級 32-bit 會在 2038 年溢位）
//
// 🔥 Q2. Unix time 與閏秒的關係？
//     答：Unix time 定義上「不計閏秒」——閏秒發生時同一個秒數會重複或被跳過，所以兩個
//         Unix timestamp 相減得到的不是真實經過的 SI 秒數。C++20 起標準明確規定
//         system_clock 表示 Unix time 且不計閏秒；需要真實物理時間差要用 utc_clock，
//         兩者之間以 clock_cast 轉換。
//     追問：這在實務上會踩到嗎？（一般業務不會，但在跨閏秒的長時間間隔、時間序列
//           資料庫、金融時間戳對齊上會）
//
// Q3. localtime / gmtime 在多執行緒下有什麼問題？
//     答：std::localtime 與 std::gmtime 回傳指向「共享的靜態內部緩衝區」的指標，多個
//         thread 同時呼叫會互相覆寫，是典型的 data race。POSIX 提供可重入的
//         localtime_r / gmtime_r。C++20 之後更好的做法是直接用 zoned_time 與
//         std::format，完全避開 C 的時間 API。
//     追問：localtime 還有什麼坑？（它依賴行程層級的 TZ 設定，在伺服器上通常應該
//           一律以 UTC 儲存與運算，只在最終顯示時才轉成本地時區）
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

// 前置宣告：附加範例
static void demo_age_from_timestamp();
static void demo_human_diff();

int main() {
    using namespace std::chrono;

    auto now = system_clock::now();

    // ─────────────────────────────────────────────────────────
    // Demo 1：time_point → Unix timestamp
    //   1.1：用 to_time_t — 結果單位是「秒」
    //   1.2：用 time_since_epoch — 想要 ms / us / ns 都行
    // ─────────────────────────────────────────────────────────
    std::time_t tt = system_clock::to_time_t(now);
    std::cout << "[Demo1.1] unix sec  = " << tt << '\n';

    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
    std::cout << "[Demo1.2] unix ms   = " << ms << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：time_point → 本地時間字串 "YYYY-MM-DD HH:MM:SS"
    //   localtime_r 是 POSIX 的 reentrant 版本（thread-safe）。
    //   Windows 沒有這個，得用 localtime_s。
    // ─────────────────────────────────────────────────────────
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    std::cout << "[Demo2] local       = "
              << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：time_point → UTC ISO 8601
    // ─────────────────────────────────────────────────────────
    std::tm utc_tm{};
#if defined(_WIN32)
    gmtime_s(&utc_tm, &tt);
#else
    gmtime_r(&tt, &utc_tm);
#endif
    std::cout << "[Demo3] utc iso8601 = "
              << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ") << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 4：字串 → time_point
    //   流程：istringstream + std::get_time → tm → mktime → time_t
    //         → system_clock::from_time_t → time_point
    //
    //   注意：mktime 把 tm 視為「本地時間」，會把 tm_isdst 等加進來算；
    //         如果你的字串是 UTC，要用 timegm（POSIX，非標準）或自己補正。
    // ─────────────────────────────────────────────────────────
    std::string s = "2026-05-05 10:30:00";
    std::tm parsed{};
    std::istringstream iss(s);
    iss >> std::get_time(&parsed, "%Y-%m-%d %H:%M:%S");
    if (iss.fail()) {
        std::cout << "[Demo4] parse failed\n";
    } else {
        parsed.tm_isdst = -1;     // 讓系統自動判斷夏令時
        std::time_t back = std::mktime(&parsed);
        auto tp = system_clock::from_time_t(back);
        std::cout << "[Demo4] parsed unix sec = "
                  << system_clock::to_time_t(tp) << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：time_t 是 32 還 64 bit？
    //    A：歷史上多為 32 bit signed → 2038 年要溢位。現代平台普遍升到
    //       64 bit。重要紀錄系統別只存 32 bit time_t！
    //
    //  Q2：put_time 不會 thread-safe 嗎？
    //    A：put_time 本身 OK，但「製造 std::tm」的 localtime/gmtime 不是
    //       reentrant — 多 thread 要用 _r / _s 版本。
    //
    //  Q3：為什麼不直接 std::format("{:%F %T}", tp)？
    //    A：要 C++20。本檔保 C++17 相容；如果你的工具鏈支援 C++20，那一行
    //       搞定，不用 put_time。
    //
    demo_age_from_timestamp();
    demo_human_diff();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 由 Unix timestamp 計算「資料年齡」
// =============================================================================
//  Server log、DB record 常存 Unix timestamp；要判斷「這筆紀錄是不是太舊」
//  時，把 timestamp 轉回 system_clock::time_point，跟 now() 相減。
// =============================================================================
static void demo_age_from_timestamp() {
    using namespace std::chrono;
    std::time_t stored = system_clock::to_time_t(system_clock::now()) - 3661; // 1h 1m 1s 前
    auto tp = system_clock::from_time_t(stored);
    auto age = duration_cast<seconds>(system_clock::now() - tp);
    std::cout << "[age] timestamp " << stored
              << " is " << age.count() << " seconds ago (約 3661)\n";
}

// =============================================================================
//  附加 2：實用範例 — 把兩個 timestamp 差距變成可讀字串
// =============================================================================
//  社群網站常見「3 分鐘前」「2 小時前」這類顯示。簡單版做法：用 chrono 的
//  duration_cast 套到不同單位，依大小選最大適合單位。
// =============================================================================
static std::string humanizeDiff(std::chrono::seconds diff) {
    using namespace std::chrono;
    auto s = diff.count();
    if (s < 60)    return std::to_string(s) + " sec ago";
    if (s < 3600)  return std::to_string(s / 60) + " min ago";
    if (s < 86400) return std::to_string(s / 3600) + " hr ago";
    return std::to_string(s / 86400) + " days ago";
}
static void demo_human_diff() {
    using namespace std::chrono;
    std::cout << "[humanize] " << humanizeDiff(seconds{45})    << '\n';
    std::cout << "[humanize] " << humanizeDiff(seconds{120})   << '\n';
    std::cout << "[humanize] " << humanizeDiff(seconds{7200})  << '\n';
    std::cout << "[humanize] " << humanizeDiff(seconds{200000})<< '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra 09_unix_timestamp.cpp -o 09_unix_timestamp

// === 預期輸出 ===
// [Demo1.1] unix sec  = 1784540461
// [Demo1.2] unix ms   = 1784540461620
// [Demo2] local       = 2026-07-20 02:41:01
// [Demo3] utc iso8601 = 2026-07-20T09:41:01Z
// [Demo4] parsed unix sec = 1778002200
// [age] timestamp 1784536800 is 3661 seconds ago (約 3661)
// [humanize] 45 sec ago
// [humanize] 2 min ago
// [humanize] 2 hr ago
// [humanize] 2 days ago
