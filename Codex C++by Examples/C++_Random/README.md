# Random：引擎產生 bits，分布產生模型

<code>&lt;random&gt;</code> 把兩件事分開：

1. Engine 產生可重現的 pseudo-random bit sequence。
2. Distribution 把 bits 映射成 uniform integer、normal、Bernoulli 等統計分布。

不要用 engine output 對 N 取餘數取代 uniform_int_distribution；若 engine range 不能整除 N，會產生 modulo bias。

## Seed 與重現

固定 seed 適合測試、benchmark 與 bug reproduction。非固定 seed 可由 random_device 或 seed_seq 建立，但 <code>random_device</code> 是否真硬體 entropy 由 implementation 決定。

可重現性還有層次：標準 engine 的序列有規格，但 distribution 如何消耗 engine、浮點轉換結果不一定跨所有標準庫完全一致。若要跨平台 bit-exact，需鎖定自己的 sampling algorithm。

## 安全性

mt19937 不是密碼學安全 PRNG。Token、密碼、session ID 應使用 OS CSPRNG 或成熟 crypto library。

## Distribution 是有狀態物件

有些 distribution 可能 cache 值；重新 seed engine 不一定等同重建完整 generator 狀態。需要完全重現時，保存/重建 engine 與 distribution，或呼叫 reset。

## 範例索引

本章共 10 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_why_not_rand.cpp`](01_why_not_rand.cpp)
- [`02_engines.cpp`](02_engines.cpp)
- [`03_uniform_int.cpp`](03_uniform_int.cpp)
- [`04_uniform_real.cpp`](04_uniform_real.cpp)
- [`05_other_distributions.cpp`](05_other_distributions.cpp)
- [`06_shuffle.cpp`](06_shuffle.cpp)
- [`07_seeding.cpp`](07_seeding.cpp)
- [`08_lc_384_shuffle_array.cpp`](08_lc_384_shuffle_array.cpp)
- [`09_lc_528_random_pick_with_weight.cpp`](09_lc_528_random_pick_with_weight.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 擲骰 60,000 次，檢查各面頻率但不要把統計波動當錯誤。
2. 把 failing randomized test 的 seed 印出並重播。
3. 說明為何抽樣不放回要用 sample，而不是重複 uniform index。
