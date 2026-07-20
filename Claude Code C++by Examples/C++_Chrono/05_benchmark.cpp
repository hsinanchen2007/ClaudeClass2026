// =============================================================================
//  05_benchmark.cpp  —  用 chrono 做函式計時 / micro-benchmark
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/chrono/steady_clock
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼要會用 chrono 做基本計時？                          │
//  └────────────────────────────────────────────────────────────┘
//
//  日常工作中最常做的時間操作就是：「跑一段程式，看花多久」。雖然有專業
//  benchmark 工具（Google benchmark、nanobench），但在「想對兩個寫法快速
//  比較大小」「想抓出 hot path 大概多慢」的階段，<chrono> + steady_clock
//  已經足夠用。
//
//  正確姿勢（2 條鐵律）：
//    (1) 一定用 steady_clock，不用 system_clock
//    (2) 多跑幾輪取平均（避免單次 cache miss 之類雜訊）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 通用計時器：measure(callable)                              │
//  └────────────────────────────────────────────────────────────┘
//
//  把「跑 fn → 算耗時」封裝成函式範本，方便 main 中呼叫：
//
//      template <class F>
//      auto measure(F&& fn) {
//          auto t1 = steady_clock::now();
//          fn();
//          auto t2 = steady_clock::now();
//          return duration_cast<microseconds>(t2 - t1);
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：通用 measure() helper
//   * Demo 2：比較 vector::push_back vs reserve + push_back（reserve 是否真有差）
//   * Demo 3：多輪取中位數，更穩定
// =============================================================================

/*
補充筆記：std::benchmark
  - std::benchmark 使用 <chrono> 時要分清楚 clock、duration、time_point：時鐘產生時間點，兩個時間點相減得到 duration。
  - steady_clock 適合量測耗時，因為它不會受系統時間調整影響；system_clock 適合和日曆時間或 Unix timestamp 互通。
  - duration 帶有單位型別，例如 milliseconds、seconds；duration_cast 用於明確轉換可能截斷的單位。
  - sleep_for/sleep_until 不保證精準喚醒，只保證至少睡到接近指定時間；排程器和系統負載會造成延遲。
  - benchmark 時要避免把輸出、配置或第一次初始化混在量測範圍，否則數字不代表目標程式碼成本。
  - C++20 calendar/time zone API 能表達年月日，但時區資料和平台支援要另外確認。
  - benchmark 應把準備資料和輸出排除在量測區間外，否則測到的是 I/O 或配置成本。
  - 被量測結果若沒有被使用，編譯器可能最佳化掉整段計算；需要用可觀察結果避免空測。
  - 單次量測容易受排程和快取影響，應重複多次並看分布，而不是只看一次數字。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用 chrono 做計時與 micro-benchmark
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 benchmark 一定要用 steady_clock？
//     答：因為 system_clock 可能在量測期間被調整——NTP 校時、使用者改系統時間、夏令時
//         切換都會讓 end - start 得到荒謬的值，甚至是負數（若再存進 unsigned 型別就會
//         溢位成天文數字）。steady_clock 保證【單調前進】、絕不會被往回調，是量測時間
//         間隔的正確選擇。⚠️ 但別講成「不受任何外部調整影響」：Linux 上它對應
//         CLOCK_MONOTONIC，仍會受 NTP 的頻率微調（slewing）影響，且「系統 suspend
//         期間算不算時間」隨平台而異。標準保證的是單調性，不是絕對速率恆定。
//     追問：那 high_resolution_clock 呢？（它只是別名，可能就是 system_clock，
//           不應使用）
//
// 🔥 Q2. 一個可信的計時該怎麼寫？
//     答：steady_clock::now() 夾住待測程式碼，用 duration<double, std::milli> 轉換以
//         保留小數（duration_cast<milliseconds> 會截斷）。務必多跑幾輪：量測「程式碼
//         本身有多快」時取最小值較能濾除排程與 cache 雜訊；量測「使用者實際體感」則
//         看中位數與 P99。單次量測基本沒有參考價值。
//     追問：now() 本身的成本？（通常走 vDSO 免系統呼叫，數十奈秒等級，但在量測極短
//           程式碼時這個成本已不可忽略，應改為量測「跑 N 次的總時間再除」）
//
// ⚠️ 陷阱. 為什麼我的 benchmark 顯示這段程式碼耗時 0 奈秒？
//     答：編譯器把它整個優化掉了（dead code elimination）。若計算結果從未被使用，
//         開啟優化後整段運算會被刪除，你量到的是空迴圈。相關陷阱還有常數摺疊：輸入
//         若是編譯期常數，結果會在編譯期就算完。解法：把結果餵給會阻止優化的機制
//         （如 Google Benchmark 的 DoNotOptimize，本質是用 inline asm 製造假的相依）、
//         或把結果累加後印出、並讓輸入來自執行期（讀檔／亂數／argv）。
//     為什麼會錯：直覺認為「我寫的程式碼一定會被執行」。編譯器只需保證可觀察行為
//         正確，沒有被觀察的運算它有權完全刪掉。注意別用 volatile 當通用解——它會強制
//         每次走記憶體，本身就扭曲了量測結果。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

using namespace std::chrono;

// 前置宣告：附加範例
static void demo_compare_sort_algorithms();
static void demo_throughput_per_second();

// ─────────────────────────────────────────────────────────────
// 共用 helper：吃 callable，回傳耗時 (microseconds)
// ─────────────────────────────────────────────────────────────
template <class F>
microseconds measure(F&& fn) {
    auto t1 = steady_clock::now();
    fn();
    auto t2 = steady_clock::now();
    return duration_cast<microseconds>(t2 - t1);
}

int main() {
    constexpr int N = 1'000'000;

    // ─────────────────────────────────────────────────────────
    // Demo 1：跑一個簡單迴圈、印耗時
    // ─────────────────────────────────────────────────────────
    auto t1 = measure([] {
        long long s = 0;
        for (int i = 0; i < N; ++i) s += i;
        // 用 volatile 防止編譯器把整個迴圈優化掉
        volatile long long sink = s;
        (void)sink;
    });
    std::cout << "[Demo1] sum loop  = " << t1.count() << " us\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：vector reserve 比較
    //   為什麼 reserve 有差？vector 容量不夠時會「重新配置 + 拷貝」，
    //   reserve 能直接配到目標容量，避免反覆搬家。
    // ─────────────────────────────────────────────────────────
    auto noReserve = measure([] {
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
    });
    auto withReserve = measure([] {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);
    });
    std::cout << "[Demo2] no reserve  = " << noReserve.count()   << " us\n";
    std::cout << "[Demo2] with reserve= " << withReserve.count() << " us\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：多輪取中位數 — 比單輪可靠很多
    //   第一輪通常會被 cold cache 拖慢；中位數對偶發雜訊更 robust。
    // ─────────────────────────────────────────────────────────
    constexpr int rounds = 9;
    std::vector<microseconds> samples;
    samples.reserve(rounds);
    for (int i = 0; i < rounds; ++i) {
        samples.push_back(measure([] {
            std::vector<int> v(N);
            std::iota(v.begin(), v.end(), 0);
        }));
    }
    std::nth_element(samples.begin(),
                     samples.begin() + rounds / 2,
                     samples.end());
    std::cout << "[Demo3] vector iota median = "
              << samples[rounds / 2].count() << " us\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼結果跑兩次差很多？
    //    A：CPU cache、turbo boost、其他 process 競爭、編譯器優化都會影響。
    //       要拿來比較大小一定要：(a) 同一個 build 模式、(b) 多輪、(c) 確認
    //       測試片段沒被「死碼消除」(用 volatile sink 之類擋住)。
    //
    //  Q2：怎麼避免「整段被 -O2 優化掉」？
    //    A：把結果寫進 volatile 變數、把結果回傳給外部 caller、或在迴圈內讀
    //       一個 volatile flag 控制 break。Google benchmark 的 DoNotOptimize
    //       做的就是這件事。
    //
    //  Q3：要量到 us 以下精度怎麼辦？
    //    A：steady_clock 在 Linux 通常解析度為 ns 等級。但量 ns 級別的東西
    //       要連跑成千上萬次取平均，並注意 cache、branch prediction 等系統
    //       性雜訊。考慮用 Google benchmark / nanobench。
    //
    demo_compare_sort_algorithms();
    demo_throughput_per_second();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 比較 std::sort vs std::stable_sort
// =============================================================================
//  工作上選 sort 算法時最在意「實測差幾倍」。下面用同樣資料分別跑兩種，並用
//  measure 報告耗時。注意一定要每次重新洗牌，避免被「已排序」優化干擾。
// =============================================================================
static void demo_compare_sort_algorithms() {
    constexpr int N = 200'000;
    std::vector<int> base(N);
    std::iota(base.begin(), base.end(), 0);
    std::mt19937 rng(42);
    std::shuffle(base.begin(), base.end(), rng);

    auto t1 = measure([&] {
        auto v = base;                  // 每次都拿乾淨副本
        std::sort(v.begin(), v.end());
    });
    auto t2 = measure([&] {
        auto v = base;
        std::stable_sort(v.begin(), v.end());
    });
    std::cout << "[sort_cmp] std::sort        = " << t1.count() << " us\n";
    std::cout << "[sort_cmp] std::stable_sort = " << t2.count() << " us\n";
}

// =============================================================================
//  附加 2：實用範例 — 量「每秒處理量」(throughput / ops per second)
// =============================================================================
//  Server / library 報效能慣用單位是「ops/sec」。做法：時間限定一秒內能做幾
//  次運算，或固定次數除以時間。下面是「固定次數除以時間」版本，更穩定。
// =============================================================================
static void demo_throughput_per_second() {
    constexpr int N = 500'000;
    auto us = measure([] {
        long long s = 0;
        // ⚠️ 踩雷：寫成 s += i * i 會先用 int 相乘,i = 46341 就溢位（UB）,
        //    之後才加進 long long 已經太遲——benchmark 數字也跟著失真。
        //    先把其中一個運算元提升成 long long。
        for (int i = 0; i < N; ++i) s += 1LL * i * i;
        volatile long long sink = s; (void)sink;
    });
    double sec = us.count() / 1'000'000.0;
    double ops = N / sec;
    std::cout << "[throughput] " << static_cast<long long>(ops)
              << " ops/sec\n";
}

// 編譯: g++ -std=c++20 -Wall -Wextra 05_benchmark.cpp -o 05_benchmark

// === 預期輸出 ===
// [Demo1] sum loop  = 2057 us
// [Demo2] no reserve  = 17775 us
// [Demo2] with reserve= 15463 us
// [Demo3] vector iota median = 6034 us
// [sort_cmp] std::sort        = 47022 us
// [sort_cmp] std::stable_sort = 46508 us
// [throughput] 421229991 ops/sec
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
