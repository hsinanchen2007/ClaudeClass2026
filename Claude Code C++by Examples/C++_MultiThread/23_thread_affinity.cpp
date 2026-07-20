// =============================================================
// 23_thread_affinity.cpp  --  Thread affinity 與 CPU 釘選 (Linux)
// =============================================================
//
// 本課目標:
//   1. 用 pthread_setaffinity_np 把執行緒釘到指定的 CPU 核心。
//   2. 用 sched_getcpu() 確認執行緒目前在哪顆核心。
//   3. 測量 affinity 帶來的效能變化:
//        - 沒釘:OS 排程器可能把執行緒在不同核心之間搬,搬一次
//          就要重新 warm cache,搬太頻繁就吃光了並行的好處。
//        - 釘住:execution flow 跟 cache 綁在一起,latency 與
//          jitter 都更穩定,適合即時、低延遲、benchmark。
//
// 注意:這支程式只跑在 Linux (含 WSL2)。Windows 用
// SetThreadAffinityMask、macOS 沒有提供等價的 API
// (只能用 thread_policy_set 給粗略提示)。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 23_thread_affinity.cpp \
//         -o 23_thread_affinity
//
// 執行方式:
//     ./23_thread_affinity
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Thread affinity ── 把執行緒釘到 CPU (Linux)
// 前置課程: lesson 01
// 觀念詞彙:
//   - affinity        ── thread 與 CPU 的綁定關係
//   - migration       ── OS 把 thread 從某 CPU 搬到另一個
//   - NUMA            ── 多 socket 機器中,每 socket 有自己的記憶體通道
//   - CPU isolation   ── 把 CPU 從 OS 排程池拿掉 (isolcpus)
// 新介紹 API:
//   #include <pthread.h> / <sched.h>
//   pthread_setaffinity_np(thr, sz, cpuset)    把 thread 釘到 cpuset
//   pthread_self()                              取得當前 thread 的 pthread_t
//   t.native_handle()                           std::thread 拿底層 pthread_t
//   sched_getcpu()                              查當下在哪顆 CPU
//   CPU_ZERO(&cs) / CPU_SET(n, &cs)             cpu_set_t 操作
// 何時使用:
//   - 即時系統 / 嚴格 P99 latency 要求
//   - NUMA 機器上想保持記憶體 locality
//   - 微基準測試,需要可重現的環境
// 何時不要用:
//   - 沒測量出 OS 排程是瓶頸前 → 預設讓 OS 自己排
//   - 熱負載動態變化的服務 → 釘住反而失去 load balancing
// 常見錯誤:
//   - 釘了之後跨 NUMA node 配置記憶體 → 跨 socket 存取仍慢
//   - WSL2 / 容器中以為釘了就獨佔 → host OS 仍可能搬整個 vCPU
//   - hyperthread sibling 也算進去 → 兩條 thread 釘在同一個 core 競爭 ALU
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── CPU 親和性的硬體與 OS 視角
// =============================================================
//
// 1. Linux 預設排程器:CFS (5.x) → EEVDF (6.6+)
//    完全公平排程器 (CFS) 把所有 thread 視為平等競爭者,把 CPU 時間切片
//    分配。若 thread 被 wakeup,scheduler 會挑「最閒、最近沒跑」的核心
//    放,*不一定* 是上次跑的那個 → cache 隨時可能變冷。
//    EEVDF (6.6 起) 改善 latency 但策略大同小異。
//    意涵:沒釘 affinity 的 thread,L1/L2 cache 命中率受 OS 心情影響。
//
// 2. Affinity 為什麼能影響效能
//    現代 x86 的 L1 cache 32 KB / core,L2 256 KB - 1 MB / core,L3 共享。
//    一條 thread 工作集若放得進 L1+L2,只要不被搬就極快。被搬到別核 →
//    cache miss 重讀 → 一兩個 µs 的 stall 累積起來不容忽視。
//    affinity 把 thread 釘住 → 同一核同一 cache → 命中率穩定。
//
// 3. 何時釘有意義
//    A. 低延遲 (< 100 µs) 的 hot path ── 高頻交易、即時音訊、CV/ML 推論。
//    B. NUMA 機器 (多 socket / Xeon、EPYC):釘到本地 NUMA node 的核,
//       搭配 numa_alloc_local 分配記憶體,避免跨 socket UPI/Infinity 經過。
//    C. CPU-bound benchmark 想要可重現結果 ── 釘住 + isolcpus + 關
//       turbo / SMT 影響。
//
// 4. 何時釘 *沒* 意義或反效果
//    A. I/O-bound 工作 ── thread 大多時間在睡覺,釘也沒省什麼。
//    B. 工作量不平均:釘了 worker N 釘到 core N,但工作分佈不均 → 某些
//       核閒著、某些爆。比讓 OS 自由排程還糟。
//    C. WSL2 / 雲端 vCPU:即使 OS 看起來釘住,hypervisor 仍能搬整個
//       vCPU。釘了也只是 *vCPU 內* 的釘,對 cache 沒幫助。
//    D. hyperthread (SMT) sibling:同 physical core 上的兩個 hardware
//       thread 共享 ALU/cache。釘兩條 CPU-bound thread 在 sibling 上
//       =每條只拿一半算力。釘要釘到 *不同 physical core*。
//
// 5. CPU 拓樸怎麼看
//      lscpu                      ── 顯示 socket / core / thread 對映
//      cat /proc/cpuinfo | head    ── physical id, core id, processor id
//      lstopo (hwloc 套件)         ── 圖形化 NUMA + cache 樹
//    範例 28 vCPU 的 WSL2:lscpu 顯示 1 socket × 14 core × 2 thread。
//    要綁 4 條 CPU-bound,挑 core 0/2/4/6 (避開 sibling)。
//
// 6. cpu_set_t / pthread_setaffinity_np
//    POSIX 沒標準化 affinity ── Linux 有 pthread_setaffinity_np / sched_
//    setaffinity,FreeBSD 有 cpuset_*,macOS 沒有 (僅有 thread_policy_set 的
//    affinity hint,通常忽略)。所以這課特別寫 #ifdef __linux__。
//    cpu_set_t 是 1024-bit (預設) 的位元集合,CPU_SET(i, &set) 把第 i 個
//    bit 設 1 表示「允許跑在 CPU i」。
//
// 7. isolcpus / cgroup cpuset 的高層工具
//    系統開機參數 isolcpus=4-7:把 CPU 4-7 從預設 scheduler 取出,只讓
//    *明確要求* 的程式跑在上面。配合應用內 setaffinity → 那幾顆核完全
//    給你獨佔。
//    cgroup v2 cpuset:per-container 的 affinity,k8s/docker 走這條。
//
// 8. NUMA 與 affinity 的搭配
//    多 socket 機器:每個 socket 有自己的記憶體 controller。跨 socket
//    存取記憶體要走 UPI/Infinity Fabric,latency 高 1.5-3×。
//    correct usage:
//      a. 釘 thread 到某 NUMA node 的核 (numactl --cpunodebind=0)
//      b. 用 numa_alloc_local() 分配本地記憶體
//    錯誤:釘了 affinity 但 malloc 拿到別 node 的記憶體 → 每次 load 都跨。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── affinity 是 Linux 平台
//     特有調校,LC 不會考。
//   → 真實系統:lesson 30 Q8 Web Crawler 若 worker 數固定 = 核數,把
//     每個 worker 釘到一個 core,可降 latency tail (不過 I/O bound 工作
//     效益不大,所以 *Web Crawler 本身* 不太需要)。affinity 真正的價值
//     是 CPU-bound 緊密迴圈 (lesson 25/26 parallel mergesort/scan)。
//
// 主要 API 對照 (cppreference + Linux man):
//   - std::thread::native_handle        https://en.cppreference.com/w/cpp/thread/thread/native_handle
//   - pthread_setaffinity_np (Linux)    https://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html
//   - sched_setaffinity (Linux)         https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html
//   - cpu_set_t (Linux)                 https://man7.org/linux/man-pages/man3/CPU_SET.3.html
//   - numactl (NUMA 工具)               https://man7.org/linux/man-pages/man8/numactl.8.html
//
// 練習建議:
//   - 把 lesson 25 parallel mergesort 的 worker 一個一個釘 core,測
//     latency 變動 (jitter) ── 釘了之後 p99 / p999 應該明顯縮小。
//   - WSL2 上 affinity 的效果比裸機弱很多,真要看效果建議在 bare metal
//     Linux 上做。
// =============================================================

#ifndef _GNU_SOURCE
#define _GNU_SOURCE          // 為了 sched_getcpu / pthread_setaffinity_np
#endif

/*
補充筆記：thread_affinity
  - thread affinity 是作業系統層級設定，用來限制某個 thread 可在哪些 CPU 上執行；它不是 C++ 標準庫功能。
  - Linux 使用 pthread_setaffinity_np、cpu_set_t、CPU_ZERO、CPU_SET 等 API，因此程式不可攜到 Windows 或 macOS。
  - affinity 主要改善 cache locality、降低 migration jitter，適合 benchmark、低延遲系統或 NUMA 感知程式。
  - 釘住 thread 不代表獨佔 CPU；同一 CPU 上仍可能有其他 thread 或 host OS 工作競爭，容器/WSL 環境尤其要小心。
  - 在 NUMA 機器上，只釘 CPU 但記憶體配置在遠端 node，仍可能因跨 socket 存取而慢；CPU 與記憶體 locality 要一起看。
  - affinity 設錯可能讓 load balancing 變差，工作負載不均時反而降低整體吞吐量。
  - sched_getcpu() 可用來觀察目前在哪顆 CPU 執行，但觀察本身只是一瞬間；沒釘住時下一刻仍可能被排程器搬走。
  - 任何 affinity 優化都應先量測 baseline，再比較 p50/p99 latency、cache miss 或 migration 次數，不能只憑直覺。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Thread affinity 與硬體並行度
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Thread affinity 是什麼？什麼時候該用？
//     答：把 thread 綁定到特定 CPU core（Linux 用 pthread_setaffinity_np /
//         sched_setaffinity；C++ 標準庫「沒有」跨平台 API）。好處是避免被排程器遷移
//         導致 cache 與 TLB 全部失效、在 NUMA 機器上確保記憶體與運算在同一 node、
//         讓延遲更可預測（降低 jitter）。代價是剝奪排程器的負載平衡能力，綁錯反而更慢。
//         實務上只有低延遲交易、即時系統、NUMA 敏感的 HPC 才手動綁定。
//     追問：為什麼一般伺服器程式不建議綁？（負載是動態的，排程器通常比人工靜態配置
//           更會分配；綁定後遇到不均衡就無法自動修正）
//
// 🔥 Q2. std::thread::hardware_concurrency() 回傳什麼？
//     答：它只是「提示值」，標準允許回傳 0（表示無法判定）。更實際的坑是在容器中：
//         它通常反映「宿主機的核心數」而非 cgroup 的 CPU 限額，於是在只分到 2 核的
//         容器裡開了 64 條 worker，造成嚴重的過度訂閱與 context switch 抖動。
//     追問：正確做法？（讀 cgroup 的 CPU quota，或由設定檔／環境變數明確指定執行緒數，
//           把 hardware_concurrency() 僅當作沒有設定時的預設值）
//
// Q3. 綁核之後為什麼延遲更穩定，但吞吐量未必更高？
//     答：綁核消除的是「遷移造成的 cache 重建」與排程不確定性，因此改善的主要是尾端
//         延遲（P99）與 jitter。吞吐量取決於整體資源利用率，綁定反而可能讓某些核閒置
//         而另一些核排隊，總吞吐下降。要不要綁，取決於你的目標是延遲還是吞吐。
//     追問：如何觀察 thread 實際跑在哪顆核？（Linux 上用 sched_getcpu()，或 perf、
//           htop 的 CPU 欄位）
// ═══════════════════════════════════════════════════════════════════════════

#include <pthread.h>
#include <sched.h>

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <unistd.h>

// -------------------------------------------------------------
// 把 *當前* 執行緒釘到 cpu。回傳是否成功。
// -------------------------------------------------------------
static bool pin_this_thread_to(int cpu)
{
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu, &cs);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
    return rc == 0;
}

// 把任意 std::thread 釘到 cpu (要 thread 已啟動)
static bool pin_thread_to(std::thread& t, int cpu)
{
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu, &cs);
    int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cs), &cs);
    return rc == 0;
}

// -------------------------------------------------------------
// CPU bound 的工作負載:做 N 次浮點累加。
// 我們在每個 worker 內把 cpu 變化記下來,觀察「OS 搬家」次數。
// -------------------------------------------------------------
struct WorkerStats {
    long long iters_done = 0;
    int       cpu_first  = -1;
    int       cpu_last   = -1;
    int       migrations = 0;     // 觀察到 sched_getcpu() 改變的次數
    long long elapsed_ms = 0;
};

static WorkerStats run_cpu_work(long long iters, int sample_every)
{
    WorkerStats s{};
    s.cpu_first = sched_getcpu();
    int last = s.cpu_first;

    auto t0 = std::chrono::steady_clock::now();

    volatile double acc = 1.0;       // volatile 避免被優化掉
    for (long long i = 0; i < iters; ++i) {
        acc = acc + 1e-9;
        if ((i % sample_every) == 0) {
            int cur = sched_getcpu();
            if (cur != last) {
                ++s.migrations;
                last = cur;
            }
        }
    }
    s.iters_done = iters;
    s.cpu_last   = last;
    s.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - t0).count();
    (void)acc;
    return s;
}


int main()
{
    int n_cpus = std::thread::hardware_concurrency();
    std::cout << "hardware_concurrency = " << n_cpus << '\n';
    std::cout << "main thread sched_getcpu() = "
              << sched_getcpu() << "\n\n";

    constexpr long long ITERS        = 200'000'000;
    constexpr int       SAMPLE_EVERY = 10'000;
    constexpr int       N_WORKERS    = 4;

    auto run_phase = [&](const char* label, bool pin)
    {
        std::vector<std::thread> ts;
        std::vector<WorkerStats> stats(N_WORKERS);
        ts.reserve(N_WORKERS);

        auto t0 = std::chrono::steady_clock::now();

        for (int i = 0; i < N_WORKERS; ++i) {
            ts.emplace_back([i, pin, &stats, n_cpus]{
                if (pin) {
                    int target = i % n_cpus;
                    pin_this_thread_to(target);
                }
                stats[i] = run_cpu_work(ITERS, SAMPLE_EVERY);
            });
        }

        for (auto& t : ts) t.join();

        auto wall = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t0).count();

        std::cout << "[" << label << "] wall = " << wall << " ms\n";
        for (int i = 0; i < N_WORKERS; ++i) {
            std::cout << "    worker " << i
                      << "  start CPU=" << stats[i].cpu_first
                      << "  end CPU="   << stats[i].cpu_last
                      << "  migrations=" << stats[i].migrations
                      << "  elapsed=" << stats[i].elapsed_ms << " ms\n";
        }
        std::cout << '\n';
        return wall;
    };

    auto t_unpinned = run_phase("UNPINNED", false);
    auto t_pinned   = run_phase("PINNED  ", true);

    std::cout << "ratio (pinned vs unpinned wall) = "
              << (static_cast<double>(t_unpinned) /
                  static_cast<double>(t_pinned))
              << "x  (>1 代表釘住有幫助)\n";

    // ---------------------------------------------------------
    // 實戰範例: dedicated I/O thread + dedicated CPU
    // ---------------------------------------------------------
    // 應用場景: 高頻交易 / 即時音訊 / DPDK packet I/O ── 有一條
    // 「絕對不能被打斷」的 thread, 必須獨佔某個 CPU core, 連 OS
    // 都不該調度別人來搶。配合 isolcpus 核心參數 + affinity 釘
    // 到該 isolated core, 可以把 jitter 拉到接近 0。
    //
    // 本課只示範如何把一條 thread 釘到 core 3 (若有 4 核以上),
    // 並驗證它確實在 core 3 跑。整套 isolcpus 設定要改 /etc/
    // default/grub 並重啟, 不在本課示範範圍。
    // ---------------------------------------------------------
    {
        std::cout << "\n[demo] 把 thread 釘到固定 core\n";
        int target_cpu = std::min(3, n_cpus - 1);
        std::thread io_thread([target_cpu]{
            pin_this_thread_to(target_cpu);
            // 跑一點工作確保 OS 真的把它搬上來
            volatile long long x = 0;
            for (int i = 0; i < 1'000'000; ++i) x += i;
            std::cout << "  pinned thread requested CPU " << target_cpu
                      << ", actually on CPU " << sched_getcpu() << '\n';
        });
        io_thread.join();
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：CPU pinning 對 cache 命中率的實際影響有多大?
    //    A：不釘時 OS 排程器會把 thread 在不同 core 之間搬,搬一次 L1/L2
    //       全部冷掉 → 重新 warm 要 1-10 µs。對工作集放得進 L1 (32 KB) /
    //       L2 (256 KB-1 MB) 的緊密迴圈,命中率從 ~98% 掉到 ~70% 不奇怪。
    //       但純 CPU-bound 大陣列計算 (lesson 23 demo) 工作集已超出 cache,
    //       釘不釘差不多 → 本課 ratio 才會接近 1.0。差距在「短工作集 +
    //       高頻 wakeup」場景才放大 (HFT、即時音訊)。
    //
    //  Q2：NUMA 機器上釘 thread 要注意什麼?
    //    A：兩件事必須一起做:(a) thread 釘到某 NUMA node 的 core
    //       (numactl --cpunodebind=0 或 sched_setaffinity);(b) 記憶體也
    //       要從同 node 配置 (numa_alloc_local 或 numactl --membind=0)。
    //       只釘 CPU 但 malloc 拿到別 node 的記憶體 → 每筆 load 都跨
    //       UPI/Infinity Fabric,latency 1.5-3×、頻寬打折。「first-touch」
    //       政策:第一次寫該頁的 thread 決定 page 落在哪個 node,所以
    //       初始化也要跑在對的 thread 上。
    //
    //  Q3：什麼時候釘 hyperthread sibling 反而更慢?
    //    A：x86 SMT 兩個 hardware thread 共享 L1/L2 cache、ALU、port。釘
    //       兩條 CPU-bound thread 到同一個 physical core 的 sibling →
    //       兩條只各拿到 ~50-60% 算力。要釘到 *不同 physical core*。lscpu
    //       看 thread/core 對映,Linux 上 /sys/devices/system/cpu/cpuN/topology
    //       的 thread_siblings_list 也可查。HFT 與 DPDK 場景常 isolcpus
    //       + 關 SMT 拿乾淨 core。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. Affinity = 把執行緒「綁定」到一個或一組 CPU 核心,告訴
//    排程器:「這條執行緒就應該在這裡跑」。OS 仍然可能在
//    那組核心之間搬,但不會搬出去。
//
// 2. 為何這對效能有幫助:
//      (a) L1/L2 cache 是每個核心私有的;搬一次就要重新 warm。
//      (b) NUMA 機器 (多 socket) 上,跨 socket 搬還會跨記憶體
//          通道,latency 與 bandwidth 都打折。
//      (c) 即時系統需要可預測的 latency;OS 排程器的 load
//          balancing 會引入不可預測的搬動。
//
// 3. *不一定* 會看到大幅加速。在純 CPU bound、無 I/O 的工作
//    負載下,本課的 ratio 通常落在 1.0–1.2x 之間,有時甚至是
//    pinned 略慢 (因為釘死了所以 OS 沒辦法 load balance)。
//    真正的差距會在這些情境下放大:
//      - 高 cache miss 工作 (掃大型陣列)
//      - 多 NUMA node、跨 socket
//      - 嚴格延遲要求 (P99)
//      - 與其他 process 競爭 CPU
//
// 4. 釘住的策略:
//      - 一個執行緒釘一個 CPU (本課做的)。最強保證,但失去
//        load balance。
//      - 一個執行緒 *指定多顆 CPU* (CPU_SET 多個位元)。OS 可
//        在裡面搬,通常給「同一個 NUMA node 的所有 CPU」。
//        這是大型服務最常用的折衷。
//      - taskset / numactl 指令:外部從 shell 把整個 process
//        釘住,不必改程式。適合臨時 benchmark。
//
// 5. 想看 NUMA 拓樸:
//      lscpu               — 看 CPU 架構與 NUMA node
//      numactl --hardware  — 看每個 NUMA node 的記憶體與 CPU
//      lstopo              — 圖形化的拓樸 (hwloc 套件)
//
// 6. WSL2 注意事項:WSL2 跑在 Hyper-V 之上,看到的 cpu 編號
//    是虛擬機分配的;sched_getcpu() 仍然會回應正確的 vcpu
//    id,但「同一個 NUMA node」的概念在 WSL2 內可能不像
//    bare metal 那麼有意義。
//
// 7. 進階:CPU isolation (isolcpus 核心參數) 把幾顆 CPU 從
//    OS 排程池中剔除,留給你 *獨佔* 用。配合 affinity 後,
//    那些 CPU 上 *沒有任何其他工作*,jitter 接近 0。HFT 與
//    DPDK 等領域必備。
//
// 8. 不要過度釘住。多執行緒程式的彈性正是「OS 能根據負載自動
//    重排」。除非你已經量測出 OS 的搬動帶來的 jitter 是個
//    真實的 latency 問題,否則先別釘 ── 釘錯了反而更慢。
// =============================================================
