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
        for (int i = 0; i < N; ++i) s += i * i;
        volatile long long sink = s; (void)sink;
    });
    double sec = us.count() / 1'000'000.0;
    double ops = N / sec;
    std::cout << "[throughput] " << static_cast<long long>(ops)
              << " ops/sec\n";
}
