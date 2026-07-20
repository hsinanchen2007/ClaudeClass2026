// =============================================================
// 09_parallel_stl.cpp  --  Parallel STL (C++17 執行策略)
// =============================================================
//
// 本課目標:
//   1. 透過在演算法的第一個參數傳入 std::execution 策略,
//      使用 std 演算法的平行版本。
//   2. 理解四種策略,以及各自合法的使用情境。
//   3. 知道你傳入的 callback 必須遵守哪些規則 (不可有資料
//      競爭;par_unseq 下不可加鎖)。
//   4. 學到 g++/libstdc++ 上的「連結地雷」:
//        較新的 libstdc++ (gcc 11+):沒加 -ltbb -> *硬性*
//          連結錯誤。會出現幾百行 "undefined reference to
//          tbb::detail::..." 的訊息。
//        較舊的 libstdc++:沒加 -ltbb -> *可以* 編譯且能跑,
//          但默默地變成 *序列* 執行 —— 這是最糟糕的 bug,
//          因為正確性測試都會通過。
//        無論哪種情況,在 g++ 上使用非 seq 策略時都 *永遠*
//        要加 -ltbb。並在大型輸入上比較 seq 與 par,確認
//        加速是真的存在。
//
// 編譯方式:
//
//   裝了 TBB (能真正平行):
//     sudo apt-get install -y libtbb-dev          # 一次性設定
//     g++ -std=c++17 -O2 -pthread 09_parallel_stl.cpp -ltbb \
//         -o 09_parallel_stl
//
//   沒裝 TBB (能編、但是序列執行 —— 也就是「地雷」):
//     g++ -std=c++17 -O2 -pthread 09_parallel_stl.cpp \
//         -o 09_parallel_stl
//
// 執行方式:
//     ./09_parallel_stl
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Parallel STL ── 用 std::execution::par 直接平行化標準演算法
// 前置課程: lesson 04 (atomic 給 callback 內共享狀態用)
// 觀念詞彙:
//   - execution policy  ── 告訴 std 演算法可以怎麼執行
//   - seq               ── 序列,等同沒傳 policy
//   - par               ── 多 thread,callback 不可有 race
//   - par_unseq         ── 多 thread + SIMD,callback 不可加鎖/配置
//   - reduce vs accumulate ── reduce 允許重排,accumulate 不允許
// 新介紹 API:
//   std::execution::seq / par / par_unseq    四種 policy 之三
//   std::reduce(par, b, e, init)              平行版的 accumulate
//   std::transform_reduce(par, b, e, init, op_red, op_xform)
//   std::sort(par, b, e)                       平行排序
//   std::for_each(par, b, e, fn)               平行迴圈
//   std::inclusive_scan(par, ...)              平行 prefix sum
// 何時使用:
//   - 大型 (≥10k) container 上要做純函式式運算
//   - 不想自己手刻 fork-join
// 何時不要用:
//   - 任務粗、長度差異大 → thread pool (lesson 08)
//   - callback 必須加鎖 → 用 par 而非 par_unseq;太多鎖則改 thread pool
// 常見錯誤:
//   - 忘了 -ltbb (libstdc++) → 連結錯誤,或 (舊版本) 默默序列
//   - par_unseq callback 內加鎖 → UB 死鎖
//   - 對小容器使用 par → overhead 比工作還大
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── parallel STL 的工程現實
// =============================================================
//
// 1. 三種執行政策
//    seq          ── 序列,跟沒寫一樣,還比沒寫稍貴 (要走 policy 分派)。
//    par          ── 平行;不同元素會被不同 thread 處理。
//                    callback 內 *允許* 同步 (mutex/atomic),但別寫得太重。
//    par_unseq    ── 平行 + vectorization;callback 內 *禁止* 任何同步
//                    (mutex/atomic 都不可),也不可 throw。違反 → UB。
//                    換來的是編譯器可以 SIMD 化 + 重排迴圈迭代。
//    unseq (C++20) ── 純 vectorization,單 thread 但 SIMD。
//
// 2. libstdc++ 的後端:Intel TBB
//    g++ 12+ 的 parallel STL 直接呼叫 TBB:每個 par 演算法被 TBB 切成
//    blocked_range,放進 TBB 的 work-stealing scheduler。所以:
//      - 你不用自己管 thread,TBB 自動拉滿核心。
//      - 第一次呼叫會建立 tbb global_control,~1-2 ms 啟動成本。
//      - 同 process 內所有 par 共用同一個 TBB 池 (好事:不會爆 thread)。
//    Clang/MSVC 後端不同,效能與 TBB 略有差異,但介面語意一致。
//
// 3. par 的 overhead 何時值得
//    粗略門檻 (現代 x86):工作量 ≥ ~10 µs 才值得 par。低於這個,
//    切 chunk + 喚醒 worker + barrier 同步比工作本身還貴。
//    例:
//      std::accumulate(par, v.begin(), v.end(), 0L)  v.size() < 100k → seq 比較快
//                                                    v.size() ≥ 1M → par 明顯快
//    現代 STL 內部會嘗試「fall back to seq if too small」但別依賴。
//
// 4. memory bandwidth 是常見天花板
//    很多 par 演算法 (transform、reduce、sort 中的 merge 階段) 是
//    *memory-bandwidth bound* ── 不是 CPU 算不過,是 RAM 餵不夠快。
//    DDR4 雙通道大約 30-40 GB/s;若你 8 個 thread 各自掃 4 GB/s,bandwidth
//    就用完了,加更多核也救不了。typical 上限:4-8× 加速,而非 N×。
//
// 5. par_unseq 的兩條鐵律
//    A. callback 不能呼叫任何 *同步* 操作:no mutex, no atomic, no I/O,
//       no throw。違反 = UB。
//    B. callback 不能依賴元素被處理的順序。
//    意思是 par_unseq 適用「element-wise pure function」── 例如把每個
//    int 平方、把 RGB 轉灰階。一旦 callback 要 mutate 共享狀態 → 退回 par。
//
// 6. 何時該用 par,何時手寫 (lesson 25/26)
//    用 par 的時機:
//      - 標準演算法剛好是你要的 (sort、transform、reduce、scan)。
//      - 不想自己管 thread。
//      - 容器 ≥ 100k 元素。
//    手寫的時機:
//      - 演算法不在 STL (例如 quickselect、histogram)。
//      - 你要精確控制 chunk size、affinity、cache 行為。
//      - 你已經有自己的 thread pool 不想又開 TBB。
//    lesson 25/26 示範手寫 mergesort/scan,可與 par 對照觀察。
//
// 7. exception 行為
//    par 演算法的 callback 若 throw → 整個演算法以 std::terminate 結束
//    (對 par_unseq 直接是 UB)。要捕捉就把 try/catch 寫進 callback 內,
//    把例外存到 atomic<exception_ptr> 裡,演算法外重新 rethrow。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── parallel STL 解的是
//     批次運算加速,LC 偏好「邏輯同步」題型。
//   → 但平台/工程實戰常用:LC 912 Sort an Array (一般 sort) 若資料量
//     夠大改用 std::sort(par, ...) 立即拿到 ~10× 加速 (lesson 09 demo)。
//     不過本專案方針是「不引入刁鑽算法題」,有興趣自己玩。
//
// 主要 API 對照 (cppreference):
//   - <execution> 政策標籤              https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag
//   - std::execution::seq               https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t
//   - std::execution::par               同上
//   - std::execution::par_unseq         同上
//   - std::execution::unseq (C++20)     同上
//   - std::sort (parallel 版)           https://en.cppreference.com/w/cpp/algorithm/sort
//   - std::transform_reduce             https://en.cppreference.com/w/cpp/algorithm/transform_reduce
//   - std::for_each                     https://en.cppreference.com/w/cpp/algorithm/for_each
//
// 練習建議:
//   - 對 lesson 30 Q5 BoundedBlockingQueue 的 consumer side,把累加 sum
//     改用 std::transform_reduce(par, ...) 收尾批次計算。
//   - 進階:lesson 25/26 手刻 mergesort/scan 與 parallel STL 對照,
//     觀察手寫版能否追平調好的 par STL。
// =============================================================

/*
補充筆記：parallel_stl
  - parallel_stl 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - parallel_stl 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Parallel STL 與 execution policy（C++17）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 四種 execution policy 差在哪？
//     答：seq 序列執行；par 可用多執行緒，元素存取函式可能在不同 thread 上並行，
//         所以你的 lambda 必須是 thread-safe（可以上鎖）；par_unseq 可平行「且可
//         向量化」，允許在同一個 thread 內交錯執行不同元素的指令，因此 lambda 不可
//         上鎖、不可配置記憶體；unseq（C++20）單執行緒但可向量化。
//     追問：平行演算法拋出例外會怎樣？（呼叫 std::terminate，不會傳播給呼叫端）
//
// 🔥 Q2. std::reduce 與 std::accumulate 差在哪？
//     答：accumulate 保證嚴格由左至右的序列順序，因此無法平行化。reduce 不保證順序
//         與分組方式（要求運算滿足結合律與交換律），所以可以平行／向量化。實務後果：
//         對 double 做 reduce，因浮點加法不滿足結合律，每次執行結果可能有微小差異；
//         對字串串接用 reduce 直接是錯的（不可交換）。
//     追問：需要決定性結果又想平行怎麼辦？（固定分塊數 + 分層歸約，或 Kahan／
//           pairwise summation）
//
// ⚠️ 陷阱. 在 par_unseq 的 lambda 裡用 mutex 上鎖會怎樣？
//     答：可能死鎖。unseq 表示實作可以在「同一個執行緒內」交錯執行不同元素的操作，
//         於是同一個 thread 可能在尚未解鎖時就開始處理下一個元素、再次上同一把鎖 →
//         自我死鎖。標準要求 par_unseq 的 element access function 不得使用需要並行
//         前進保證的操作，即不可上鎖、不可配置記憶體、不可做 IO。要同步請降級用 par。
//     為什麼會錯：多數人把 par_unseq 理解成「par 再快一點」，沒意識到 unseq 改變的是
//         「同一 thread 內指令可交錯」這個更根本的前提。
//
// ⚠️ 陷阱2. 在 g++ 上沒加 -ltbb 就用 par，為什麼很危險？
//     答：較新的 libstdc++ 會直接給連結錯誤（好事）；但在某些較舊的組態下可以編譯、
//         能執行，卻默默退化成序列——正確性測試全部通過，只有效能沒有提升。是否
//         需要 -ltbb 屬於實作細節（libstdc++ 以 TBB 為後端），標準未規定。
//     為什麼會錯：只看「有沒有跑出正確答案」來驗收平行化。應該同時比較 seq 與 par
//         在大輸入下的實際耗時，確認加速真的存在。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <numeric>     // std::iota, std::reduce, std::transform_reduce
#include <algorithm>   // std::sort, std::for_each, std::transform
#include <execution>   // std::execution::seq / par / par_unseq
#include <random>
#include <chrono>
#include <atomic>
#include <cmath>

// -------------------------------------------------------------
// 四種執行策略 (C++17,加上 C++20 的 unseq)
//
// 把下面其中一個當成第一個參數傳給有平行化版本的 std
// 演算法,即可決定它「可以」用什麼方式執行。
//
//   std::execution::seq         —— sequenced (序列)。
//                                  與不傳策略相同。沒有
//                                  執行緒、沒有向量化。
//
//   std::execution::par         —— parallel (平行)。runtime
//                                  可能把工作切到多個執行緒
//                                  上跑。你的 callback 可能
//                                  在 *任何* 執行緒上執行,
//                                  甚至同時許多次,但每次呼叫
//                                  本身仍是一個序列步驟
//                                  (沒有 SIMD 交錯)。
//                                  規則:callback 不能對共享
//                                  狀態造成資料競爭。可以
//                                  使用鎖。
//
//   std::execution::par_unseq   —— parallel + unsequenced。
//                                  既有執行緒又有向量化。
//                                  runtime 可以把同一執行緒
//                                  上的兩次 callback 呼叫
//                                  *交錯* 進行 (SIMD)。
//                                  規則:callback 必須對
//                                  向量化安全。實務上代表:
//                                    不可有 mutex
//                                    不可配置記憶體
//                                    不可做 I/O
//                                    不可呼叫會做以上事情
//                                       的函式
//                                  只能做純算術。
//
//   std::execution::unseq       —— C++20。在 *單一* 執行緒
//                                  內做 unsequenced (SIMD)。
//                                  callback 規則同 par_unseq。
//
// 心智模型:
//   - "seq" = 「就跟普通 for 迴圈一樣」
//   - "par" = 「可以跑在很多執行緒上」
//   - "par_unseq" = 「...而且每個執行緒可以使用 SIMD lane」
//   - "unseq" = 「單一執行緒,但允許 SIMD」
// -------------------------------------------------------------


// 小型碼錶 (與 lesson 06 相同)。
struct Stopwatch {
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    long long ms() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - t0).count();
    }
};

int main()
{
    // 規模要夠大,O(N log N) 排序才量得到時間;在它之上
    // 做歸約 (reduction) 也才會做到實際的工作。
    constexpr std::size_t N = 20'000'000;
    std::vector<int> data(N);

    // 用隨機 int 填滿 (單執行緒做;我們是要測平行演算法,
    // 不是測這段)。
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    for (auto& x : data) x = dist(rng);

    // 留一份未排序的副本,讓每次排序的輸入都一樣。
    std::vector<int> a = data;
    std::vector<int> b = data;
    std::vector<int> c = data;

    // ---------------------------------------------------------
    // PART 1 —— 三種策略下的 std::sort
    // ---------------------------------------------------------
    {
        Stopwatch sw;
        std::sort(std::execution::seq, a.begin(), a.end());
        std::cout << "[sort seq      ] " << sw.ms() << " ms\n";
    }
    {
        Stopwatch sw;
        std::sort(std::execution::par, b.begin(), b.end());
        std::cout << "[sort par      ] " << sw.ms() << " ms\n";
    }
    {
        Stopwatch sw;
        std::sort(std::execution::par_unseq, c.begin(), c.end());
        std::cout << "[sort par_unseq] " << sw.ms() << " ms\n";
    }
    // 如果 seq、par、par_unseq 三者的數字幾乎一樣,代表
    // 你的 build 默默地在跑序列版 —— 你忘了 -ltbb (或者
    // 你的 libstdc++ 找不到 TBB)。請看本檔最上面的編譯
    // 說明。

    // ---------------------------------------------------------
    // PART 2 —— 用 std::reduce 做平行歸約
    //
    // std::accumulate 故意是 *序列* 的:它指定的是 left-fold
    // ((((0+a)+b)+c)+...)。std::reduce 放棄了這個順序保證,
    // 所以 runtime 可以在多個執行緒之間以任意順序累加各段。
    //
    // 副作用:對浮點數,結果可能與 accumulate 略有不同,
    // 因為浮點加法不是嚴格結合律。對整數則完全相同。
    // ---------------------------------------------------------
    {
        Stopwatch sw;
        long long s = std::accumulate(data.begin(), data.end(), 0LL);
        std::cout << "[accumulate seq] " << sw.ms() << " ms  sum=" << s << '\n';
    }
    {
        Stopwatch sw;
        long long s = std::reduce(std::execution::par,
                                  data.begin(), data.end(), 0LL);
        std::cout << "[reduce    par ] " << sw.ms() << " ms  sum=" << s << '\n';
    }

    // ---------------------------------------------------------
    // PART 3 —— transform_reduce,融合的 map + reduce
    //
    // 對每個 x 取平方,再加總。平行進行。這就是 parallel STL
    // 中表達「先逐元素轉換、再做歸約」一次過完成的方式。
    // ---------------------------------------------------------
    {
        Stopwatch sw;
        long long s = std::transform_reduce(
            std::execution::par,
            data.begin(), data.end(),
            0LL,                          // 初值
            std::plus<>{},                // 歸約 (reduce)
            [](int x) { return 1LL * x * x; });   // 轉換 (transform)
        std::cout << "[transform_reduce par] " << sw.ms() << " ms  sum_of_squares=" << s << '\n';
    }

    // ---------------------------------------------------------
    // PART 4 —— callback 的最大規則:不可有資料競爭
    //
    // 正確:
    //   std::atomic<long long> counter{0};
    //   std::for_each(par, v.begin(), v.end(),
    //                 [&](int x){ if (x % 2 == 0) ++counter; });
    //
    // 錯誤 (競爭 —— lesson 02 重來一遍):
    //   long long counter = 0;
    //   std::for_each(par, v.begin(), v.end(),
    //                 [&](int x){ if (x % 2 == 0) ++counter; });
    //
    // 示範正確的寫法。
    // ---------------------------------------------------------
    {
        std::atomic<long long> evens{0};
        std::for_each(std::execution::par,
                      data.begin(), data.end(),
                      [&](int x) { if ((x & 1) == 0) ++evens; });
        std::cout << "[for_each par  ] evens = " << evens.load() << '\n';
    }

    // ---------------------------------------------------------
    // PART 5 —— par 與 par_unseq 之間的「額外」限制
    //
    // par 允許 callback 內使用 mutex。par_unseq *不允許* ——
    // 因為 runtime 可能透過 SIMD 把同一執行緒上的兩次
    // callback 交錯執行。如果兩次都去拿同一把 mutex,你會
    // 把自己鎖死。
    //
    // 示範:一個 CPU bound 的 transform。純算術、沒有共享
    // 狀態、沒有鎖 -> par_unseq 合法,通常還比較快。
    // ---------------------------------------------------------
    std::vector<double> in (N), out(N);
    for (std::size_t i = 0; i < N; ++i) in[i] = static_cast<double>(i);

    {
        Stopwatch sw;
        std::transform(std::execution::par,
                       in.begin(), in.end(), out.begin(),
                       [](double x){ return std::sin(x) * std::sin(x)
                                          + std::cos(x) * std::cos(x); });
        std::cout << "[transform par      ] " << sw.ms() << " ms\n";
    }
    {
        Stopwatch sw;
        std::transform(std::execution::par_unseq,
                       in.begin(), in.end(), out.begin(),
                       [](double x){ return std::sin(x) * std::sin(x)
                                          + std::cos(x) * std::cos(x); });
        std::cout << "[transform par_unseq] " << sw.ms() << " ms\n";
    }

    // ---------------------------------------------------------
    // 實戰範例: parallel filter + count
    // ---------------------------------------------------------
    // 應用場景: 從 20M 筆 log/event 中, 找出符合某條件的數量
    // (e.g. status >= 500 的 error log, 或大於某 threshold 的
    // 監測值)。用 count_if + par 就可以平行化, 不用自己切片。
    //
    // 為什麼快: count_if 的 predicate 是純函式 (無副作用), 完美
    // 滿足 par_unseq 條件; runtime 能把工作切多核並可向量化。
    // ---------------------------------------------------------
    {
        std::cout << "\n[demo] parallel count_if (大於 500000 的元素)\n";
        Stopwatch sw_seq;
        long long n_seq = std::count_if(std::execution::seq,
                              data.begin(), data.end(),
                              [](int x){ return x > 500'000; });
        long long ms_seq = sw_seq.ms();

        Stopwatch sw_par;
        long long n_par = std::count_if(std::execution::par,
                              data.begin(), data.end(),
                              [](int x){ return x > 500'000; });
        long long ms_par = sw_par.ms();
        std::cout << "  seq: " << ms_seq << " ms, n=" << n_seq << '\n';
        std::cout << "  par: " << ms_par << " ms, n=" << n_par
                  << " (兩者數量應該完全一致)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::execution::par 跟 par_unseq 怎麼選?
    //    A：par 保證「對每個元素的 callback」與其他元素不會同時跑,
    //       callback 內仍可以加鎖、做 I/O。par_unseq 額外允許「同一
    //       條 thread 內把多個 callback 交錯/向量化執行」, 所以
    //       callback 內絕對不能加鎖、不能呼叫 vector.push_back, 否則
    //       deadlock 或 UB。預設選 par; callback 是純函式 + SIMD 友善
    //       才升級成 par_unseq。
    //
    //  Q2：libstdc++ 上沒裝 -ltbb 會怎樣?
    //    A：gcc 11+ 是「硬性連結錯誤」, 會看到一坨 tbb::detail::xxx
    //       的 undefined reference, 顯眼好修。但更舊的 libstdc++ 可能
    //       「靜默退化成序列執行」── 程式跑得對但完全沒平行, 是最
    //       糟的偽 bug, 因為功能測試全過。所以使用 par/par_unseq 一定
    //       要 (a) 連 -ltbb (b) 量 benchmark 確認真的有加速。
    //
    //  Q3：什麼時候 parallel STL 反而比 sequential 慢?
    //    A：(a) 資料量太小 (< 10k 元素), thread pool 啟動成本主宰。
    //       (b) callback 太輕 (例如純加法), 反而被 cache miss + 同步
    //       開銷拖死。(c) 元素間有相依 (sort 比較函式有副作用) 觸發
    //       串行化。經驗法則: 元素 ≥ 10⁵ 且 callback 中等以上計算量
    //       才考慮平行, 否則 std::sort 已經夠快。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 要把一個 std 演算法平行化,只要在第一個參數傳入執行
//    策略即可。不必學新函式、不必學新型別 —— 用的還是
//    你已經熟悉的 std::sort、std::for_each、std::transform。
//
// 2. 接受策略的演算法 (列舉一部分):
//      sort, stable_sort, partition, nth_element
//      for_each, transform, copy, fill, generate
//      find, find_if, count, count_if, all_of/any_of/none_of
//      reduce, transform_reduce, exclusive_scan, inclusive_scan
//      adjacent_difference, merge, set_union, set_intersection
//
// 3. callback 規則,從寬到嚴:
//      seq        : 怎樣都行。
//      par        : 不可對共享狀態造成資料競爭。可以加鎖。
//      par_unseq  : 不可加鎖、不可配置記憶體、不可做 I/O。
//                   只能做純(粹的) 算術。
//      unseq      : 與 par_unseq 相同,但只在單一執行緒上。
//
//    違反這些規則屬於未定義行為,而 *不是* 編譯錯誤。
//    編譯器會相信你。
//
// 4. std::accumulate vs. std::reduce。
//      accumulate 依規格就是 *序列* (left fold)。
//      reduce 允許重新安排運算順序,這正是平行求和合法的
//      原因。當運算為結合律 (整數 +、*、min、max、bitwise
//      操作) 時,使用 reduce。浮點 + 在實務上「夠像」結合律,
//      但跨次執行可能會看到些微差異。
//
// 5. 連結地雷 (libstdc++/g++)。
//      平行後端是 Intel TBB。行為依 libstdc++ 版本而異:
//
//        gcc >= 11 (例如 Ubuntu 24.04, gcc 13):
//            沒加 -ltbb  ->  *硬性* 連結錯誤。
//            數百行 "undefined reference to
//            tbb::detail::r1::..." 的訊息。編譯器在這裡
//            幫你救一次 —— 你根本連「默默序列」的 build
//            都做不出來。
//
//        gcc 9-10 與某些 libc++ build:
//            沒加 -ltbb  ->  *默默序列*。能編、能跑、
//            答案正確,卻沒有任何加速。這個版本的地雷
//            害過數不清的工程師。
//
//      防禦性習慣:在大型輸入上對 `seq` 與 `par` 都計時。
//      若數字幾乎一致,代表你的平行後端缺失或設定有問題,
//      不論你用的是哪個編譯器。
//
//      libc++ (clang) 與 MSVC 各自有自己的後端與不同地雷。
//      永遠要量。
//
// 6. 何時該用 Parallel STL。
//      - 輸入夠大 (經驗法則:> ~10k 元素)。在那以下,平行
//        派發的開銷會吃掉收益。
//      - 每個元素的工作量不算太小 (排序、需要實際數學的
//        transform、針對大範圍的歸約)。
//      - 你的 callback 是純函式,或同步寫得正確。
//
//      若任務粒度粗、長度差異大 (執行時間天差地遠),
//      請改用 lesson 08 的執行緒池或真正的執行器
//      (oneTBB、Boost.Asio)。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread -ltbb 09_parallel_stl.cpp -o 09_parallel_stl

// === 預期輸出 ===
// [sort seq      ] 6681 ms
// [sort par      ] 1108 ms
// [sort par_unseq] 1106 ms
// [accumulate seq] 38 ms  sum=10000814108355
// [reduce    par ] 9 ms  sum=10000814108355
// [transform_reduce par] 11 ms  sum_of_squares=6667595483900943037
// [for_each par  ] evens = 9998227
// [transform par      ] 108 ms
// [transform par_unseq] 119 ms
//
// [demo] parallel count_if (大於 500000 的元素)
//   seq: 177 ms, n=10001657
//   par: 16 ms, n=10001657 (兩者數量應該完全一致)
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
