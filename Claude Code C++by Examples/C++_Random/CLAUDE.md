# C++ Random 學習專案

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答**。
2. **每個主題加入「簡單實用」的 LeetCode 範例**。
3. **每個程式只談一個主題**。
4. **範例要簡單直接**。
5. **參考來源** —
   - https://en.cppreference.com/w/cpp/numeric/random
   - https://cplusplus.com/reference/random/

## 專案結構

```
C++_Random/
├── CLAUDE.md
├── Makefile
├── 01_why_not_rand.cpp                # 為什麼不要用 C 的 rand()
├── 02_engines.cpp                     # mt19937 / mt19937_64 / random_device
├── 03_uniform_int.cpp                 # uniform_int_distribution
├── 04_uniform_real.cpp                # uniform_real_distribution
├── 05_other_distributions.cpp         # bernoulli / normal / exponential / discrete
├── 06_shuffle.cpp                     # std::shuffle
├── 07_seeding.cpp                     # 用 random_device + seed_seq 種子
├── 08_lc_384_shuffle_array.cpp        # LeetCode 384
└── 09_lc_528_random_pick_with_weight.cpp  # LeetCode 528
```

## 速查表（現代 C++ 隨機數標準姿勢）

```cpp
#include <random>

std::mt19937 rng{std::random_device{}()};      // 引擎 + 種子
std::uniform_int_distribution<int> dist{1, 6}; // 1..6 骰子
int v = dist(rng);
```

| 想做 | 用什麼 |
|---|---|
| 整數 [a, b] | `uniform_int_distribution<int>{a, b}` |
| 實數 [a, b) | `uniform_real_distribution<double>{a, b}` |
| 機率 p 的 0/1 | `bernoulli_distribution{p}` |
| 常態分佈 | `normal_distribution<double>{mean, sd}` |
| 帶權離散選 | `discrete_distribution{weights}` |
| 洗牌 | `std::shuffle(first, last, rng)` |

## 編譯

```bash
make
make run
make clean
```
