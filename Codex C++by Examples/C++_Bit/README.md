# Bit：用 unsigned 型別處理位元資料

位元操作常見於權限旗標、協定欄位、硬體 register、壓縮與演算法。核心原則是使用 unsigned 固定寬度型別，避免 signed shift 與 overflow 的語意陷阱。

## 基本運算

- AND：測試或保留 bits。
- OR：設定 bits。
- XOR：切換 bits。
- NOT：反轉 bits，記得結果寬度由型別決定。
- shift：乘除 2 的冪只是其中一種用途；shift count 不可大於等於型別寬度。

對 signed 負值右移是 implementation-defined（較新標準語意有所收斂但仍不適合協定）；signed 左移還可能 UB。位元資料應轉為 unsigned。

## Enum flags

<code>enum class</code> 避免名字污染與意外整數轉換，但不自帶 bit operators。只為明確設計成 flags 的 enum 定義 OR/AND helper。

## bitset 與 C++20 bit utilities

bitset 適合 compile-time 固定 bit count，提供 test/count/set。<code>&lt;bit&gt;</code> 提供 popcount、rotl、countl_zero、bit_width、has_single_bit 等，不必手刻易錯版本。

## Endianness

<code>std::endian</code> 描述 native byte order，但不會自動轉網路序。檔案/協定必須指定 byte order，逐 byte encode/decode 或用明確 conversion。

## 範例索引

本章共 13 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_basic_operators.cpp`](01_basic_operators.cpp)
- [`02_common_tricks.cpp`](02_common_tricks.cpp)
- [`03_bit_header_cpp20.cpp`](03_bit_header_cpp20.cpp)
- [`04_bit_cast.cpp`](04_bit_cast.cpp)
- [`05_lc_191_hamming_weight.cpp`](05_lc_191_hamming_weight.cpp)
- [`06_lc_136_single_number.cpp`](06_lc_136_single_number.cpp)
- [`07_lc_268_missing_number.cpp`](07_lc_268_missing_number.cpp)
- [`08_lc_338_counting_bits.cpp`](08_lc_338_counting_bits.cpp)
- [`09_lc_231_power_of_two.cpp`](09_lc_231_power_of_two.cpp)
- [`10_lc_201_range_bitwise_and.cpp`](10_lc_201_range_bitwise_and.cpp)
- [`11_lc_78_subsets.cpp`](11_lc_78_subsets.cpp)
- [`12_practical_flags.cpp`](12_practical_flags.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 從 32-bit 狀態字取出 bits 8 到 15。
2. 為 Permission 實作清除旗標。
3. 將 uint32_t 以 big-endian 四 bytes 編碼再解碼。
