# Chrono：把單位放進型別

<code>&lt;chrono&gt;</code> 的核心是三個概念：clock 產生 time_point，兩個 time_point 相減得到 duration，duration 由 representation 與 period 決定。這讓秒、毫秒與奈秒不能在不明確的情況下混用。

## Clock 選擇

- <code>steady_clock</code>：單調不倒退，適合量耗時與 deadline。
- <code>system_clock</code>：可轉日曆/Unix time，可能因校時而跳動。
- <code>high_resolution_clock</code>：可能只是前兩者 alias，不提供單調保證，不能因名字好聽就用於 benchmark。

## Duration conversion

從細單位轉粗單位可能截斷，需要 <code>duration_cast</code>；從粗轉細通常可隱式進行。若要小數秒，用 floating representation，例如 <code>duration&lt;double&gt;</code>。

## Benchmark 邊界

簡單 chrono 計時只能做初步比較。要避免：

- 被測程式被 optimizer 消除。
- 把資料準備、配置或 I/O 混入測量。
- 只跑一次。
- 不同 CPU frequency/cache 狀態混在一起。
- 用結果差異推論因果。

正式 microbenchmark 應用專門框架，並報告分布、環境與 build flags。

## Deadline

相對等待與 timeout 應由 steady_clock 計算 absolute deadline，並在迴圈重用同一 deadline，避免 spurious wakeup 讓每次相對 timeout 重新起算。

## 範例索引

本章共 10 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_overview.cpp`](01_overview.cpp)
- [`02_clocks.cpp`](02_clocks.cpp)
- [`03_duration.cpp`](03_duration.cpp)
- [`04_time_point.cpp`](04_time_point.cpp)
- [`05_benchmark.cpp`](05_benchmark.cpp)
- [`06_sleep.cpp`](06_sleep.cpp)
- [`07_calendar_cpp20.cpp`](07_calendar_cpp20.cpp)
- [`08_practical_timer.cpp`](08_practical_timer.cpp)
- [`09_unix_timestamp.cpp`](09_unix_timestamp.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 將 1500ms 轉成整數秒與浮點秒，比較結果。
2. 對同一演算法跑 21 次並取中位數。
3. 寫一個總期限 100ms、可多次重試但不重置期限的函式。
