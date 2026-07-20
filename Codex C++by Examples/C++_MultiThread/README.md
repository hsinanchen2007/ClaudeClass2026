# MultiThread：先建立 happens-before，再談平行

多執行緒正確性的核心不是「看起來同時跑完」，而是 shared state 的每次衝突存取都有同步關係。Data race 是 undefined behavior，不能靠測試一百次沒壞證明安全。

## Thread lifetime

<code>std::thread</code> 在 destructor 時若仍 joinable 會 terminate。C++20 <code>std::jthread</code> 會在析構時 request stop 並 join，適合 scope-bound worker；但 cooperative cancellation 仍需要工作函式主動檢查 stop token。

## Mutex

Mutex 建立 unlock 到後續 lock 的同步關係。使用 lock_guard/unique_lock 以 RAII 解鎖。鎖保護的是不變式，不只是某個變數；所有讀寫同一狀態的路徑必須遵守同一規則。

## Condition variable

永遠搭配 predicate：

    condition.wait(lock, predicate);

原因是 spurious wakeup，且通知可能早於 waiter 進入等待。Predicate 必須在同一 mutex 保護下讀取。

## Task-based concurrency

若只需要「執行工作並取得結果/例外」，future/async 比裸 thread 更高階。<code>std::async</code> 不指定 launch policy 時可能 deferred；需要並行時明確指定 <code>std::launch::async</code>。

## Atomic

Atomic 適合單一變數的 lock-free/low-level 同步，不會自動保護跨多欄位不變式。Memory order 是進階 contract；預設 sequential consistency 通常是正確起點。

## 範例索引

本章共 31 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_hello_thread.cpp`](01_hello_thread.cpp)
- [`02_data_race.cpp`](02_data_race.cpp)
- [`03_mutex.cpp`](03_mutex.cpp)
- [`04_atomic.cpp`](04_atomic.cpp)
- [`05_condition_variable.cpp`](05_condition_variable.cpp)
- [`06_async_future.cpp`](06_async_future.cpp)
- [`07_jthread.cpp`](07_jthread.cpp)
- [`08_thread_pool.cpp`](08_thread_pool.cpp)
- [`09_parallel_stl.cpp`](09_parallel_stl.cpp)
- [`10_shared_mutex.cpp`](10_shared_mutex.cpp)
- [`11_memory_model.cpp`](11_memory_model.cpp)
- [`12_cpp20_sync.cpp`](12_cpp20_sync.cpp)
- [`13_false_sharing.cpp`](13_false_sharing.cpp)
- [`14_thread_sanitizer.cpp`](14_thread_sanitizer.cpp)
- [`15_init_once.cpp`](15_init_once.cpp)
- [`16_spsc_ring.cpp`](16_spsc_ring.cpp)
- [`17_mpmc_sharded.cpp`](17_mpmc_sharded.cpp)
- [`18_cow_snapshot.cpp`](18_cow_snapshot.cpp)
- [`19_hazard_pointers.cpp`](19_hazard_pointers.cpp)
- [`20_coroutines_pool.cpp`](20_coroutines_pool.cpp)
- [`21_deadlock.cpp`](21_deadlock.cpp)
- [`22_graceful_shutdown.cpp`](22_graceful_shutdown.cpp)
- [`23_thread_affinity.cpp`](23_thread_affinity.cpp)
- [`24_async_logger.cpp`](24_async_logger.cpp)
- [`25_parallel_mergesort.cpp`](25_parallel_mergesort.cpp)
- [`26_parallel_scan.cpp`](26_parallel_scan.cpp)
- [`27_disruptor.cpp`](27_disruptor.cpp)
- [`28_mutex_variants.cpp`](28_mutex_variants.cpp)
- [`29_atomic_flag_ref_fence.cpp`](29_atomic_flag_ref_fence.cpp)
- [`30_interview_problems.cpp`](30_interview_problems.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 寫一個 jthread worker，每處理一批工作後檢查 stop。
2. 將 queue 擴成 bounded queue，加入 not-full condition。
3. 讓 async task 拋例外，確認在 future.get() 重新拋出。
