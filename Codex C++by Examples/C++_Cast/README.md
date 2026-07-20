# Cast：轉型是在宣告風險

C++ 的 named casts 讓 reviewer 看出意圖。轉型不是「讓 compiler 安靜」；它應該表示你已驗證某個前置條件。

## static_cast

用於明確 numeric conversion、enum conversion、已知安全的 class hierarchy 向上轉型，以及少數明確的向下轉型。向下轉型若 runtime object 不是目標 derived type，後續使用會出錯；有多型檢查需求應用 dynamic_cast。

## dynamic_cast

只適用 polymorphic hierarchy（base 至少有一個 virtual member）。Pointer 失敗回 nullptr，reference 失敗丟 <code>bad_cast</code>。若頻繁 dynamic_cast 決定行為，可能代表介面或 variant 更適合。

## const_cast

只改 cv qualification。若原物件本來就是 const，移除 const 後寫入是 undefined behavior。它主要用於銜接錯誤或舊 C API，不是一般設計工具。

## reinterpret_cast

表示低階 representation/ABI 操作，風險包含 alignment、strict aliasing、object lifetime 與 portability。讀取 object bits 優先用 C++20 <code>bit_cast</code>；搬 bytes 用 memcpy。

## Numeric narrowing

static_cast 不檢查 overflow/truncation。由大範圍轉小範圍前要比對 numeric_limits，尤其 signed/unsigned 混合。

## 範例索引

本章共 9 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_overview.cpp`](01_overview.cpp)
- [`02_static_cast.cpp`](02_static_cast.cpp)
- [`03_dynamic_cast.cpp`](03_dynamic_cast.cpp)
- [`04_const_cast.cpp`](04_const_cast.cpp)
- [`05_reinterpret_cast.cpp`](05_reinterpret_cast.cpp)
- [`06_bit_cast_cpp20.cpp`](06_bit_cast_cpp20.cpp)
- [`07_implicit_explicit.cpp`](07_implicit_explicit.cpp)
- [`08_pitfalls.cpp`](08_pitfalls.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 讓 checked cast 支援 signed/unsigned 的所有組合。
2. 以 virtual function 取代一段 dynamic_cast 分支。
3. 比較 bit_cast 與錯誤的 pointer reinterpret/dereference。
