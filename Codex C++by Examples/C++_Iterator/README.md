# Iterator：容器與演算法的共同語言

Iterator 把「如何走訪資料」和「要對資料做什麼」分離。演算法不必知道資料來自 vector、list、array 或 stream，只需要特定能力的 iterator。

## 能力階層

- input：單向讀取，可能只能走一次。
- output：單向寫入。
- forward：可多次走訪。
- bidirectional：可前進與後退。
- random access：可常數時間跳距離與比較位置。
- contiguous：C++20 起表示元素實際連續，可取得相鄰地址。

演算法只應要求最低必要能力。例如 sort 需要 random access，因此 list 要用自己的 member <code>sort()</code>。

## Range 是半開區間

標準慣例是 <code>[first, last)</code>：first 指向第一個元素，last 指向最後元素之後。空 range 由 <code>first == last</code> 自然表示，也讓相鄰 range 可直接拼接。

## Adapter

<code>back_inserter</code> 把 assignment 轉成 push_back；reverse_iterator 反向解讀移動方向；istream_iterator 把輸入流包成 input iterator。Adapter 讓既有演算法接上不同資料來源與目的地。

## 失效

Iterator 是否有效由原容器操作規則決定。失效後即使地址數值看起來相同，也不能解參考、比較或做差。正確模式是使用修改操作回傳的新 iterator，或在修改後重新取得。

## 範例索引

本章共 15 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`00_overview.cpp`](00_overview.cpp)
- [`01_input_iterator.cpp`](01_input_iterator.cpp)
- [`02_output_iterator.cpp`](02_output_iterator.cpp)
- [`03_forward_iterator.cpp`](03_forward_iterator.cpp)
- [`04_bidirectional_iterator.cpp`](04_bidirectional_iterator.cpp)
- [`05_random_access_iterator.cpp`](05_random_access_iterator.cpp)
- [`06_reverse_iterator.cpp`](06_reverse_iterator.cpp)
- [`07_insert_iterators.cpp`](07_insert_iterators.cpp)
- [`08_stream_iterators.cpp`](08_stream_iterators.cpp)
- [`08b_streambuf_iterators.cpp`](08b_streambuf_iterators.cpp)
- [`09_move_iterator.cpp`](09_move_iterator.cpp)
- [`10_iterator_operations.cpp`](10_iterator_operations.cpp)
- [`11_iterator_invalidation.cpp`](11_iterator_invalidation.cpp)
- [`12_custom_iterator.cpp`](12_custom_iterator.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 讓 sum_range 同時接受 vector 與 list。
2. 用 ostream_iterator 輸出以逗號分隔的資料，處理最後一個逗號問題。
3. 寫出 vector erase 所有負數的安全迴圈。
