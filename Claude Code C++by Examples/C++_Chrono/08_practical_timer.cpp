// =============================================================================
//  08_practical_timer.cpp  —  工作上現成可用的小工具:Stopwatch / RateLimiter
// =============================================================================
//  參考:
//    - https://en.cppreference.com/cpp/chrono                       (chrono 總覽)
//    - https://en.cppreference.com/w/cpp/chrono/steady_clock        (steady_clock)
//    - https://en.cppreference.com/w/cpp/thread/sleep_for           (sleep_for)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼這檔特別重要？                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  之前幾個檔案都是 chrono 的「概念與 API」；這檔聚焦兩個「丟到任何專案
//  都能用」的小工具：
//
//   1. Stopwatch      — RAII 計時器，建構即開始計時，析構印耗時或回傳
//   2. RateLimiter    — 控制「每 N 毫秒最多做一次」，常用於 retry / poll
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 工具 1：Stopwatch                                          │
//  └────────────────────────────────────────────────────────────┘
//
//  經典 RAII 模式：建構時記下起點、析構時印耗時。可以加 lap()、reset()
//  等成員擴充功能。
//
//      {
//          Stopwatch sw{"compute"};
//          do_compute();
//      } // ← 這裡會印「compute took XX ms」
//
//  如果不想印只想拿回耗時，給它一個 callback / 累加器即可（這裡用回傳）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 工具 2：RateLimiter                                        │
//  └────────────────────────────────────────────────────────────┘
//
//  邏輯：紀錄「上次做的時間」，新一次呼叫如果距上次不到 minInterval，回傳
//  false（拒絕）；否則更新上次時間並回傳 true。
//
//  典型用途：
//   * 重試 retry：失敗後別馬上重試，至少間隔 200ms
//   * 紀錄抖動 logging：避免短時間刷一堆同樣 log
//   * Poll：每秒最多查一次
// =============================================================================

/*
補充筆記：std::practical_timer
  - std::practical_timer 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - 實用 Timer 類別應在建構時記錄 steady_clock::now，elapsed 時再相減。
  - Timer 若要多次重用，reset() 應明確更新起點，避免呼叫者重新建立物件。
  - 輸出耗時前先決定單位；API 可回傳 duration，讓呼叫者自己選 milliseconds 或 microseconds。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】實用計時工具：Stopwatch 與 RateLimiter
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 寫一個 RAII scope timer 要注意什麼？
//     答：建構時記錄 steady_clock::now()，解構時輸出差值——好處是提早 return 或拋出
//         例外時仍然會印出耗時。注意三點：時鐘必須用 steady_clock；輸出時明確指定單位
//         （用 duration<double, std::milli> 而非 count() 裸值）；解構子中「絕不可拋出
//         例外」，因此輸出失敗只能吞掉或寫到不會拋的通道。
//     追問：如何避免它自己影響量測？（I/O 放在解構子最後、不要在計時區間內做格式化；
//           量測極短區段時要考慮 now() 本身的成本）
//
// 🔥 Q2. 實作限流器（rate limiter）為什麼一定要用 steady_clock？
//     答：限流的本質是「單位時間內最多幾次」，是時間間隔的計算。若用 system_clock，
//         系統時間被往回調時，「上次呼叫時間」會落在未來，限流器可能永久拒絕所有請求；
//         往前調則會一次放行大量請求。這類 bug 只在校時發生時出現，極難重現。
//     追問：token bucket 與固定視窗差在哪？（固定視窗在邊界會出現兩倍突發流量；
//           token bucket 以持續補充的方式平滑，較能反映真實的速率限制）
//
// Q3. 逾時（timeout）該用哪種時鐘與 API？
//     答：相對逾時用 steady_clock 語意的 wait_for；若一定要用絕對時刻的 wait_until，
//         傳入的也應該是 steady_clock 的 time_point，否則系統調時會讓逾時提早或延後。
//         另外逾時後必須重新檢查條件——逾時返回不代表條件沒成立（可能剛好同時發生）。
//     追問：wait_for 帶 predicate 的回傳值意義？（回傳 predicate 的最終值，false 代表
//           逾時且條件仍未成立）
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std::chrono;
using namespace std::chrono_literals;

// 前置宣告：附加範例
static void demo_token_bucket();
static void demo_deadline_check();

// ─────────────────────────────────────────────────────────────
// 工具 1：Stopwatch — RAII 計時
// ─────────────────────────────────────────────────────────────
class Stopwatch {
public:
    explicit Stopwatch(std::string label) : label_(std::move(label)),
                                            start_(steady_clock::now()) {}
    ~Stopwatch() {
        auto ms = duration_cast<milliseconds>(steady_clock::now() - start_);
        std::cout << "[Stopwatch] " << label_ << " took "
                  << ms.count() << " ms\n";
    }

    // 如果中途想取個 lap 不想等析構
    milliseconds lap() const {
        return duration_cast<milliseconds>(steady_clock::now() - start_);
    }

    // 禁止拷貝（避免印兩次）
    Stopwatch(const Stopwatch&) = delete;
    Stopwatch& operator=(const Stopwatch&) = delete;

private:
    std::string label_;
    steady_clock::time_point start_;
};

// ─────────────────────────────────────────────────────────────
// 工具 2：RateLimiter — 「至少間隔 interval 才允許再做一次」
// ─────────────────────────────────────────────────────────────
class RateLimiter {
public:
    explicit RateLimiter(milliseconds interval)
        : interval_(interval),
          // 把 last_ 設為「足夠久之前」，第一次呼叫 allow() 就能通過
          last_(steady_clock::now() - interval - 1ms) {}

    // 回傳 true 表示「允許做」並紀錄時間；false 表示「請晚點再試」
    bool allow() {
        auto now = steady_clock::now();
        if (now - last_ < interval_) return false;
        last_ = now;
        return true;
    }

private:
    milliseconds interval_;
    steady_clock::time_point last_;
};

// ─────────────────────────────────────────────────────────────
// 範例 1：用 Stopwatch 包一段「計算」
// ─────────────────────────────────────────────────────────────
static long long heavyCompute() {
    Stopwatch sw{"heavyCompute"};
    long long s = 0;
    for (int i = 0; i < 5'000'000; ++i) s += i;
    return s;
}

// ─────────────────────────────────────────────────────────────
// 範例 2：用 RateLimiter 限制 log 頻率
// ─────────────────────────────────────────────────────────────
static void busyLogging() {
    RateLimiter logRl{50ms};
    int allowed = 0, blocked = 0;
    auto deadline = steady_clock::now() + 250ms;
    while (steady_clock::now() < deadline) {
        if (logRl.allow()) ++allowed;
        else               ++blocked;
        std::this_thread::sleep_for(5ms);
    }
    std::cout << "[RateLimiter] in 250ms with 50ms cap: "
              << "allowed=" << allowed << " blocked=" << blocked << '\n';
}

int main() {
    // ─────────────────────────────────────────────────────────
    // 觀察：heavyCompute 析構時會印耗時
    // ─────────────────────────────────────────────────────────
    auto sum = heavyCompute();
    std::cout << "[main] sum  = " << sum << '\n';

    // ─────────────────────────────────────────────────────────
    // 觀察：250ms / 50ms 期望大約允許 5~6 次（受 sleep jitter 影響）
    // ─────────────────────────────────────────────────────────
    busyLogging();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：Stopwatch 為什麼禁止拷貝？
    //    A：拷貝意味著兩個物件在不同 scope 析構，會印兩遍同一個 label。
    //       同樣應該禁 move（或讓 move 後的源不再列印） — 我這裡簡單 delete
    //       拷貝、沒處理 move（move 後源仍會打印，但 label 為空）。實作上
    //       可以加個 bool armed_ flag 控制。
    //
    //  Q2：RateLimiter thread-safe 嗎？
    //    A：本檔版本不是 thread-safe。多 thread 用要把 last_ 改成
    //       std::atomic<steady_clock::time_point>（注意 atomic 的支援要看
    //       平台），或加 mutex。
    //
    //  Q3：可不可以做「token bucket」？
    //    A：可以；token bucket 把「速率」轉成 token，每秒生產 N 個、用一個
    //       就減一個。RateLimiter 是極簡版（容量=1）；正式 token bucket 多
    //       一個 capacity 與 last_refill。
    //
    demo_token_bucket();
    demo_deadline_check();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — Token Bucket（API rate limiter 的標準實作）
// =============================================================================
//  比簡單 RateLimiter 更彈性：允許「短時間突發」(burst) 但長期速率受限。
//  概念：每經過 1/rate 秒補 1 個 token，最多累積到 capacity 個；
//        每次 take() 消耗 1 個 token；token 不夠就拒絕。
// =============================================================================
class TokenBucket {
public:
    TokenBucket(double tokensPerSec, double capacity)
        : rate_(tokensPerSec), capacity_(capacity), tokens_(capacity),
          last_(steady_clock::now()) {}

    bool take() {
        refill();
        if (tokens_ < 1.0) return false;
        tokens_ -= 1.0;
        return true;
    }
private:
    void refill() {
        auto now = steady_clock::now();
        double sec = duration<double>(now - last_).count();
        tokens_ = std::min(capacity_, tokens_ + sec * rate_);
        last_ = now;
    }
    double rate_;            // tokens / second
    double capacity_;        // max tokens in bucket
    double tokens_;
    steady_clock::time_point last_;
};
static void demo_token_bucket() {
    TokenBucket tb(50.0, 5.0); // 50 tokens/s, burst 5
    int allowed = 0, denied = 0;
    auto deadline = steady_clock::now() + 200ms;
    while (steady_clock::now() < deadline) {
        if (tb.take()) ++allowed;
        else           ++denied;
        std::this_thread::sleep_for(5ms);
    }
    std::cout << "[token_bucket] 200ms @ 50/s: allowed=" << allowed
              << " denied=" << denied << '\n';
}

// =============================================================================
//  附加 2：實用範例 — DeadlineChecker（檢查是否已過某截止時刻）
// =============================================================================
//  網路請求、長運算的「全局 timeout」工具：建構時設定 deadline，整個 task
//  期間可不斷呼叫 expired() / remaining()，超時就 abort。
// =============================================================================
class DeadlineChecker {
public:
    explicit DeadlineChecker(milliseconds budget)
        : deadline_(steady_clock::now() + budget) {}
    bool expired() const { return steady_clock::now() >= deadline_; }
    milliseconds remaining() const {
        auto r = deadline_ - steady_clock::now();
        return r.count() > 0 ? duration_cast<milliseconds>(r) : milliseconds{0};
    }
private:
    steady_clock::time_point deadline_;
};
static void demo_deadline_check() {
    DeadlineChecker dc{100ms};
    std::this_thread::sleep_for(40ms);
    std::cout << "[deadline] remaining=" << dc.remaining().count()
              << " ms, expired? " << dc.expired() << '\n';
    std::this_thread::sleep_for(80ms);
    std::cout << "[deadline] remaining=" << dc.remaining().count()
              << " ms, expired? " << dc.expired() << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra 08_practical_timer.cpp -o 08_practical_timer

// === 預期輸出 ===
// [Stopwatch] heavyCompute took 10 ms
// [main] sum  = 12499997500000
// [RateLimiter] in 250ms with 50ms cap: allowed=5 blocked=45
// [token_bucket] 200ms @ 50/s: allowed=14 denied=25
// [deadline] remaining=59 ms, expired? 0
// [deadline] remaining=0 ms, expired? 1
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
