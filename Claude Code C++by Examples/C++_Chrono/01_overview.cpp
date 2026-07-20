// =============================================================================
//  01_overview.cpp  —  <chrono> 三大概念總覽
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/chrono
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼 C++ 要重新設計時間 API？                       │
//  └────────────────────────────────────────────────────────────┘
//
//  傳統 C 的時間 API（time, clock, gettimeofday, struct tm）有兩個大問題：
//   (1) 「單位」混在型別裡 — 你拿到一個 long，要靠註解告訴你它是「毫秒、
//       微秒、秒、tick」哪一種；很容易在傳遞中把 ms 當成 us 用、結果差千倍。
//   (2) 「Wall clock 和 monotonic clock」混為一談 — 牆上時間會被 NTP / 使用
//       者調回去，量測經過時間時就會看到「-3 秒」這種荒謬結果。
//
//  C++11 的 <chrono> 用三個型別分工解決：
//
//      Clock         「誰在計時」      — system_clock / steady_clock 等
//      duration<N,P> 「一段時間長度」  — 模板帶單位，編譯期型別安全
//      time_point    「某個時刻」      — 綁定 Clock 的瞬間
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、三概念互相關係                                         │
//  └────────────────────────────────────────────────────────────┘
//
//      time_point - time_point  =  duration       (兩時刻相差多久)
//      time_point + duration    =  time_point     (時刻平移)
//      duration  + duration     =  duration       (兩段時間總和)
//      Clock::now()             →  time_point<Clock>
//
//  關鍵點：duration 與 time_point 都是 template，內部把「單位」存在型別裡：
//
//      std::chrono::duration<long long, std::milli>   // 「以毫秒為單位」
//      ↑                       ↑
//      Rep（用什麼整數型別存）   Period（每 tick 多長秒，用 std::ratio 表示）
//
//      標準別名：
//        nanoseconds  = duration<int64_t, nano>
//        microseconds = duration<int64_t, micro>
//        milliseconds = duration<int64_t, milli>
//        seconds      = duration<int64_t>
//        minutes      = duration<int64_t, ratio<60>>
//        hours        = duration<int64_t, ratio<3600>>
//
//  不同單位的 duration 互轉「不會自動丟失精度」 — 細→粗會編譯錯，必須用
//  std::chrono::duration_cast 明寫切除：
//
//      auto t = std::chrono::milliseconds{1500};
//      auto s = std::chrono::seconds{t};                          // ❌ 編譯錯
//      auto s = std::chrono::duration_cast<std::chrono::seconds>(t); // ✅ → 1
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：Clock::now() 取 time_point
//   * Demo 2：兩個 time_point 相減 → duration
//   * Demo 3：duration_cast 切換單位
//   * Demo 4：duration 字面量 (1s, 100ms, 5min) — 簡潔好讀
// =============================================================================

/*
補充筆記：std::overview
  - std::overview 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - <chrono> 的三個核心名詞是 clock、time_point、duration；先把這三者分清楚，後面 API 會自然很多。
  - 不要用整數裸值代表毫秒或秒，duration 型別能讓單位出現在型別系統中。
  - 時間程式最常出錯的是用 system_clock 量測耗時；量測間隔應優先使用 steady_clock。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】<chrono> 總覽：duration / time_point / clock
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. chrono 相比「用 long 存毫秒」的優勢是什麼？
//     答：型別安全。裸數值無法區分單位，sleep(1000) 究竟是秒還是毫秒只能靠註解與紀律，
//         而單位錯誤是實務上非常常見且昂貴的 bug。chrono 把單位編碼進型別：seconds 與
//         milliseconds 是不同型別，混用時編譯器自動轉換或直接報錯，隱式轉換只在無損
//         方向開放。而且 std::ratio 的換算在編譯期完成，執行期零額外成本。
//     追問：代價是什麼？（型別名冗長，需要 using namespace std::chrono_literals 搭配
//           100ms 這類字面值改善可讀性；跨 C API 邊界時仍需 .count()）
//
// 🔥 Q2. chrono 的三個核心概念如何區分？
//     答：duration 是「時間長度」（10ms、3s），是向量；time_point 是「某個時鐘紀元起算
//         的一個時刻」，是點；clock 則提供 now() 並定義 epoch 與精度。它們構成一個
//         affine space：point - point = duration、point ± duration = point，但
//         point + point 沒有意義（編譯錯誤）。
//     追問：為什麼 time_point 要把 Clock 編進型別？（不同時鐘的 epoch 意義完全不同，
//           編進型別才能在編譯期擋掉「拿 steady_clock 的時刻去算日期」這種錯誤）
//
// ⚠️ 陷阱. auto ms = (t1 - t0).count(); 這行有什麼問題？
//     答：count() 回傳的是「已脫離單位資訊的裸數值」，而 t1 - t0 的型別是
//         Clock::duration，其單位是實作定義的。同一份程式碼換平台後，同一個數字的
//         意義可能從奈秒變成別的單位。正確做法是先明確轉換再取值，例如
//         duration_cast<microseconds>(d).count() 或 duration<double, std::milli>(d).count()。
//     為什麼會錯：把 count() 當成「取出時間值」的標準動作。它其實是 chrono 型別安全
//         的唯一破口，應該盡量延後到最後要輸出或跨 API 邊界時才呼叫。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <thread>

// 前置宣告：附加範例
static void demo_timeout_remaining();
static void demo_frame_budget_check();

int main() {
    using namespace std::chrono;
    using namespace std::chrono_literals; // 啟用 ms / s / min 字面量

    // ─────────────────────────────────────────────────────────
    // Demo 1：取兩個時刻
    // ─────────────────────────────────────────────────────────
    auto t1 = steady_clock::now();
    std::this_thread::sleep_for(50ms);    // 故意睡 50ms
    auto t2 = steady_clock::now();

    // ─────────────────────────────────────────────────────────
    // Demo 2：兩個 time_point 相減 → duration
    //   t2 - t1 的型別是 steady_clock::duration（通常是 nanoseconds）
    // ─────────────────────────────────────────────────────────
    auto elapsed = t2 - t1;
    // elapsed 的型別是 steady_clock::duration（典型 nanoseconds）
    // .count() 把 duration 變成裸數值。要正確的物理意義，還是該先 cast。
    std::cout << "[Demo2] elapsed = " << elapsed.count() << " (raw ticks)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：duration_cast 顯式切單位
    // ─────────────────────────────────────────────────────────
    auto ms = duration_cast<milliseconds>(elapsed);
    auto us = duration_cast<microseconds>(elapsed);
    std::cout << "[Demo3] elapsed = " << ms.count()  << " ms, "
                                       << us.count()  << " us\n";

    // ─────────────────────────────────────────────────────────
    // Demo 4：字面量 — 在現代程式碼把單位寫進字面值就好
    //   (要 using namespace std::chrono_literals 才認得 ms / s / min)
    // ─────────────────────────────────────────────────────────
    auto sum = 1s + 500ms;       // 1 秒 + 500 毫秒
    auto sum_ms = duration_cast<milliseconds>(sum);
    std::cout << "[Demo4] 1s + 500ms = " << sum_ms.count() << " ms\n"; // 1500

    auto huge = 2min + 3s;
    std::cout << "[Demo4] 2min + 3s   = "
              << duration_cast<seconds>(huge).count() << " s\n"; // 123

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼不能直接 ms → s？
    //    A：粗→細是「乘」，細→粗是「除」會丟資訊（1500ms → 1s 把 0.5 秒丟
    //       了）。為了避免「不小心丟精度」，標準要求「會丟資訊」的轉換必須
    //       用 duration_cast 顯式表達。
    //
    //  Q2：什麼時候用 ratio？
    //    A：99% 用標準別名（ms、s、min...）即可。罕見場合（半拍、自訂計時
    //       單位）才會自己寫 duration<int64_t, ratio<1, 30>> 之類。
    //
    //  Q3：duration.count() 是幹嘛的？
    //    A：把 duration「拆掉型別資訊」變成裸數值（譬如 int64_t）。
    //       要印出來、傳到 C 接口時才用，平常運算「不要 count」 — 直接讓
    //       duration 本身運算，型別系統會幫你檢查單位。
    //
    demo_timeout_remaining();
    demo_frame_budget_check();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 計算「剩餘 timeout」工具
// =============================================================================
//  工作上：開始時設定一個 deadline (now + 5s)，做了幾件事後想知道「還剩多少
//  時間可以做下一步」。直接做 time_point - now() 即可，結果是 duration。
//  陷阱：若已經過期，duration 會是負數；要先檢查或用 max(0, ...) 處理。
// =============================================================================
static std::chrono::milliseconds timeoutRemaining(
        std::chrono::steady_clock::time_point deadline) {
    using namespace std::chrono;
    auto diff = deadline - steady_clock::now();
    if (diff.count() < 0) return milliseconds{0};
    return duration_cast<milliseconds>(diff);
}
static void demo_timeout_remaining() {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    auto deadline = steady_clock::now() + 200ms;
    std::this_thread::sleep_for(50ms);
    auto rem = timeoutRemaining(deadline);
    std::cout << "[timeout] remaining ≈ " << rem.count()
              << " ms (預期約 150)\n";
}

// =============================================================================
//  附加 2：實用範例 — 遊戲 / 動畫的 frame budget 檢查
// =============================================================================
//  60 FPS 遊戲每幀有 16.6ms 預算。如果某段邏輯超過預算就要警告 / 跳幀。
//  duration 之間直接比較（< > ==）會在內部統一單位，不需手動換算。
// =============================================================================
static void demo_frame_budget_check() {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    auto budget = duration_cast<microseconds>(16666us); // ≈ 16.6ms
    auto used   = 9ms + 200us;                          // 假設這幀已花這麼多

    if (used > budget) std::cout << "[frame] over budget!\n";
    else std::cout << "[frame] ok, used=" << duration_cast<microseconds>(used).count()
                   << "us / budget=" << budget.count() << "us\n";
}

// 編譯: g++ -std=c++20 -Wall -Wextra 01_overview.cpp -o 01_overview

// === 預期輸出 ===
// [Demo2] elapsed = 50073933 (raw ticks)
// [Demo3] elapsed = 50 ms, 50073 us
// [Demo4] 1s + 500ms = 1500 ms
// [Demo4] 2min + 3s   = 123 s
// [timeout] remaining ≈ 148 ms (預期約 150)
// [frame] ok, used=9200us / budget=16666us
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
