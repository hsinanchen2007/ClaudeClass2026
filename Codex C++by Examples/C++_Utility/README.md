# Utility：小型型別組合出清楚介面

<code>&lt;utility&gt;</code> 與相關 vocabulary types 提供許多小工具。重點是使用它們表達 contract，而不是把所有資料塞進匿名 pair/tuple。

## optional、variant、any

- optional：零或一個 T。
- variant：編譯期已知的有限型別集合，visit 可完整處理。
- any：runtime 可裝任何 copyable 型別，取錯型別會丟 bad_any_cast。它最彈性，也最少靜態資訊，應限於 plugin/config 等真正開放集合。

## pair 與 tuple

適合局部回傳多個值、zip-like 操作與 generic glue。若欄位具有領域意義，具名 struct 通常更可讀，也能維持不變式。

## move、forward、exchange

- move：把 expression cast 成可移動類別。
- forward：在 forwarding reference 中保留 caller 的 value category。
- exchange：取出舊值並放入新值，適合 move implementation 與狀態轉移。

## reference_wrapper

容器不能直接存 reference；<code>reference_wrapper</code> 提供可複製的 reference-like wrapper。來源仍必須活得夠久，它不是 ownership。

## 範例索引

本章共 14 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_pair.cpp`](01_pair.cpp)
- [`02_tuple.cpp`](02_tuple.cpp)
- [`03_optional.cpp`](03_optional.cpp)
- [`04_variant.cpp`](04_variant.cpp)
- [`05_any.cpp`](05_any.cpp)
- [`06_swap.cpp`](06_swap.cpp)
- [`07_move.cpp`](07_move.cpp)
- [`08_forward.cpp`](08_forward.cpp)
- [`09_exchange.cpp`](09_exchange.cpp)
- [`10_as_const.cpp`](10_as_const.cpp)
- [`11_bitset.cpp`](11_bitset.cpp)
- [`12_hash.cpp`](12_hash.cpp)
- [`13_initializer_list.cpp`](13_initializer_list.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 把 any 設定表改成 variant，觀察 compile-time 保護。
2. 將三欄 tuple 改為具名 struct，比較 call site。
3. 用 reference_wrapper 建立不擁有物件的工作清單。
