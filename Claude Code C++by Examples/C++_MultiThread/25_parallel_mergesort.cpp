// =============================================================
// 25_parallel_mergesort.cpp  --  自己寫一個 parallel mergesort
// =============================================================
//
// 本課目標:
//   1. 用 fork-join 的方式手刻 parallel mergesort:遞迴切兩半,
//      其中一半開新執行緒去排,另一半留在當前執行緒排,然後
//      merge。
//   2. 用「深度上限」(parallel_depth) 阻止無止盡開新執行緒 ──
//      每多一層遞迴,並行度只會多一倍,真實機器 8–16 顆核心
//      之後再開更多執行緒只會浪費。
//   3. 把結果跟 std::sort (序列) 與 std::sort + std::execution::par
//      (lesson 09 的 parallel STL) 比較。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 25_parallel_mergesort.cpp -ltbb \
//         -o 25_parallel_mergesort
//   (-ltbb 給 lesson 09 的 std::execution::par 用)
//
// 執行方式:
//     ./25_parallel_mergesort
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     手刻 parallel mergesort ── fork-join 模式
// 前置課程: lesson 06, 09
// 觀念詞彙:
//   - fork-join         ── 上層分支、下層合併
//   - parallel depth    ── 上層幾層用平行,下層回到序列
//   - base case         ── 序列演算法處理小區段
//   - parallel merge    ── 把 merge 步驟也平行化 (本課未做)
// 新介紹 API:
//   std::async(launch::async, fn)    最簡單的 fork
//   future.get()                     等待子任務完成
//   std::sort                        小範圍 base case 的好選擇
// 何時手刻 parallel mergesort:
//   - 學習 fork-join 模式 (BFS、quicksort、tree 都是同個樣)
//   - 沒 parallel STL 後端的環境
// 何時直接用 std::sort + execution::par:
//   - 一般情況下,parallel STL 內部 work-stealing 比 std::async 聰明
//   - 想要 *最好* 的效能,不想手刻
// 常見錯誤:
//   - parallel_depth 沒限制 → 開太多 thread,context switch 暴增
//   - base case 太小 → fork overhead 比工作還多
//   - merge 步驟沒平行化 → 上層 fork 紓解不了下層 merge 的瓶頸
//   - 子 future 沒 .get() → 父 thread 可能比子先結束,UB
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── fork-join 與分治平行化心法
// =============================================================
//
// 1. fork-join 模型
//    遞迴算法 (mergesort、quicksort、tree traversal、map-reduce) 自然
//    切成「分」 (fork) + 「合」 (join) 兩階段:
//        sort(L) {
//            if (small) base_case;
//            fork sort(L1);    fork sort(L2);    join;    merge;
//        }
//    每個 fork 是獨立子問題,可平行執行;join 等所有子完成才繼續。
//
// 2. 為什麼 std::async 比 std::thread 適合 fork-join
//    std::thread 每次 fork 都實際開新 OS thread (~10-50 µs)。深度
//    log2(10M) ≈ 24,光啟動 thread 就花 ~24 × 10 = 240 µs × 每節點不止
//    一條 → 啟動成本主導。
//    std::async 在 launch::async 下,實作可能用內部執行緒池重用 thread
//    (libstdc++ 沒做,但概念上正確)。配合 base case 早早退出遞迴,
//    fork 數量降到合理範圍。
//    更好:用自己的 thread pool (lesson 08),fork = pool.submit。
//
// 3. depth limit 的必要性
//    沒限制深度,1000 萬筆數的 mergesort 會 fork 出指數級任務。例:
//    1000 萬切到 base=64 = 1000 萬 / 64 ≈ 16 萬 fork。每 fork 的 future
//    + heap allocation → 記憶體爆 + queue 爆。
//    解法:限定深度 D = log2(核數) + 2。深度 ≤ D fork,深度 > D 序列。
//    例如 8 核 → D=5,fork 出 32 個任務剛好餵滿 + 一點 buffer。
//
// 4. base case threshold
//    遞迴到多大才直接序列?經驗值:
//      - mergesort: 4k - 64k 筆 (整個放得進 L2 cache)
//      - quicksort: 32 - 256 筆 (insertion sort 反而快)
//      - 矩陣乘:32x32 - 64x64 (一個 cache block)
//    需 benchmark 找最佳值,且不同 CPU 不同。
//
// 5. merge 步驟為什麼是 mergesort 的瓶頸
//    最後一輪 merge 兩條長度 N/2 的序列 → O(N) 工作,在 *單一 thread* 上跑。
//    理想加速比 = 核數,實測 4-6× ── 因為最後一層 merge 拖。
//    解法:平行 merge (Cilk-style):兩條序列以中位數切兩半,交叉 merge。
//    複雜度 O(N) 不變但平行度從 1 提升到 log N。lesson 25 沒做,因為
//    程式碼複雜度跳一個量級。
//
// 6. memory bandwidth 是另一個天花板
//    sort 5M ints (每個 4 byte) = 20 MB,完全在 L3 範圍。多次 pass
//    讀寫 ── 8 條 thread 各 ~20 GB/s = 160 GB/s,超過 DDR4 上限 (~40 GB/s)。
//    所以實際加速比達到 6-8× 後就不再上升。
//    對策:
//      - 設計 cache-oblivious 演算法,使中間結果留在 cache
//      - 用 SIMD merge 提高 per-thread 吞吐
//      - 分多 socket → 用 NUMA-aware 配置
//
// 7. 與 std::sort par STL (lesson 09) 比較
//    par STL 是 *調好的* mergesort/sample-sort 混合,且後端 (TBB) 用
//    work-stealing。手寫版的價值在 *理解內部*,實戰幾乎一定輸。
//    本課 demo 結果:5M ints,std::sort 601 ms / par STL 55 ms (10.9×) /
//    自製 144 ms (4.17×)。par STL 贏不只是效率,也是工程細節 (cache-aware
//    merge、work-stealing 平衡)。
//
// 8. 為什麼 quicksort 比 mergesort 較少被平行化
//    - quicksort 的 pivot 選不好會極不平衡,平行加速波動大
//    - quicksort in-place,平行化要小心 partition 的覆寫順序
//    - mergesort 自然分治,merge 是純合併不依賴 pivot
//    工業上純整數 sort 反而流行 *radix sort* (per-byte counting),
//    更容易平行且 O(N) 而非 O(N log N)。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目*。
//   → 「不引入刁鑽算法題」的方針排除 LC 912 Sort an Array、LC 215 Kth
//     Largest Element 這類純算法題。本課的價值不在「會不會 mergesort」,
//     而是看懂 fork-join、depth limit、base case 三件事。
//   → 若想把這些觀念套到 LC 上,可挑 LC 23 Merge K Sorted Lists 練手 ──
//     單緒解是用 priority_queue,N 條時可平行 K-way merge。
//
// 主要 API 對照 (cppreference):
//   - std::sort                         https://en.cppreference.com/w/cpp/algorithm/sort
//   - std::stable_sort                  https://en.cppreference.com/w/cpp/algorithm/stable_sort
//   - std::merge / std::inplace_merge   https://en.cppreference.com/w/cpp/algorithm/merge
//   - std::async                        https://en.cppreference.com/w/cpp/thread/async
//   - std::execution::par               https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag
//
// 練習建議:
//   - 對比本課手刻版與 std::sort(par, ...) 在 1M / 5M / 50M ints 的
//     效能變化。觀察「parallel STL 內建調好,自己手寫追平就值得驕傲」。
// =============================================================

/*
補充筆記：parallel_mergesort
  - parallel_mergesort 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - parallel_mergesort 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>
#include <future>
#include <thread>
#include <chrono>
#include <random>

// -------------------------------------------------------------
// 序列 mergesort (in-place 我們不做,用 buffer 比較簡單)
// -------------------------------------------------------------
template <typename It>
static void merge(It lo, It mid, It hi, std::vector<typename It::value_type>& buf)
{
    buf.clear();
    buf.reserve(hi - lo);
    auto a = lo, b = mid;
    while (a != mid && b != hi) {
        if (*a <= *b) buf.push_back(*a++);
        else          buf.push_back(*b++);
    }
    while (a != mid) buf.push_back(*a++);
    while (b != hi)  buf.push_back(*b++);
    std::move(buf.begin(), buf.end(), lo);
}

template <typename It>
static void seq_msort(It lo, It hi, std::vector<typename It::value_type>& buf)
{
    auto n = hi - lo;
    if (n <= 1024) {
        // 小區段直接用 std::sort ── std::sort 在小範圍上是
        // introsort,通常比手寫 base case 還快。
        std::sort(lo, hi);
        return;
    }
    auto mid = lo + n / 2;
    seq_msort(lo, mid, buf);
    seq_msort(mid, hi, buf);
    merge(lo, mid, hi, buf);
}

// -------------------------------------------------------------
// Parallel mergesort:在遞迴的「上層」開 std::async 平行做,
// 到 parallel_depth 為 0 後切回序列版。
// -------------------------------------------------------------
template <typename It>
static void par_msort(It lo, It hi, int depth)
{
    auto n = hi - lo;
    if (n <= 1024 || depth == 0) {
        std::vector<typename It::value_type> buf;
        seq_msort(lo, hi, buf);
        return;
    }

    auto mid = lo + n / 2;
    auto fut = std::async(std::launch::async, [=]{
        par_msort(lo, mid, depth - 1);
    });
    par_msort(mid, hi, depth - 1);
    fut.get();

    // 兩半都各自排好了 → 合併。merge 自己也可以平行化
    // (parallel merge),但這條 demo 不展開。
    std::vector<typename It::value_type> buf;
    merge(lo, mid, hi, buf);
}


int main()
{
    constexpr std::size_t N = 5'000'000;

    // 用一份固定的隨機資料,給每個演算法各排一份副本。
    std::vector<int> base(N);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1'000'000'000);
    for (auto& x : base) x = dist(rng);

    // 並行深度:dlog2(hardware_concurrency()) 取整
    int n_cpus = std::max(1u, std::thread::hardware_concurrency());
    int depth  = 0;
    while ((1 << depth) < n_cpus) ++depth;
    std::cout << "hardware_concurrency = " << n_cpus
              << ", parallel_depth = "      << depth
              << " (= log2 ceil)\n\n";

    auto bench = [&](const char* label, auto&& fn) {
        auto v = base;     // 每次都拿一份新的副本來排
        auto t0 = std::chrono::steady_clock::now();
        fn(v);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();
        bool ok = std::is_sorted(v.begin(), v.end());
        std::cout << "[" << label << "] " << ms << " ms   "
                  << (ok ? "sorted OK" : "NOT SORTED!") << '\n';
        return ms;
    };

    auto t_seq = bench("std::sort (seq)        ",
        [](auto& v){ std::sort(v.begin(), v.end()); });

    auto t_par_stl = bench("std::sort + par (TBB)  ",
        [](auto& v){ std::sort(std::execution::par, v.begin(), v.end()); });

    auto t_my = bench("our par_msort          ",
        [&](auto& v){ par_msort(v.begin(), v.end(), depth); });

    std::cout << "\nspeedup vs std::sort:\n"
              << "  parallel STL : "
              << (static_cast<double>(t_seq) / t_par_stl) << "x\n"
              << "  our par_msort: "
              << (static_cast<double>(t_seq) / t_my)      << "x\n";

    // ---------------------------------------------------------
    // 實戰範例: fork-join 平行 sum (簡單版 mergesort 結構)
    // ---------------------------------------------------------
    // 應用場景: mergesort 的 fork-join 結構不只能拿來排序, 對
    // *任何* 可分而治之的工作 (reduce、search、tree traversal)
    // 都通用。本範例展示「平行求和」── 把 vector 對半切, 各自
    // 用 std::async 算, 合併時相加。等同於 std::reduce(par)
    // 但能讓你看清 fork-join 的最小骨架。
    //
    // base case: chunk 小於 50k 直接序列加, 不再 fork。
    // ---------------------------------------------------------
    {
        std::cout << "\n[demo] fork-join 平行 sum (與 mergesort 同骨架)\n";
        std::vector<int> v(1'000'000, 1);   // sum 應為 1,000,000

        // 標準 lambda 回呼 (depth = 限制 fork 深度)
        std::function<long long(int*, int*, int)> par_sum =
            [&](int* lo, int* hi, int d) -> long long {
                if (d == 0 || (hi - lo) < 50'000) {
                    long long s = 0;
                    for (int* p = lo; p != hi; ++p) s += *p;
                    return s;
                }
                int* mid = lo + (hi - lo) / 2;
                // 右半交給另一條 thread (fork)
                auto right_fut = std::async(std::launch::async,
                                            par_sum, mid, hi, d - 1);
                // 左半就在當前 thread 跑 (省一條 thread)
                long long left = par_sum(lo, mid, d - 1);
                return left + right_fut.get();   // join + 合併
            };

        auto t0 = std::chrono::steady_clock::now();
        long long sum = par_sum(v.data(), v.data() + v.size(), depth);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();
        std::cout << "  sum=" << sum
                  << " (預期 " << v.size() << "), wall=" << ms << " ms\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：base case 切到多細才是有效的?太細為什麼反而變慢?
    //    A：經驗值 mergesort base case 設 1k-64k 筆 (整個放得進 L2 cache,
    //       std::sort 的 introsort 對小範圍最有效率)。切太細:每個 fork
    //       要 ~10-50 µs (heap alloc future + 排隊 + cache thrash),工作
    //       本身可能只要 1 µs → fork overhead 大於工作。切太粗:平行度不夠,
    //       閒 core 浪費。實作上「base case = N/(P × 8)」是個合理起點,
    //       讓 P 個核每核 ~8 個 task 給 work-stealing 平衡。
    //
    //  Q2：為什麼 parallel STL (-ltbb) 通常比手刻版快?work-stealing 是什麼?
    //    A：std::async 是 fork-and-wait 模型,每次 fork 開新 OS thread (~10 µs),
    //       且若某條 thread 工作早早做完只能閒著。TBB 內部維護 per-thread
    //       deque,每個 worker 從自己 deque 後端 push/pop 工作 (LIFO,cache
    //       友善);閒的 worker 從 *別人 deque 前端* 偷工作 (FIFO,搶最大、
    //       cache 較冷)。這就是 work-stealing。結果:idle 時間趨近 0、
    //       cache locality 好、爭用低,通常比手寫快 1.5-3×。
    //
    //  Q3：parallel mergesort 的最後一輪 merge 為什麼是瓶頸?
    //    A：兩條長度 N/2 的序列 merge 是 O(N),且預設只在「一條 thread」
    //       上跑 → 不論前面平行得多漂亮,最後這段把 speedup 拉回 ~序列。
    //       Cilk-style parallel merge 解法:對序列 A 取中位數 m,在 B 上
    //       binary search 找 m 的位置,把 A、B 各切兩半,*交叉*遞迴 merge。
    //       複雜度 O(N) 不變,但平行度從 1 提升到 log N。實作複雜度跳一
    //       個量級,本課省略,但這就是 par STL 真正贏的最後一塊。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. Fork-join (mergesort 是經典範例) 的鐵律:
//      - 上層平行,下層序列。下層工作太小,平行的 overhead
//        (建 thread + atomic counter + cache thrashing) 比
//        節省的時間還多。
//      - 「平行度上限」由硬體決定。超過 hardware_concurrency()
//        再開更多 worker 只會 *變慢* (context switch 暴增)。
//
// 2. 我們手寫的 par_msort 通常會 *比* std::sort + execution::par
//    略慢一點。原因:
//      - parallel STL 內部 (TBB) 用的 work-stealing scheduler
//        比 std::async 更聰明,能更平均分擔。
//      - parallel STL 內部用了 in-place 比較 + 更高效的 base
//        case。我們的 demo 為了清楚用了臨時 buffer。
//      - merge 那一步我們是 *序列* 跑的;真正高效的 parallel
//        mergesort 會做 parallel merge 把這段也平行化。
//
// 3. 為何還是要學會手刻?
//      - parallel STL 對演算法形狀有限制 (必須是「對 range
//        做純函式 callback」),很多真實工作不貼合。
//      - 學會 fork-join + std::async 的模式之後,寫 quicksort、
//        BFS、map-reduce、tree 處理通通是同個樣子。
//      - parallel STL 在某些平台沒有後端 (lesson 09 的 -ltbb
//        坑),手寫的反而通用。
//
// 4. 觀察 parallel mergesort 的 *理論* 加速:
//      序列複雜度 O(N log N)
//      P 個處理器,「上層 log P 層完全平行,merge 是 O(N)」:
//        T_par = O((N log N)/P + N log P)
//        speedup = T_seq / T_par ~ P / (1 + (N log P)/(N log N))
//      實務上 N=5M、P=8 時 speedup 大約 4–6x ── 我們觀察到
//      的就在這附近。
//
// 5. 改良路線 (沿著教學堆疊)::
//      (a) 把 std::async 換成 lesson 08 的 ThreadPool,降低
//          每次「開 thread」的 ~10µs 開銷。
//      (b) parallel merge:把兩段再用 binary search 切成 P
//          段,每段獨立 merge。複雜但能榨出最後一倍。
//      (c) 用 lesson 16 的 SPSC 做 streaming pipeline,讓上
//          下游同時跑。
// =============================================================
