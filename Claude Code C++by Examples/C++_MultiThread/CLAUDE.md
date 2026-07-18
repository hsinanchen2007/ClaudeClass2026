# CLAUDE.md — C++ MultiThread learning project

## 專案五大方針 (Project Authoring Principles)

以下五點是這個 repo 的最高優先指引,所有後續編輯與新增都必須遵守:

1. **永遠用繁體中文回答** ── 不論使用者用中文或英文發問,Claude 一律用
   繁體中文 (台灣用語慣例) 回覆。程式碼識別子、API 名稱、技術詞 (例如
   `std::mutex`, `acquire/release`, `RAII`) 維持英文以對應原始碼。

2. **每個主題對應 LeetCode 題目** ── 每個主題若有對應的 *簡單實用* 的
   LeetCode 題目 (以 *Concurrency 標籤* 9 題為主,排除為了考算法刁鑽
   不實用的題型),就在該課程式碼裡加上引用 + 完整詳細註釋,並指向
   lesson 30 看完整解。沒有直接對應的主題,就在註釋中標明「無直接對應」
   並說明它如何支撐其他課的解答。

3. **一個檔案只談一個主題** ── 每支 `.cpp` 只專注一個主題,主題介紹與
   觀念解釋要 *完整詳細*,放在程式碼上方的註釋裡。註釋遠多過程式碼也
   沒關係 ── 學習用的 repo,清楚 > 精簡。

4. **程式範例簡單直接** ── 主 demo 用最少程式碼示範主題,不寫艱澀的
   C++ template 競技、不堆無關的抽象。lesson 的份量主要在註釋,不是
   在程式碼。

5. **以 cppreference / cplusplus.com 為標準** ── 所有 C/C++ API 描述
   都對齊 https://en.cppreference.com/cpp 與 https://cplusplus.com/reference/。
   每課應在註釋中列出該課用到的關鍵 API 對應的 cppreference 連結,讓
   讀者方便深入查官方規格。

## Communication preferences

- **Always reply my questions by traditional chinese** (繁體中文 / 正體中文,
  台灣用語慣例) ── 即使提問是英文。 (與方針 #1 一致)
- Code, identifiers, console output strings, and quoted technical
  terms (`std::mutex`, `acquire/release`, `RAII`, etc.) stay in
  English so they match the actual source.

## Project purpose

A step-by-step, runnable C++ multi-threading tutorial. Each lesson is
a single self-contained `.cpp` file with heavily commented code (in
Traditional Chinese) plus a small demo / benchmark in `main()`.

每支 `.cpp` 檔頭部依序包含四層註解:
1. **檔頭簡介** ── 主題、編譯指令、執行說明
2. **課程資訊 (Class Info)** ── 主題、前置課程、觀念詞彙、新介紹 API、
   何時使用、何時不要用、常見錯誤
3. **深入解析 (Deep Dive)** ── 此主題在 OS / CPU / 編譯器底層真正的機制、
   設計動機、效能直覺、失敗模式、與其他語言/工具的對照
4. **LeetCode 對照 + cppreference** ── 對應的 LeetCode 題目 (若有)、
   解題思路指引、以及主要 API 在 cppreference 上的連結
讀者可以從上往下快速掃過 Class Info 拿主軸,需要更深的「為什麼」就讀
Deep Dive,想動手練習就照 LeetCode 對照去 lesson 30 看完整解。
全部 30 課都有這四層。

## LeetCode Concurrency 題目 → Lesson 對照表

LeetCode 的 Concurrency 標籤目前共 9 題,完整解答全部在 `30_interview_problems.cpp`。
下表列出「每個主題用到的 primitive 對應 lesson」── lesson 是基本功,
LeetCode 是練習,lesson 30 是解答。

| LC #  | 題目                                  | 主要用到的 lesson                           |
| ----- | ------------------------------------- | ------------------------------------------- |
| 1114  | Print in Order                        | lesson 12 (`binary_semaphore`)              |
| 1115  | Print FooBar Alternately              | lesson 05 (cv) / lesson 12 (semaphore)      |
| 1116  | Print Zero Even Odd                   | lesson 05 (cv) / lesson 12 (semaphore)      |
| 1117  | Building H2O                          | lesson 12 (`counting_semaphore` + `barrier`) |
| 1188  | Design Bounded Blocking Queue         | lesson 03 (mutex) + lesson 05 (cv)          |
| 1195  | Fizz Buzz Multithreaded               | lesson 05 (cv with predicate)               |
| 1226  | The Dining Philosophers               | lesson 21 (`scoped_lock`)                   |
| 1242  | Web Crawler Multithreaded             | lesson 08 (thread pool) + lesson 05 (cv)    |
| 1279  | Traffic Light Controlled Intersection | lesson 03 (`std::mutex`)                    |

## Lessons

### 第一層:基本 primitives

| #   | File                          | Topic                                                  | Build flags                                |
| --- | ----------------------------- | ------------------------------------------------------ | ------------------------------------------ |
| 01  | `01_hello_thread.cpp`         | First `std::thread`, `join()`, `sleep_for` vs `sleep_until` | `-std=c++17 -pthread`                  |
| 02  | `02_data_race.cpp`            | Data race demo (`++counter`)                           | `-std=c++17 -O0 -pthread` (race visible)   |
| 03  | `03_mutex.cpp`                | `std::mutex` + `std::lock_guard`                       | `-std=c++17 -O0 -pthread`                  |
| 04  | `04_atomic.cpp`               | `std::atomic<T>` lock-free counter                     | `-std=c++17 -O2 -pthread`                  |
| 05  | `05_condition_variable.cpp`   | Producer/consumer cv + `condition_variable_any` + `notify_all_at_thread_exit` | `-std=c++17 -O2 -pthread` |
| 06  | `06_async_future.cpp`         | `std::async` + `std::future`                           | `-std=c++17 -O2 -pthread`                  |
| 07  | `07_jthread.cpp`              | `std::jthread` + `std::stop_token` (C++20)             | `-std=c++20 -O2 -pthread`                  |
| 08  | `08_thread_pool.cpp`          | A small thread pool (capstone of 01–07)                | `-std=c++20 -O2 -pthread`                  |
| 09  | `09_parallel_stl.cpp`         | Parallel STL (`std::execution::par`)                   | `-std=c++17 -O2 -pthread -ltbb`            |
| 10  | `10_shared_mutex.cpp`         | Reader/writer lock (`std::shared_mutex`)               | `-std=c++17 -O2 -pthread`                  |
| 11  | `11_memory_model.cpp`         | `memory_order_*`, release/acquire, spinlock            | `-std=c++17 -O2 -pthread`                  |

### 第二層:C++20 新工具 + 微觀效能 + 偵錯

| #   | File                          | Topic                                                  | Build flags                                |
| --- | ----------------------------- | ------------------------------------------------------ | ------------------------------------------ |
| 12  | `12_cpp20_sync.cpp`           | `latch` / `barrier` / `counting_semaphore` / `atomic::wait` | `-std=c++20 -O2 -pthread`             |
| 13  | `13_false_sharing.cpp`        | False sharing 與 cache-line padding (`alignas(64)`)    | `-std=c++17 -O2 -pthread`                  |
| 14  | `14_thread_sanitizer.cpp`     | ThreadSanitizer 抓 race + 抓 release/acquire 配對錯誤  | `-std=c++17 -O2 -pthread` (一般) 或 TSan   |
| 15  | `15_init_once.cpp`            | `std::call_once` / Meyer's singleton / `thread_local`  | `-std=c++20 -O2 -pthread`                  |

### 第三層:高效併發資料結構

| #   | File                          | Topic                                                  | Build flags                                |
| --- | ----------------------------- | ------------------------------------------------------ | ------------------------------------------ |
| 16  | `16_spsc_ring.cpp`            | Lock-free SPSC ring buffer (release/acquire + padding) | `-std=c++17 -O2 -pthread`                  |
| 17  | `17_mpmc_sharded.cpp`         | Sharded MPMC queue (vs single-mutex queue)             | `-std=c++17 -O2 -pthread`                  |
| 18  | `18_cow_snapshot.cpp`         | Copy-on-Write 原子快照 (含 libstdc++ 函式庫稅探討)     | `-std=c++20 -O2 -pthread`                  |
| 19  | `19_hazard_pointers.cpp`      | Hazard pointers + Treiber lock-free stack              | `-std=c++17 -O2 -pthread`                  |

### 第四層:Executor / 生產實戰模式

| #   | File                          | Topic                                                  | Build flags                                |
| --- | ----------------------------- | ------------------------------------------------------ | ------------------------------------------ |
| 20  | `20_coroutines_pool.cpp`      | C++20 coroutines + thread pool executor (`schedule_on`)| `-std=c++20 -O2 -pthread`                  |
| 21  | `21_deadlock.cpp`             | Deadlock 預防:scoped_lock / `std::try_lock` 自由函式 / 全域排序 | `-std=c++17 -O2 -pthread`        |
| 22  | `22_graceful_shutdown.cpp`    | Graceful shutdown 五階段流程 (intake/drain/timeout/...) | `-std=c++20 -O2 -pthread`                  |
| 23  | `23_thread_affinity.cpp`      | `pthread_setaffinity_np` 釘執行緒到 CPU (Linux)        | `-std=c++17 -O2 -pthread`                  |

### 第五層:async logging + parallel 演算法

| #   | File                          | Topic                                                  | Build flags                                |
| --- | ----------------------------- | ------------------------------------------------------ | ------------------------------------------ |
| 24  | `24_async_logger.cpp`         | Async logger:把 I/O 從應用執行緒移到 drain 執行緒     | `-std=c++20 -O2 -pthread`                  |
| 25  | `25_parallel_mergesort.cpp`   | 手刻 parallel mergesort (fork-join + std::async)       | `-std=c++17 -O2 -pthread -ltbb`            |
| 26  | `26_parallel_scan.cpp`        | Parallel prefix sum (block-based two-phase)            | `-std=c++17 -O2 -pthread -ltbb`            |
| 27  | `27_disruptor.cpp`            | Disruptor / LMAX SPMC ring (broadcast 風格)            | `-std=c++17 -O2 -pthread`                  |

### 第六層:STL 補完 (mutex 變體 + atomic 補完)

| #   | File                          | Topic                                                  | Build flags                                |
| --- | ----------------------------- | ------------------------------------------------------ | ------------------------------------------ |
| 28  | `28_mutex_variants.cpp`       | `recursive_mutex` / `timed_mutex` / `try_lock_for/until` | `-std=c++17 -O2 -pthread`                |
| 29  | `29_atomic_flag_ref_fence.cpp`| `atomic_flag` (spinlock) + `atomic_ref` (C++20) + `atomic_thread_fence` | `-std=c++20 -O2 -pthread`     |

### 第七層:面試題集 (interview reference)

| #   | File                          | Topic                                                  | Build flags                                |
| --- | ----------------------------- | ------------------------------------------------------ | ------------------------------------------ |
| 30  | `30_interview_problems.cpp`   | LeetCode Concurrency 標籤全 9 題 (1114-1279)           | `-std=c++20 -O2 -pthread`                  |

## Notable benchmarks observed in this session (Ubuntu 24.04, WSL2, 28 vCPUs)

| Lesson | Measurement                                       | Result                                                |
| ------ | ------------------------------------------------- | ----------------------------------------------------- |
| 09     | `std::sort` 20M ints, seq vs par                  | 2376 ms → 211 ms (~11×)                               |
| 10     | 8 readers × 200k lookups, mutex vs shared         | 568 ms → 314 ms (~1.8×)                               |
| 11     | counter `seq_cst` vs `relaxed`, x86               | 0.997× (essentially identical, x86 hides the diff)    |
| 13     | False sharing, adjacent vs padded counters        | 3111 ms → 530 ms (~5.87×)                             |
| 14     | TSan 抓出 lesson 02 / 11 隱藏的 race              | 一般編譯下「答案完美」,TSan 立刻指出 line 55 / 92    |
| 16     | SPSC ring 雙執行緒 push+pop 10M ints              | 134 ms (~74 M ops/s)                                  |
| 17     | MPMC 單佇列 vs 8-shard,4P+4C × 2M items          | 575 ms → 438 ms (~1.31×)                              |
| 18     | CoW 概念正確,但 libstdc++ atomic<shared_ptr> 不是 lock-free → 純讀情況下 0.36× (反輸 shared_mutex) | 教學意義:「概念對 ≠ 實作快」 |
| 19     | HP 保護的 Treiber stack,4P+4C × 200k 各 push+pop | 800k 操作 309 ms,sum 完全正確                        |
| 20     | 6 個 coroutine 在主執行緒開始、co_await 後跳到 pool worker | 各 task tid 換到 pool 執行緒,完成 6/6           |
| 21     | 8 threads × 50k 隨機轉帳,scoped_lock / try_lock / address-ordered | 119 ms / 80 ms / 122 ms,三者皆無 deadlock |
| 22     | 30 任務 + shutdown(1500ms),晚到的 submit 被拒    | 1012 ms 排空全部 30 任務,zero loss                  |
| 23     | 28-core WSL2,4 worker × 200M 浮點累加,pin vs not | 958 ms → 955 ms (1.003×,純 CPU bound 不顯著)        |
| 24     | 8 thread × 50k log 訊息,sync vs async             | 734 ms → 343 ms (~2.14×) 應用端                      |
| 25     | 5M ints,std::sort vs par STL vs 自製 par_msort    | 601 ms / 55 ms / 144 ms (10.9× / 4.17×)              |
| 26     | 50M ints inclusive scan,seq vs par STL vs 自製   | 862 ms / 354 ms / 357 ms (~2.4× 與 par STL 並駕)     |
| 27     | Disruptor SPMC,1P+2C 廣播 5M events               | 37 ms,~135 M events/s                                |
| 28     | timed_mutex try_lock_for(500ms) 對抗 3s busy worker | 三 reader 全部 timeout 走 fallback,worker 結束後第 4 個立即拿到 |
| 29     | atomic_flag spinlock,8 thread × 100k increments   | 25 ms,counter = 800000 (zero loss)                   |
| 29     | atomic_ref 對 plain `vector<int>`,4P × 250k fetch_add | sum = 1'000'000 (預期值)                          |
| 30     | LeetCode Concurrency 9 題 (1114/1115/1116/1117/1188/1195/1226/1242/1279) 全部通過 | Q3 01020304... / Q6 1 2 fizz 4 buzz... / Q8 web crawler 5 news.com URLs (跨 host 濾掉) 61 ms / Q9 max concurrent = 1 |

## Environment notes (Ubuntu 24.04, gcc 13.3 on WSL2)

- `libtbb-dev` is required for lessons 09, 25, 26 (`-ltbb`). On this
  machine `libtbb.so` is at `/usr/lib/x86_64-linux-gnu/`. On a fresh
  box: `sudo apt-get install -y libtbb-dev`.
- Lessons 07, 08, 12, 15, 18, 20, 22, 24 use C++20 features.
  gcc 11+ supports them.
- Lesson 23 uses Linux `pthread_setaffinity_np` (also works on WSL2).
  Not portable to Windows or macOS without changes.
- **TSan on WSL2:** running a `-fsanitize=thread` binary on a recent
  WSL2 kernel may print
  `FATAL: ThreadSanitizer: unexpected memory mapping`. The kernel's
  ASLR collides with TSan's shadow layout. Run via `setarch -R ./prog`
  to disable ASLR for that process.
- **Lesson 18 caveat:** libstdc++'s `std::atomic<std::shared_ptr<T>>`
  is *not* lock-free; it uses a hidden internal lock. Naive CoW is
  therefore slower than `std::shared_mutex` on libstdc++. To get
  textbook CoW performance, use HP (lesson 19) or RCU (folly /
  liburcu). Lesson 18's source has a frank discussion of this.

## Topics deliberately skipped (and why)

| Topic                            | Why skipped                                              | Where to learn |
| -------------------------------- | -------------------------------------------------------- | -------------- |
| Boost.Asio basics                | Boost not installed; out-of-scope dependency             | Asio docs, Christopher Kohlhoff's talks |
| C++26 senders/receivers (`std::execution`) | Not in gcc 13                                  | libunifex (NVIDIA), upcoming gcc/clang |
| Linux `io_uring`                 | Needs `liburing-dev`; very Linux-specific                | liburing docs, Jens Axboe's talks |
| Multithreaded testing strategy   | More about CI methodology than runnable code             | TSan in CI matrix; concuerror / loom-style stress runners |

## Build everything at once

```bash
g++ -std=c++17 -pthread     01_hello_thread.cpp        -o 01_hello_thread
g++ -std=c++17 -O2 -pthread 02_data_race.cpp           -o 02_data_race
g++ -std=c++17 -O0 -pthread 03_mutex.cpp               -o 03_mutex
g++ -std=c++17 -O2 -pthread 04_atomic.cpp              -o 04_atomic
g++ -std=c++17 -O2 -pthread 05_condition_variable.cpp  -o 05_condition_variable
g++ -std=c++17 -O2 -pthread 06_async_future.cpp        -o 06_async_future
g++ -std=c++20 -O2 -pthread 07_jthread.cpp             -o 07_jthread
g++ -std=c++20 -O2 -pthread 08_thread_pool.cpp         -o 08_thread_pool
g++ -std=c++17 -O2 -pthread 09_parallel_stl.cpp -ltbb  -o 09_parallel_stl
g++ -std=c++17 -O2 -pthread 10_shared_mutex.cpp        -o 10_shared_mutex
g++ -std=c++17 -O2 -pthread 11_memory_model.cpp        -o 11_memory_model
g++ -std=c++20 -O2 -pthread 12_cpp20_sync.cpp          -o 12_cpp20_sync
g++ -std=c++17 -O2 -pthread 13_false_sharing.cpp       -o 13_false_sharing
g++ -std=c++17 -O2 -pthread 14_thread_sanitizer.cpp    -o 14_thread_sanitizer
g++ -std=c++17 -g -O1 -fsanitize=thread -pthread \
    14_thread_sanitizer.cpp                            -o 14_thread_sanitizer_tsan
g++ -std=c++20 -O2 -pthread 15_init_once.cpp           -o 15_init_once
g++ -std=c++17 -O2 -pthread 16_spsc_ring.cpp           -o 16_spsc_ring
g++ -std=c++17 -O2 -pthread 17_mpmc_sharded.cpp        -o 17_mpmc_sharded
g++ -std=c++20 -O2 -pthread 18_cow_snapshot.cpp        -o 18_cow_snapshot
g++ -std=c++17 -O2 -pthread 19_hazard_pointers.cpp     -o 19_hazard_pointers
g++ -std=c++20 -O2 -pthread 20_coroutines_pool.cpp     -o 20_coroutines_pool
g++ -std=c++17 -O2 -pthread 21_deadlock.cpp            -o 21_deadlock
g++ -std=c++20 -O2 -pthread 22_graceful_shutdown.cpp   -o 22_graceful_shutdown
g++ -std=c++17 -O2 -pthread 23_thread_affinity.cpp     -o 23_thread_affinity
g++ -std=c++20 -O2 -pthread 24_async_logger.cpp        -o 24_async_logger
g++ -std=c++17 -O2 -pthread 25_parallel_mergesort.cpp -ltbb -o 25_parallel_mergesort
g++ -std=c++17 -O2 -pthread 26_parallel_scan.cpp -ltbb       -o 26_parallel_scan
g++ -std=c++17 -O2 -pthread 27_disruptor.cpp           -o 27_disruptor
g++ -std=c++17 -O2 -pthread 28_mutex_variants.cpp      -o 28_mutex_variants
g++ -std=c++20 -O2 -pthread 29_atomic_flag_ref_fence.cpp -o 29_atomic_flag_ref_fence
g++ -std=c++20 -O2 -pthread 30_interview_problems.cpp  -o 30_interview_problems
```

## Topics covered (mental map)

```
                         ┌───── 14 TSan (debug tool) ────────┐
                         │                                   │
01 thread ─┬─► 02 race ─►│ 03 mutex ─┬─► 04 atomic ─► 11 memory model
           │             │           │                       │
           └─► 05 cv ────┘           │                       ▼
                                     │                  16 SPSC ring
06 async/future ─────────────────────┤                       │
07 jthread/stop_token ───────────────┤                       │
12 C++20 sync (latch/barrier/...)    │                       │
                                     ▼
                              08 thread pool        19 HP + lock-free stack
                                     │                       │
                                     ├──► 17 sharded MPMC    │
                                     ├──► 20 coroutines      │
                                     ├──► 22 graceful shutdown
                                     └──► 24 async logger    │
                                                             │
09 parallel STL ──┬──► 25 parallel mergesort                │
                  └──► 26 parallel scan                      │
                                                             │
10 shared_mutex ──► 18 CoW snapshot ────────────────────────┘

13 false sharing  (微觀效能 — 跟 16/17/19/27 的 ring/queue/stack 都相關)
15 init_once     (call_once / Meyer / thread_local — 多執行緒初始化)
21 deadlock      (scoped_lock / try_lock / 全域排序)
23 affinity      (CPU pin — 與 13 一起調 latency / jitter)
27 Disruptor     (SPMC broadcast — SPSC ring 的廣播版)
28 mutex variants (recursive / timed / try_lock_for / try_lock_until — 補完 03)
29 atomic_flag / atomic_ref / atomic_thread_fence (補完 04 + 11)
30 interview problems (LC 1114 / 1115 / 1117 / 1188 / 1226 — 把 03/05/12/21
   的 primitives 兜成題目要的同步行為)
```

## Suggested next topics (further study)

- **Boost.Asio** ── `io_context` + strand + async ops,設計成執行器
  與 lesson 20 coroutines 整合 (Asio 內建 awaitable 支援)。
- **`std::execution` (P2300)** ── senders/receivers 模型,目標 C++26。
  在標準前用 libunifex 或 NVIDIA stdexec 學習。
- **Linux io_uring** ── 真正的非同步 I/O 系統呼叫;適合 high-fanout
  伺服器、儲存系統。`liburing` 是入口。
- **Userspace RCU (liburcu)** ── 把 lesson 18 的 CoW 概念落實到 *真正
  能跑得快* 的程度。Linux 核心、DPDK 在用。
- **Memory profilers / Lock contention tools** ── perf c2c (cache-line
  bouncing)、perf lock、Intel VTune,把 lesson 13 / 17 的爭用實際量化。
- **NUMA-aware programming** ── lesson 23 的延伸,加上 `numactl`、
  per-NUMA allocator (jemalloc with arenas)。
- **Lock-free MPMC queue** ── Vyukov 的 bounded MPMC、moodycamel 的
  ConcurrentQueue;比 lesson 17 的 sharding 更上一層。
- **Reactor / Proactor patterns** ── 將事件迴圈與 lesson 08 的 pool
  整合成完整的網路服務骨架 (libevent、Asio、folly::AsyncSocket)。
- **多執行緒測試方法論** ── TSan in CI、stress runners (rr、loom-style
  的 C++ 工具如 concuerror)、屬性測試對 concurrent state machine。
