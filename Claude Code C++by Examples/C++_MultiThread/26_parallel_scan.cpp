// =============================================================
// 26_parallel_scan.cpp  --  Parallel prefix sum (scan)
// =============================================================
//
// 本課目標:
//   1. 理解「prefix sum (前綴和)」這個演算法的「依賴鏈」:
//      看似每個位置都依賴前一個,似乎天生序列 ── 但是有個
//      漂亮的「兩階段」演算法可以平行化。
//   2. 寫出最簡單也最實用的「block-based parallel scan」:
//        Phase 1: 每塊獨立算「塊內 prefix sum」與「塊總和」
//        Phase 2: 對「塊總和」做序列 prefix sum,得到每塊
//                 的 offset
//        Phase 3: 每塊獨立把 offset 加到自己每個位置
//      Phase 1 與 3 完全平行;Phase 2 序列但只有 (N/blk) 個
//      項目,通常 ≪ N。
//   3. 跟 std::inclusive_scan 與 std::inclusive_scan + par
//      (lesson 09 風格) 比較。
//
// 為什麼 prefix sum 重要?
//   compaction、bucket sort、parallel quicksort 的 partition、
//   GPU compute 各種 stream 處理、稀疏矩陣的 row pointer
//   建構……許多平行演算法的內層用的就是 scan。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 26_parallel_scan.cpp -ltbb \
//         -o 26_parallel_scan
//
// 執行方式:
//     ./26_parallel_scan
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Parallel prefix sum (scan) ── block-based 兩階段法
// 前置課程: lesson 09
// 觀念詞彙:
//   - prefix sum / inclusive scan ── out[i] = a[0] + ... + a[i]
//   - exclusive scan              ── out[i] = a[0] + ... + a[i-1]
//   - block-based scan            ── 分塊處理 + 序列 carry
//   - up-sweep / down-sweep       ── Blelloch tree 風格 (本課未用)
// 新介紹 API:
//   std::inclusive_scan(b, e, out)        三參數,用 + 與預設 init
//   std::inclusive_scan(par, b, e, out)   平行版本
//   std::exclusive_scan(b, e, out, init)
//   std::transform_inclusive_scan(...)    map 後 scan,合一個 pass
// Block-based scan 三階段:
//   Phase 1: 每塊獨立算 inclusive scan + 塊總和  (完全平行)
//   Phase 2: 對 P 個塊總和做序列 prefix sum   (序列,但 P 很小)
//   Phase 3: 每塊各自加上自己的 offset        (完全平行)
// 何時使用:
//   - 大型 input + 工作量適中的 scan
//   - parallel partition、radix sort、stream compaction 的內層
// 何時不要用:
//   - 小 input → 序列即可
//   - GPU 上有更佳的 Hillis-Steele 變體
// 常見錯誤:
//   - 把 inclusive_scan 與 reduce 混淆 → reduce 不需要逐位置結果
//   - 假設 + 的結合律對浮點完全成立 → 會看到 run-to-run 微差
//   - 只切 2 塊 → 沒充分利用核心
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── 為什麼 scan 比 reduce 還難平行化
// =============================================================
//
// 1. reduce vs scan 的根本差別
//    reduce(in)  → 一個 scalar 結果 (例如 sum、max)。
//    scan(in)    → *與 in 等長* 的陣列,每個位置是「至此為止的累積值」。
//                  inclusive_scan: out[i] = in[0] + in[1] + ... + in[i]
//                  exclusive_scan: out[i] = in[0] + in[1] + ... + in[i-1]
//    reduce 切塊各自加再合一次 (logN 樹) ── 簡單。
//    scan 看似有依賴 (out[i] 需要 out[i-1]) → 一見覺得只能序列。
//    這個直覺錯。scan 可以平行,只是要兩階段。
//
// 2. 兩階段平行 scan (block-based)
//    階段 1 (block reduce, fully parallel):
//        把陣列切成 K 塊,每塊各自序列算內部 scan + 該塊總和。
//        block_sums[k] = 第 k 塊全部和
//    階段 2 (scan over blocks, K 個元素的小 scan):
//        offsets[k] = block_sums[0] + ... + block_sums[k-1]
//    階段 3 (apply offset, fully parallel):
//        把 offsets[k] 加回到第 k 塊的每個元素上
//    全程 O(N) 工作,平行度約 N / chunk_size,實務上 4-8× 加速可達。
//
// 3. 階段 2 為什麼可以序列
//    K 通常等於核數 (~ 8-32),工作量極小。即使序列也只佔總時間 < 1%。
//    若 K 大可遞迴用同一個演算法,但實務沒必要。
//
// 4. associativity 的要求
//    scan 演算法把工作切成多塊重新組合,*交換律不要求*,但 *結合律* 必要。
//      (a + b) + c = a + (b + c)            ✓ scan 用得到
//      a + b = b + a                        ✗ 不要求
//    意涵:
//      整數加法、字串串接、矩陣乘 (順序固定) 都可 scan。
//      浮點加法 *理論上* 結合,*實務上* 不嚴格 (rounding) → 平行 scan 與
//      序列 scan 結果可有 ULP 級差異。對科學運算可能是 bug 來源。
//
// 5. 用途
//    A. 累積總和 / 流量統計 → inclusive_scan
//    B. 用於 stream compaction (filter):先建 mask (0/1),scan 後得到
//       「這個位置該被搬到輸出的第幾格」
//    C. histogram bucket boundary 計算
//    D. 排序中的 radix sort 用 scan 算 prefix sum
//    E. parser / lexer:括號深度、行尾位置
//    GPU programming 把 scan 視為和 reduce 同等重要的基本算子。
//
// 6. inclusive vs exclusive scan
//    很多 stream compaction 演算法要 *exclusive*:目標位置從 0 開始算偏移。
//    例:
//      input  [5, 3, 0, 8, 0]
//      mask   [1, 1, 0, 1, 0]
//      excl   [0, 1, 2, 2, 3]   ← 寫入位置
//    C++17 std::inclusive_scan / std::exclusive_scan 都有,且支援 par。
//
// 7. par STL 的 scan 與自製版差異
//    lesson 26 demo:50M ints inclusive scan,seq 862 ms / par STL 354 ms
//    (~2.4×) / 自製 357 ms。
//    par STL 已調得很好,自製版到平手就值得驕傲了 (證明算法理解到位)。
//    手刻的價值仍在:
//      - 自定義 op (不只是加法)
//      - integrate 進已有 thread pool,避免 TBB 啟動成本
//      - 調 chunk size 應對特殊資料分佈
//
// 8. memory bandwidth 瓶頸 (再次出現)
//    50M ints = 200 MB 讀 + 200 MB 寫 = 400 MB I/O。DDR4 雙通道 ~40 GB/s
//    → 純 RAM 時間 ~10 ms。如果沒有 cache reuse,平行 scan 怎樣都不會
//    快過 ~10 ms。實測 354-357 ms 主因是計算與寫入相對 RAM 不算極端
//    bandwidth-bound (整數 add 很快、寫回時 cache line 友善),仍受 ~40 GB/s
//    bound 一部分。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目*。LC 上 prefix sum 屬算法題
//     (LC 303 Range Sum Query / LC 560 Subarray Sum K),非 concurrency
//     考點。本課關注的是「能不能平行化」這個觀念,LC 不會考。
//
// 主要 API 對照 (cppreference):
//   - std::inclusive_scan (C++17)       https://en.cppreference.com/w/cpp/algorithm/inclusive_scan
//   - std::exclusive_scan (C++17)       https://en.cppreference.com/w/cpp/algorithm/exclusive_scan
//   - std::partial_sum (序列版)         https://en.cppreference.com/w/cpp/algorithm/partial_sum
//   - std::execution::par               https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag
//   - std::reduce / transform_reduce    https://en.cppreference.com/w/cpp/algorithm/reduce
//
// 練習建議:
//   - 想把 prefix sum 套到 LC,可挑 LC 238 Product of Array Except Self ──
//     LC 範例的單緒解是兩次掃 O(N),平行版可拆成 inclusive_scan + reverse
//     scan,各自 par。
// =============================================================

/*
補充筆記：parallel_scan
  - parallel_scan 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - parallel_scan 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/
#include <iostream>
#include <vector>
#include <thread>
#include <numeric>
#include <execution>
#include <chrono>
#include <random>

// -------------------------------------------------------------
// 我們的 parallel inclusive scan
//   in[i] = a0, a1, a2, ..., a_{N-1}
//   out[i] = a0 + a1 + ... + ai
// -------------------------------------------------------------
static void parallel_inclusive_scan(const std::vector<long long>& in,
                                    std::vector<long long>& out,
                                    int n_workers)
{
    const std::size_t N = in.size();
    out.resize(N);
    if (N == 0) return;

    if (n_workers <= 0) n_workers = 1;
    // 切成 n_workers 個 block。每個 block 大小盡量平均。
    std::size_t blk = (N + n_workers - 1) / n_workers;

    std::vector<long long> block_sum(n_workers, 0);   // 每塊的總和

    // ---- Phase 1: 每塊各算自己的 inclusive scan + 總和 ----
    {
        std::vector<std::thread> ts;
        for (int b = 0; b < n_workers; ++b) {
            std::size_t lo = b * blk;
            std::size_t hi = std::min(lo + blk, N);
            if (lo >= hi) continue;
            ts.emplace_back([&, b, lo, hi]{
                long long acc = 0;
                for (std::size_t i = lo; i < hi; ++i) {
                    acc += in[i];
                    out[i] = acc;
                }
                block_sum[b] = acc;
            });
        }
        for (auto& t : ts) t.join();
    }

    // ---- Phase 2: 對 block_sum 做序列 prefix sum,得到 offset ----
    // (n_workers 個項目,通常很小,序列即可)
    long long carry = 0;
    std::vector<long long> block_offset(n_workers, 0);
    for (int b = 0; b < n_workers; ++b) {
        block_offset[b] = carry;
        carry += block_sum[b];
    }

    // ---- Phase 3: 每塊各自把 offset 加到自己每個位置 ----
    {
        std::vector<std::thread> ts;
        for (int b = 1; b < n_workers; ++b) {       // b=0 不需要加
            std::size_t lo = b * blk;
            std::size_t hi = std::min(lo + blk, N);
            if (lo >= hi) continue;
            long long off = block_offset[b];
            ts.emplace_back([&out, lo, hi, off]{
                for (std::size_t i = lo; i < hi; ++i) {
                    out[i] += off;
                }
            });
        }
        for (auto& t : ts) t.join();
    }
}


int main()
{
    constexpr std::size_t N = 50'000'000;

    std::vector<long long> in(N);
    std::mt19937 rng(7);
    std::uniform_int_distribution<int> dist(0, 99);
    for (auto& x : in) x = dist(rng);

    int n_cpus = std::max(1u, std::thread::hardware_concurrency());
    std::cout << "N = " << N
              << ", hardware_concurrency = " << n_cpus << "\n\n";

    auto bench = [&](const char* label, auto&& fn) {
        std::vector<long long> out;
        auto t0 = std::chrono::steady_clock::now();
        fn(out);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();
        // 為了驗算正確性,印出最後一個元素 (= 總和)
        std::cout << "[" << label << "] " << ms << " ms"
                  << "  out.back() = " << (out.empty() ? 0 : out.back())
                  << '\n';
        return ms;
    };

    long long t_seq = bench("std::inclusive_scan (seq) ",
        [&](auto& out){
            out.resize(N);
            // 三參數版本就是「用 + 與預設 init=0」
            std::inclusive_scan(in.begin(), in.end(), out.begin());
        });

    long long t_par_stl = bench("std::inclusive_scan (par)",
        [&](auto& out){
            out.resize(N);
            std::inclusive_scan(std::execution::par,
                                in.begin(), in.end(), out.begin());
        });

    long long t_my = bench("our parallel scan         ",
        [&](auto& out){ parallel_inclusive_scan(in, out, n_cpus); });

    std::cout << "\nspeedup vs std::inclusive_scan (seq):\n"
              << "  parallel STL : "
              << (static_cast<double>(t_seq) / t_par_stl) << "x\n"
              << "  our scan     : "
              << (static_cast<double>(t_seq) / t_my)      << "x\n";

    // ---------------------------------------------------------
    // 實戰範例: filter→pack 用 prefix sum 的 compaction 模式
    // ---------------------------------------------------------
    // 應用場景: 從 1M 筆資料中, 挑出「偶數」並 *連續* 放進新
    // 陣列 (壓緊, compaction)。如果序列做就是 push_back 迴圈;
    // 平行做必須知道「每個我的元素該放第幾個位置」── 這正是
    // prefix sum 的經典應用:
    //   step 1: 每筆元素產生 0/1 的「是否保留」mask
    //   step 2: exclusive_scan(mask) 得到每個元素的 *目的 index*
    //   step 3: 平行寫入 dest[mask_scan[i]] = src[i]  if mask[i]
    // 這個 pattern 廣泛用在 GPU stream compaction、parallel
    // partition (quicksort 內層)、radix sort 等。
    // ---------------------------------------------------------
    {
        std::cout << "\n[demo] parallel filter via prefix-sum compaction\n";
        std::vector<int> src(1'000'000);
        for (size_t i = 0; i < src.size(); ++i) src[i] = int(i);

        std::vector<int> mask(src.size());
        for (size_t i = 0; i < src.size(); ++i)
            mask[i] = (src[i] % 2 == 0) ? 1 : 0;     // 偶數 → 1

        std::vector<int> idx(src.size());
        std::exclusive_scan(std::execution::par,
                            mask.begin(), mask.end(), idx.begin(), 0);

        int kept = idx.back() + mask.back();
        std::vector<int> dest(kept);
        // 平行寫入 (沒有衝突: 每個 src 對應唯一的 dest 位置)
        std::for_each(std::execution::par,
            src.begin(), src.end(),
            [&](int& v){
                size_t i = &v - src.data();
                if (mask[i]) dest[idx[i]] = v;
            });
        std::cout << "  kept " << kept << " evens, dest[0..3] = "
                  << dest[0] << ',' << dest[1] << ','
                  << dest[2] << ',' << dest[3] << " (預期 0,2,4,6)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 prefix sum 比 reduce 還難平行化?
    //    A：reduce(in) → 一個 scalar,各塊獨立算再合一次 (logN tree merge),
    //       全程零依賴。scan(in) → 與 in 等長的陣列,每個位置 out[i] 邏輯
    //       上依賴 out[i-1] → 直覺看起來是嚴格序列。block-based 解法把它
    //       拆成「Phase 1 各塊獨立 (平行) → Phase 2 對 P 個塊和做小 scan
    //       (序列) → Phase 3 各塊套 offset (平行)」,但 Phase 2 的依賴鏈
    //       無法消除,所以 scan 的平行加速比 reduce 差。
    //
    //  Q2：什麼是 Hillis-Steele 演算法?CPU 上為什麼不流行?
    //    A：Hillis-Steele 是 GPU 經典 scan:每一輪每個位置 i 把 a[i] 加上
    //       a[i-2^k],k 從 0 跑到 log N → 共 log N 輪、總工作 O(N log N)。
    //       GPU 上每輪是免費的 SIMD wave,所以只看「步驟數 log N」最佳。
    //       CPU 上總工作多了 log N 倍 → 比序列 O(N) 還慢。block-based 保
    //       持總工作 O(N+P) 對 CPU 友善得多。Blelloch (up-sweep/down-sweep)
    //       則是 GPU 上總工作 O(N) 的進階版。
    //
    //  Q3：浮點 scan 的結果為什麼可能 run-to-run 微差?
    //    A：scan 演算法把工作切成多塊重組,需要 + 滿足結合律。整數加法、
    //       字串串接、矩陣乘 (順序固定) 嚴格結合;浮點加法 *理論上* 結合,
    //       但 IEEE 754 的 rounding 讓 (a+b)+c ≠ a+(b+c) 在 ULP 級成立。
    //       塊切法不同 → 求和順序不同 → 結果差幾個 ULP。對科學計算可能
    //       是 bug source,std::reduce 的文件特別警告這點 (par 與 seq 結
    //       果可能不同,std::accumulate 才保證左折疊順序)。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. Prefix sum 看起來「天生序列」,但 Hillis-Steele、
//    Blelloch、block-based 等多個演算法都能把它平行化。本課
//    用 block-based 是因為它最直觀、效能也夠好,實作只需要
//    幾十行。
//
// 2. 三階段的本質:
//      Phase 1: 每塊「假設前面為 0」自己算 inclusive scan。
//               完全平行,O(N/P)。
//      Phase 2: 對 P 個 block_sum 做序列 prefix sum。
//               序列,但只有 P 個元素,O(P) ≪ O(N)。
//      Phase 3: 每塊把自己那塊的 offset 加上去。
//               完全平行,O(N/P)。
//      總計:O(N/P + P)。當 P 是 16、N 是 5000 萬,P 那項
//            可忽略。
//
// 3. 為什麼不用 Hillis-Steele 「跳躍式」演算法?
//    Hillis-Steele 的並行步驟數是 O(log N),理論上最少;但是
//    它要做 O(N log N) 的總工作,反而比序列的 O(N) 多。GPU 上
//    划算 (每個 step 是免費的 SIMD),CPU 上不划算。block-based
//    保持總工作 O(N) + O(P),CPU 友善得多。
//
// 4. std::inclusive_scan 與 std::reduce 的對比:
//      reduce  -> 多執行緒分頭算 + 合併,結果是「一個值」。
//      inclusive_scan -> 每個位置都要結果,所以演算法複雜得多。
//    反映在效能上:scan 的並行加速通常比 reduce *差*,因為
//    scan 必然有 Phase 2 的序列依賴。
//
// 5. 平行 scan 的實際應用:
//      - parallel partition (quicksort、quickselect 的內層)
//      - radix sort 的 prefix-sum 階段
//      - sparse matrix CSR 格式的 row-pointer 建構
//      - parallel BFS 的 frontier compaction
//      - GPU shader 裡的 stream compaction
//
// 6. block 數的調整:不一定要等於 hardware_concurrency()。
//    如果輸入很大、L2/L3 cache 容不下整個 array,把 block
//    切得 *略大於* CPU 數有時更好 (cache friendly + work
//    stealing 自然平衡)。實務上 2P、4P 都試過,看哪個快。
// =============================================================
