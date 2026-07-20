// =============================================================
// 13_false_sharing.cpp  --  False sharing 與 cache line padding
// =============================================================
//
// 本課目標:
//   1. 看到「False sharing」如何在你以為各執行緒互不干擾的
//      情況下,把效能拖慢 5–10 倍。
//   2. 學會用 alignas(64) (或 std::hardware_destructive_
//      interference_size) 把每個熱點變數放到自己的 cache line
//      上。
//   3. 培養一個習慣:任何「每執行緒一份、會被頻繁寫入」的
//      變數,都應該被填充到 cache line 邊界。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 13_false_sharing.cpp \
//         -o 13_false_sharing
//
// 執行方式:
//     ./13_false_sharing
// =============================================================
//
// 什麼是 False Sharing?
//
// 現代 CPU 的 cache 不是以 byte 為單位,而是以「cache line」
// 為單位 (x86 上通常是 64 bytes,Apple silicon 是 128 bytes)。
// 當核心 A 寫入它擁有的某個變數時,如果這個 cache line *也*
// 包含核心 B 正在使用的變數,B 那份 cache 就會被 *無效化* —— 即使
// 兩邊在邏輯上根本沒有共享資料。下次 B 要讀,得從 LLC 或主
// 記憶體重新拉。
//
// 這個現象就叫做 "false sharing":資料不是真的共享,但底層
// 的 cache coherence 協定 (MESI) 卻把它當成共享來處理。
//
// 在多執行緒計數器、per-thread accumulator、執行緒池中的
// per-worker stats 等地方非常常見,是真實世界中最常見的
// 隱形效能殺手之一。
//
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     False sharing ── 看不見的 cache line 爭用
// 前置課程: lesson 04
// 觀念詞彙:
//   - cache line     ── x86 通常 64B,ARM 大多 128B
//   - false sharing  ── 兩個邏輯上不相干的變數共用一條 line
//   - cache line bouncing ── line 在多核 cache 之間來回搬
//   - alignas        ── 強制變數對齊到指定邊界
// 新介紹 API:
//   alignas(N) Type x;                    對齊到 N 的倍數
//   alignas(64) std::atomic<T> x;         經典寫法,獨佔一條 line
//   std::hardware_destructive_interference_size  標準的 line 大小常數
//   #include <new>                         上面這個常數的 header
// 何時做 padding:
//   - 多 thread 各自頻繁寫的「per-thread slot」
//   - SPSC ring 的 head/tail 指標 (lesson 16)
//   - thread pool 中每個 worker 的本地統計
// 何時不必 padding:
//   - 唯讀的共用變數 (false sharing 只在「寫」時有害)
//   - 程式還沒到效能瓶頸 → 不用過度優化
// 常見錯誤:
//   - 假設 std::atomic 自動對齊到 cache line → 不會
//   - 對所有變數都加 alignas(64) → cache 利用率反而下降
//   - 看到效能怪 → 沒想到 false sharing,只盯 lock
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── cache coherence 與 false sharing
// =============================================================
//
// 1. cache line 是同步單位
//    現代 CPU 的 cache (L1/L2/L3) 不是按單一 byte 管理,而是按 *cache line*。
//    line 大小通常 64 bytes (Intel/AMD/Apple Silicon、ARM Cortex-A 多為 64;
//    M1 大型核 128;部分 ARM 64 + 64 sector)。
//    意涵:即使你只想讀 1 個 int,CPU 仍把整 64 byte 線拉進 L1。
//    寫入也是一整條線當作 invalidate 單位。
//
// 2. MESI 協定 (cache 之間怎麼對話)
//    每條 cache line 在每個 core 的 L1 上有狀態:
//      M (Modified)   ── 我有最新的,RAM 是舊的;只有我有
//      E (Exclusive)  ── 我有唯一一份,且和 RAM 同步
//      S (Shared)     ── 多個核都有,內容跟 RAM 同步
//      I (Invalid)    ── 失效,要再去拉
//    當 core A 寫一條被 core B 也持有的 line:A 必須先 broadcast invalidate,
//    讓 B 把它丟成 I,A 才能變 M。這個 invalidate 訊號穿越 ring bus / mesh,
//    成本約 10-50 ns (同 socket) ~ 100-300 ns (跨 socket NUMA)。
//
// 3. true sharing vs false sharing
//    true sharing:兩條 thread *真的* 改同一個變數 ── 沒辦法,本來就要排隊。
//    false sharing:兩個 *邏輯上獨立* 的變數恰好落在同一條 line。看起來毫
//                   無關係的程式,效能卻像在搶同一把鎖。
//    最容易踩坑的場景:
//      - struct { atomic<int> a, b; }   a 給 thread A,b 給 thread B
//      - vector<atomic<int>> stats(N)   N 個 thread 各 ++ 自己的 slot
//    每次寫都要把整條 line 從別人手中奪回來 → ping-pong → 效能崩塌。
//
// 4. 怎麼診斷
//    perf c2c (Linux)  ── cache-to-cache transfer 紀錄,直接顯示哪些 line
//                         在 ping-pong + 哪兩個函式在搶。
//    perf stat -e l2_rqsts.demand_data_rd_miss / cache-misses
//    Intel VTune (Memory Access analysis)
//    比起盲調 alignas,先量再下藥。
//
// 5. 怎麼修
//    A. alignas(64) 把熱資料各自獨佔一條 line:
//         struct alignas(64) Slot { atomic<long> v; };
//         vector<Slot> stats(N);
//    B. 用 std::hardware_destructive_interference_size (C++17) 取代寫死 64:
//         struct alignas(std::hardware_destructive_interference_size) Slot { ... };
//       但 libstdc++ 對這個常數有過 ABI 警告,某些版本會吐 warning。
//    C. 把熱欄位與冷欄位 *分開放* (cold/hot split)。
//    D. local accumulator pattern:每 thread 累加在 stack local 變數,
//       結束時把結果合到全域 → 完全避免共享熱資料。
//
// 6. constructive vs destructive interference size
//    std::hardware_destructive_interference_size:你「不希望共享」的最小
//                                                 對齊 (避免 false sharing)。
//    std::hardware_constructive_interference_size:你「希望共用」的最大
//                                                  範圍 (例如把相關欄位放
//                                                  同一條 line 提升 prefetch)。
//    兩者多數平台都是 64,但意義不同。
//
// 7. 為什麼別 *對所有東西* 都 alignas(64)
//    每個 atomic<int> 變 64 bytes → cache footprint 暴增 → L1 命中率下降。
//    只有 *跨 thread 寫* 的熱欄位才需要,單 thread 用的不需要。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── false sharing 是微觀效能
//     議題,LC 評分只看正確不看 latency。
//   → 但 lesson 16 SPSC ring 與 lesson 17 sharded MPMC 的效能,直接
//     依賴本課的觀念。LC 1242 Web Crawler 若 visited set 與 in_flight
//     計數沒拉開到不同 cache line,scaling 會被 false sharing 卡住。
//
// 主要 API 對照 (cppreference):
//   - alignas (對齊指定符)              https://en.cppreference.com/w/cpp/language/alignas
//   - alignof                           https://en.cppreference.com/w/cpp/language/alignof
//   - std::hardware_destructive_interference_size (C++17)
//                                       https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
//   - std::hardware_constructive_interference_size
//                                       同上
//
// 練習建議:
//   - 拿 lesson 17 sharded MPMC 把每個 shard 的 mutex 與 queue 是否
//     落在同一 cache line 印出來,用 perf c2c 驗證。
//   - 進階:把 lesson 30 Q8 Web Crawler 的 in_flight (int) 換成
//     alignas(64) 的版本,觀察 4-8 worker 時的 scaling。
// =============================================================


/*
補充筆記：false_sharing
  - false sharing 是效能問題，不是資料正確性問題；兩個 thread 可能完全沒有 data race，仍因為寫到同一條 cache line 而互相拖慢。
  - CPU cache coherence 的同步單位通常是 cache line，不是單一變數；兩個獨立 atomic 若落在同一條 line，寫入仍會讓 line 在核心間來回失效。
  - alignas(64) 或 std::hardware_destructive_interference_size 的目的，是把頻繁寫入的 per-thread slot 分開到不同 cache line。
  - padding 只應用在熱點資料；對所有 struct 盲目補齊 cache line 會浪費記憶體並降低 cache locality。
  - std::atomic 保證原子操作，不保證自動避開 false sharing；atomic 變數彼此相鄰仍可能共享 cache line。
  - 量測 false sharing 時要開最佳化並避免 I/O 混入 benchmark，否則看到的是輸出成本或 debug build 成本。
  - C++17 的 std::hardware_destructive_interference_size 定義在 <new>，但不同標準庫支援程度與數值可能不同，必要時仍要有平台 fallback。
  - 修 false sharing 前要先用 profiler 或對照實驗確認瓶頸；它常見但不是所有多執行緒慢速的原因。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】False sharing 與 cache line
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 false sharing？如何解決？
//     答：兩個 thread 各自寫入「邏輯上無關」的變數，但它們落在同一條 cache line 上。
//         由於 cache 一致性協定（MESI）以 cache line 為單位，每次寫都會使對方 core 的
//         該 line 失效，造成來回搬運（ping-pong），效能可能掉數倍。解法：用
//         alignas(std::hardware_destructive_interference_size)（C++17）或保守的
//         alignas(64) 把熱點變數各自對齊到獨立 cache line；或加 padding；或最徹底的
//         ——改成 thread-local 累加、最後才合併。
//     追問：如何確認真的是 false sharing？（perf c2c、perf stat 看 cache-miss，或把
//           padding 加上去看是否變快）
//
// 🔥 Q2. cache line 是 64 bytes 嗎？
//     答：x86-64 與多數目前常見的平台上是 64 bytes，但這是「硬體／實作定義」的值，
//         C++ 標準並未規定。標準提供的是
//         std::hardware_destructive_interference_size（C++17）作為可攜的查詢方式，
//         應該用它而不是硬編數字。
//     追問：destructive 與 constructive interference size 差在哪？（前者是「要分開的
//           最小距離」，後者是「可放在一起共享的最大範圍」）
//
// ⚠️ 陷阱. struct { std::atomic<long> a; std::atomic<long> b; }; 兩個 thread 各自狂寫
//     a 和 b，為什麼可能比單執行緒還慢？
//     答：False sharing。a 與 b 相鄰、共 16 bytes，必定落在同一條 64-byte cache line。
//         兩個 core 各自寫入時 MESI 不斷讓對方失效，每次寫都變成跨核同步（數十到上百
//         cycle）。邏輯上完全正確、也沒有 data race，但硬體層面它們在爭用同一個資源。
//     為什麼會錯：大家的心智模型停在「不同變數 = 互不干擾」，只檢查了正確性層面的
//         獨立性，忽略了記憶體是以 cache line 為單位在核心之間搬運的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <new>          // std::hardware_destructive_interference_size

// -------------------------------------------------------------
// CACHE_LINE 的選擇
//
// 標準提供了:
//     std::hardware_destructive_interference_size
// 它是「應避免被擺在同一條 cache line 的最小距離」。但 GCC
// 對它有 ABI 警告 (數值可能在不同 GCC 版本之間變動),且某些
// 架構上會產生奇怪的值。實務上,絕大多數 C++ 程式碼直接寫
// alignas(64),理由是:
//   - x86_64 上幾乎一律 64 bytes。
//   - 即使在 128-byte line 的 ARM 上,padding 到 64 bytes
//     仍然 *減少* 了 false sharing,只是沒完全消除。
//   - 相容性、可讀性、可移植性都比較好。
//
// 真要追求嚴謹,可以這樣寫:
//     #ifdef __cpp_lib_hardware_interference_size
//       constexpr std::size_t CACHE_LINE =
//           std::hardware_destructive_interference_size;
//     #else
//       constexpr std::size_t CACHE_LINE = 64;
//     #endif
// 為了示範簡潔,這裡直接用 64。
// -------------------------------------------------------------
constexpr std::size_t CACHE_LINE = 64;

// =============================================================
// 兩種版本的 counter
// =============================================================

// 「壞」版:兩個 atomic 緊鄰,擠在同一條 cache line。
// 兩個執行緒分別寫 a 與 b 看似互不影響,但 cache line 會在
// 兩顆核心之間 ping-pong,效能崩塌。
struct CountersAdjacent {
    std::atomic<long long> a{0};
    std::atomic<long long> b{0};
};

// 「好」版:強制每個 counter 放在自己的 cache line 起點。
// alignas(CACHE_LINE) 同時處理「該欄位本身對齊」與「下一個
// 欄位被推到下一條 line」這兩件事。
struct CountersPadded {
    alignas(CACHE_LINE) std::atomic<long long> a{0};
    alignas(CACHE_LINE) std::atomic<long long> b{0};
};

// =============================================================
// Benchmark:兩個 thread 各自把自己那邊的 counter 加好加滿
// =============================================================
template <typename T>
static long long bench(const char* label, T& c, int iters)
{
    auto t0 = std::chrono::steady_clock::now();

    std::thread t1([&]{
        for (int i = 0; i < iters; ++i)
            c.a.fetch_add(1, std::memory_order_relaxed);
    });
    std::thread t2([&]{
        for (int i = 0; i < iters; ++i)
            c.b.fetch_add(1, std::memory_order_relaxed);
    });

    t1.join();
    t2.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();
    std::cout << "[" << label << "] " << ms << " ms"
              << "  (a=" << c.a.load() << ", b=" << c.b.load() << ")\n";
    return ms;
}

int main()
{
    // 印出實際結構大小,讓 padding 的效果一目了然。
    std::cout << "sizeof(CountersAdjacent) = "
              << sizeof(CountersAdjacent) << " bytes\n";
    std::cout << "sizeof(CountersPadded)   = "
              << sizeof(CountersPadded)   << " bytes  "
              << "(2 x " << CACHE_LINE << " 對齊)\n\n";

    constexpr int ITERS = 50'000'000;

    // 跑兩次取平均不必要,差距夠明顯,各跑一次即可。
    CountersAdjacent ca;
    CountersPadded   cp;

    auto t_bad  = bench("adjacent (false sharing)", ca, ITERS);
    auto t_good = bench("padded   (no  sharing)  ", cp, ITERS);

    std::cout << "\nspeedup from cache-line padding = "
              << (static_cast<double>(t_bad) /
                  static_cast<double>(t_good))
              << "x\n";

    // ---------------------------------------------------------
    // 實戰 1: per-thread shard 計數器 (避免 true + false sharing)
    // ---------------------------------------------------------
    // 應用場景: 統計每條 thread 處理的事件數, 最後加總成全域
    // 結果。如果用一個共享 atomic, true sharing 把你打爆; 如果
    // 用 std::vector<atomic<long>> 但沒 padding, false sharing 把
    // 你打爆。正確做法: 每個 shard 用 cache-line 對齊的 struct,
    // 各 thread 寫自己 shard, 結束後再 reduce 加總。
    //
    // 這個 pattern 在 jemalloc、Folly Histogram、tcmalloc 都看得
    // 到。重點是 alignas(CACHE_LINE) 必須既對齊 *也* 撐大到 ≥
    // 一條 cache line, 否則陣列相鄰的兩個 shard 仍在同一 line。
    // ---------------------------------------------------------
    {
        std::cout << "\n[demo] per-thread shard counter (padded)\n";
        struct alignas(CACHE_LINE) Shard {
            std::atomic<long long> v{0};
            char pad[CACHE_LINE - sizeof(std::atomic<long long>)];
        };
        constexpr int T = 4;
        std::vector<Shard> shards(T);

        std::vector<std::thread> ts;
        for (int t = 0; t < T; ++t) {
            ts.emplace_back([&, t]{
                for (int i = 0; i < 1'000'000; ++i)
                    shards[t].v.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& th : ts) th.join();

        long long total = 0;
        for (auto& s : shards) total += s.v.load();
        std::cout << "  per-shard total = " << total << " (預期 4000000)\n";
    }

    // ---------------------------------------------------------
    // 實戰 2: producer/consumer ring 的 head/tail 必須分 line
    // ---------------------------------------------------------
    // 應用場景: SPSC ring buffer (lesson 16) 有兩個 atomic 指標:
    //   - head: 由 producer 寫, consumer 只讀
    //   - tail: 由 consumer 寫, producer 只讀
    // 如果這兩個 atomic 並排放在同一個 struct, 它們會擠在同一
    // 條 cache line, 然後 producer 寫 head 的時候會 invalidate
    // consumer 端的 cache, 即使 consumer 只是想讀 tail。結果
    // ring 效能掉 5-10×。這就是 lesson 16 為什麼要 alignas(64)。
    //
    // 這裡示範 sizeof 對比, 不跑 benchmark (lesson 16 有完整版)。
    // ---------------------------------------------------------
    {
        struct RingNaive {
            std::atomic<size_t> head{0};
            std::atomic<size_t> tail{0};
        };
        struct RingPadded {
            alignas(CACHE_LINE) std::atomic<size_t> head{0};
            alignas(CACHE_LINE) std::atomic<size_t> tail{0};
        };
        std::cout << "\n[demo] SPSC ring head/tail layout\n";
        std::cout << "  sizeof(RingNaive)  = " << sizeof(RingNaive)
                  << " (head/tail 同一 line, false sharing)\n";
        std::cout << "  sizeof(RingPadded) = " << sizeof(RingPadded)
                  << " (head/tail 各佔一 line, 正確設計)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：cache line 多大? 各平台一致嗎?
    //    A：x86-64 (Intel/AMD) 是 64 bytes, ARM Cortex 大多 64 bytes,
    //       Apple Silicon (M1/M2/M3) 是 128 bytes (對齊 P-core), PowerPC
    //       常見 128。可移植寫法用 std::hardware_destructive_interference
    //       _size (C++17), 但 libstdc++ 可能回傳 64 但跑在 Apple 機器
    //       上不夠, 保險起見手動 alignas(128) 對主流平台都安全。
    //
    //  Q2：怎麼正確 padding 一個 per-thread counter?
    //    A：alignas(64) struct Counter { std::atomic<long> v; char pad
    //       [64 - sizeof(v)]; };。或更簡單: alignas(64) std::atomic
    //       <long> v; 後面留空白。注意 array<Counter, N> 才有意義 ──
    //       每個 Counter 起點對齊 cache line, 且大小 ≥ 64 才不會兩個
    //       擠進同一 line。alignof 不夠, 還要保證 sizeof ≥ cache line。
    //
    //  Q3：false sharing 跟 true sharing 差在哪?
    //    A：true sharing: 兩條 thread 真的在同一個變數上競爭 (例如同一
    //       個 atomic counter), 解法是 sharding 或 lock-free 演算法。
    //       false sharing: 兩條 thread 各寫各的變數, 但這兩個變數剛好
    //       住在同一條 cache line, 硬體誤以為衝突而強制同步。解法是
    //       padding。perf c2c 工具能幫你定位 cache line bouncing 熱點。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 兩個變數在邏輯上「不相干」,並不代表它們在硬體上不相干。
//    只要它們落在同一條 cache line,*寫* 操作就會在核心之間
//    來回搬 cache line,把 latency 與 throughput 都吃光。
//
// 2. False sharing 最常見的「中獎場景」:
//      - 一個結構內幾個會被不同執行緒頻繁寫入的計數器
//      - vector / array 裡相鄰的「per-thread slot」
//      - 執行緒池中每個 worker 的本地統計、queue head/tail
//      - 反覆寫入的 std::atomic 旗標,旁邊還住著別的東西
//
// 3. 解法:讓每個熱點變數獨佔一條 cache line。
//      - 欄位:alignas(64) std::atomic<...> x;
//      - 陣列:struct alignas(64) Slot { ... };  std::vector<Slot>;
//      - 隊形:在欄位之間加上 char pad[64 - sizeof(...)] 也行,
//        但 alignas 更乾淨。
//
// 4. *Reads* 不會引發 false sharing。只讀的共享變數放在同一
//    條 line 反而更省記憶體頻寬。問題只發生在「至少一邊在
//    寫入」時。
//
// 5. 不要過度 padding。每個變數獨佔 64 bytes 的代價是 cache
//    使用率下降。原則:會被多執行緒寫的熱點 padding,其他
//    照常壓緊。
//
// 6. 怎麼確認你中標了?
//      - 用 perf c2c (Linux) 直接列出哪些 cache line 在跨核
//        反覆 ping-pong。
//      - 用 perf stat 看 HITM / cache-misses 兩個事件,跑前
//        後比對。
//      - 在程式裡擺 std::atomic 計數器,padding 前後跑同一個
//        微基準,看時間差,就像本課做的一樣。
//
// 7. 一個常見的 bonus 陷阱:std::atomic<T> 本身對齊規則不一定
//    把它推到 cache line 邊界。它的 alignof 通常等於
//    sizeof(T)。所以 std::atomic<int> 在結構裡是 4-byte 對齊,
//    跟 false sharing 完全沒關係。需要的話自己加 alignas。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 13_false_sharing.cpp -o 13_false_sharing

// === 預期輸出 ===
// sizeof(CountersAdjacent) = 16 bytes
// sizeof(CountersPadded)   = 128 bytes  (2 x 64 對齊)
//
// [adjacent (false sharing)] 1399 ms  (a=50000000, b=50000000)
// [padded   (no  sharing)  ] 802 ms  (a=50000000, b=50000000)
//
// speedup from cache-line padding = 1.74439x
//
// [demo] per-thread shard counter (padded)
//   per-shard total = 4000000 (預期 4000000)
//
// [demo] SPSC ring head/tail layout
//   sizeof(RingNaive)  = 16 (head/tail 同一 line, false sharing)
//   sizeof(RingPadded) = 128 (head/tail 各佔一 line, 正確設計)
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
