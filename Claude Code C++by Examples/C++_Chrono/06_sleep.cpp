// =============================================================================
//  06_sleep.cpp  —  this_thread::sleep_for / sleep_until
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/thread/sleep_for
//        https://en.cppreference.com/w/cpp/thread/sleep_until
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、兩個 sleep API 的差別                                  │
//  └────────────────────────────────────────────────────────────┘
//
//   sleep_for(duration)     — 「至少睡這麼久」（相對時間）
//   sleep_until(time_point) — 「睡到這個時刻」（絕對時間）
//
//  「至少」二字很重要：作業系統不保證「剛好」 — 可能會晚幾 µs ~ ms。睡
//  完醒來之後一定要當下重新讀時鐘確認。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、什麼時候用 for vs until？                              │
//  └────────────────────────────────────────────────────────────┘
//
//   * 想做「節流」(每隔 N 毫秒做一次) → sleep_until。
//     用 sleep_for 會累積誤差：每次都「至少」N 毫秒，多出來的 jitter 會逐
//     漸把節奏拉長。sleep_until 把「下一次該醒來的時刻」明寫，誤差不會累積。
//   * 「等個 N 毫秒重試」「retry backoff」之類 → sleep_for 即可。
//
//  範例對比：
//
//      // 累積漂移版（sleep_for）
//      while (running) {
//          do_work();
//          std::this_thread::sleep_for(100ms);
//      }
//      // 第 N 次的時間是 N*(work_time + 100ms + jitter)
//
//      // 不漂移版（sleep_until）
//      auto next = steady_clock::now();
//      while (running) {
//          do_work();
//          next += 100ms;
//          std::this_thread::sleep_until(next);
//      }
//      // 第 N 次的時間就是「起點 + N*100ms」（除非 do_work 超過 100ms）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：sleep_for 基本用法
//   * Demo 2：sleep_until 達成「節拍器」
// =============================================================================

/*
補充筆記：std::sleep
  - std::sleep 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - sleep_for 表示至少睡一段 duration，實際醒來時間可能更晚，不能當精準 timer。
  - sleep_until 用絕對 time_point 表示截止時間，適合週期性任務避免累積 drift。
  - 阻塞睡眠期間 thread 不做工作；需要可取消等待時要搭配 condition_variable、jthread stop_token 或平台機制。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】sleep_for / sleep_until
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. sleep_for 與 sleep_until 差在哪？
//     答：sleep_for(d) 是相對睡眠，sleep_until(tp) 是睡到絕對時刻。關鍵差異在做「固定
//         頻率的週期迴圈」時：sleep_for 會累積每輪的處理時間造成漂移（drift），越跑
//         越偏；正確做法是 next += period; sleep_until(next);，讓每一輪都對齊絕對時刻。
//     追問：sleep_until 傳 system_clock 的 time_point 有什麼風險？（會受系統調時影響，
//           時間被往前調就可能睡過頭或提早醒；週期迴圈請用 steady_clock）
//
// 🔥 Q2. 睡眠時間準確嗎？
//     答：兩者都只保證「至少」睡這麼久。實際喚醒時間取決於作業系統的排程精度與 timer
//         解析度，一般排程延遲可達毫秒等級，而且負載越高越不準。任何依賴「剛好睡 X
//         毫秒」的設計都是錯的。
//     追問：需要更高精度怎麼辦？（縮短一點再用 spin 補齊，燒 CPU 換精度；或用平台
//           特定機制如 timerfd。this_thread::yield() 則是「讓出這次時間片」，語意與
//           sleep_for(0) 不同）
//
// Q3. 用 sleep 做輪詢等待有什麼問題？
//     答：延遲與 CPU 之間永遠只能二選一：睡太久反應遲鈍，睡太短則整天在空轉喚醒。
//         而且輪詢無法保證「不漏事件」。有明確事件可等時，應該用 condition_variable、
//         semaphore 或 atomic::wait，讓作業系統在狀態真的改變時才喚醒你。
//     追問：那什麼時候輪詢才合理？（等待的對象沒有提供通知機制時，例如檢查外部檔案
//           或硬體暫存器；此時應搭配指數退讓與上限）
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <thread>

// 前置宣告：附加範例
static void demo_animation_tick_60fps();
static void demo_retry_with_backoff();

int main() {
    using namespace std::chrono;
    using namespace std::chrono_literals;

    // ─────────────────────────────────────────────────────────
    // Demo 1：sleep_for
    // ─────────────────────────────────────────────────────────
    auto t1 = steady_clock::now();
    std::this_thread::sleep_for(80ms);
    auto t2 = steady_clock::now();
    std::cout << "[Demo1] sleep_for(80ms) -> "
              << duration_cast<milliseconds>(t2 - t1).count() << " ms actual\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：sleep_until 節拍器 — 跑 5 拍，每拍 50ms
    //   觀察「next - 起點」應該是 50, 100, 150, 200, 250 附近
    // ─────────────────────────────────────────────────────────
    auto start = steady_clock::now();
    auto next  = start;
    for (int beat = 1; beat <= 5; ++beat) {
        next += 50ms;
        std::this_thread::sleep_until(next);
        auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start);
        std::cout << "[Demo2] beat " << beat
                  << " at +" << elapsed.count() << " ms\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：sleep 在收到 signal 時會被中斷嗎？
    //    A：標準保證會「最多」回傳早於指定時間（spurious wakeup）這個情況
    //       「不應發生」，但實作上 POSIX nanosleep 會被 signal 中斷；
    //       std::this_thread::sleep_for 通常會內部 retry，但保險起見：
    //       想「不漂移」就用 sleep_until。
    //
    //  Q2：sleep 0 / 負時間會怎樣？
    //    A：0 通常等於 thread yield；負 duration 是「立刻返回」（標準允許
    //       但行為不有趣）。
    //
    //  Q3：怎麼正確讓 thread 醒來執行某條件？
    //    A：不要用 sleep + 輪詢；用 std::condition_variable 配
    //       wait_for / wait_until 更省 CPU、反應更快。
    //
    demo_animation_tick_60fps();
    demo_retry_with_backoff();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 60 FPS 動畫 tick（用 sleep_until 不漂移）
// =============================================================================
//  遊戲 / 動畫渲染需要「穩定每秒 60 幀」 — 也就是每幀 ~16.67ms。
//  用 sleep_for 會累積誤差；用 sleep_until 把「下一幀應該開始的時刻」明寫，
//  即使某幀畫太久，下一幀仍能立即接上（不會漂移）。
// =============================================================================
static void demo_animation_tick_60fps() {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    auto frameTime = duration_cast<microseconds>(microseconds{16667}); // ~60 FPS
    auto next = steady_clock::now();
    auto start = next;
    for (int frame = 1; frame <= 5; ++frame) {
        // 模擬一幀的工作
        std::this_thread::sleep_for(2ms);
        next += frameTime;
        std::this_thread::sleep_until(next);
        auto elapsed = duration_cast<microseconds>(steady_clock::now() - start);
        std::cout << "[60fps] frame " << frame
                  << " at +" << elapsed.count() << " us\n";
    }
}

// =============================================================================
//  附加 2：實用範例 — Retry with exponential backoff
// =============================================================================
//  網路請求失敗時典型策略：重試但每次等更久（避免 hammer 對方）。
//  sleep_for 配 duration * (1 << attempt) 就是教科書級實作。
// =============================================================================
static bool flakyOperation(int attempt) {
    // 假設前 3 次都失敗，第 4 次成功
    return attempt >= 3;
}
static void demo_retry_with_backoff() {
    using namespace std::chrono_literals;
    auto base = 20ms;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (flakyOperation(attempt)) {
            std::cout << "[backoff] success at attempt " << attempt << '\n';
            return;
        }
        auto delay = base * (1 << attempt);
        std::cout << "[backoff] attempt " << attempt
                  << " failed, sleep " << delay.count() << " ms\n";
        std::this_thread::sleep_for(delay);
    }
}
