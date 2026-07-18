# C++ Bit Manipulation 學習專案

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答**。
2. **每個主題加入「簡單實用」的 LeetCode 範例**。位元題目本身就是 LC 大家族，本專案 1/3 檔案是 LC 題目對應實作。
3. **每個程式只談一個主題**。一個 `.cpp` 聚焦一個位元小主題或一道 LC 題。
4. **範例要簡單直接**。
5. **參考來源** —
   - https://en.cppreference.com/w/cpp/header/bit
   - https://cplusplus.com/reference/

## 專案結構

```
C++_Bit/
├── CLAUDE.md
├── Makefile
├── 01_basic_operators.cpp        # & | ^ ~ << >> 與優先級陷阱
├── 02_common_tricks.cpp          # n&(n-1)、n&-n、x^x=0 等經典技巧
├── 03_bit_header_cpp20.cpp       # <bit>: popcount / countl_zero / bit_width ...
├── 04_bit_cast.cpp               # std::bit_cast 安全做 bit-pattern 轉換
├── 05_lc_191_hamming_weight.cpp  # LC191 統計位元 1 的個數
├── 06_lc_136_single_number.cpp   # LC136 用 XOR 找唯一值
├── 07_lc_268_missing_number.cpp  # LC268 用 XOR 找缺失值
├── 08_lc_338_counting_bits.cpp   # LC338 0..n 各數的 popcount，DP+位元
├── 09_lc_231_power_of_two.cpp    # LC231 n & (n-1) == 0
├── 10_lc_201_range_bitwise_and.cpp # LC201 範圍 AND（找公共前綴）
├── 11_lc_78_subsets.cpp          # LC78 用 bitmask 枚舉子集
└── 12_practical_flags.cpp        # 工作上的「flag enum」常見用法
```

## 編譯

```bash
make
make run
make clean
```

## 知識速查

| 表達式 | 用途 |
|---|---|
| `n & (n-1)` | 把最低位的 1 清掉 |
| `n & -n`    | 取出最低位的 1 |
| `n ^ n = 0` | 同值 XOR 為 0 |
| `n ^ 0 = n` | 與 0 XOR 不變 |
| `n & 1`     | 最低位是 1（奇數） |
| `n >> 1`    | 除以 2（無符號） |
| `n << k`    | 乘以 2^k |
| `~n + 1`    | 取補數 (= -n) |
| `n & (1<<i)`| 第 i 位是 1？ |
| `n \| (1<<i)` | 設第 i 位為 1 |
| `n & ~(1<<i)` | 清第 i 位 |
| `n ^ (1<<i)`  | 翻轉第 i 位 |
