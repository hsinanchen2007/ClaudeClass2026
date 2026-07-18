# C++ Chrono 學習專案

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答** — 不管使用者用中文或英文發問，回覆一律繁體中文。
2. **每個主題加入「簡單實用」的範例** — 不挑刁鑽演算法題；要選日常 / 面試 warm-up 等級。
3. **每個程式只談一個主題** — 一個 `.cpp` 聚焦一個 chrono 子主題，並附詳細註釋。
4. **範例要簡單直接** — 不寫艱澀冗長的 C++。
5. **參考來源** —
   - https://en.cppreference.com/cpp/chrono
   - https://cplusplus.com/reference/chrono/

## 專案結構

```
C++_Chrono/
├── CLAUDE.md
├── Makefile
├── 01_overview.cpp           # clock / duration / time_point 三概念總覽
├── 02_clocks.cpp             # system_clock / steady_clock / high_resolution_clock
├── 03_duration.cpp           # duration、duration_cast、字面量 (ms/s/min)
├── 04_time_point.cpp         # time_point 運算、time_since_epoch
├── 05_benchmark.cpp          # 用 steady_clock 量測函式時間
├── 06_sleep.cpp              # this_thread::sleep_for / sleep_until
├── 07_calendar_cpp20.cpp     # C++20 year_month_day / 日期運算
├── 08_practical_timer.cpp    # Stopwatch / RateLimiter 工具類別
└── 09_unix_timestamp.cpp     # 與 Unix epoch 的轉換、time_t / strftime
```

## 知識速查

| 場景 | 推薦 clock | 理由 |
|---|---|---|
| 量測一段程式跑多久 | `steady_clock` | 單調遞增，不會被 NTP 校正 |
| 紀錄真實時間（log、TTL）| `system_clock` | 對應牆上時間、可轉 time_t |
| 高精度量測（基本上等同 steady_clock）| `high_resolution_clock` | 多數實作就是 steady_clock 別名 |

## 編譯方式

```bash
make
make run
make clean
```
