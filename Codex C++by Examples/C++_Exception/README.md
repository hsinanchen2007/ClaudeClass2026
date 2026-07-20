# Exception：先定義失敗後的物件狀態

例外處理的核心不是 catch 語法，而是 exception safety：操作失敗時，資源是否釋放、物件是否仍符合不變式、caller 能否判斷下一步。

## 三種常見保證

- no-throw：承諾不拋出；destructor、swap、move 常需要這項。
- strong guarantee：失敗時沒有可觀察變更，像 transaction。
- basic guarantee：沒有洩漏且物件仍有效，但值可能已改變。

不是所有操作都值得 strong guarantee；應依成本與需求說明 contract。

## RAII 是例外安全的地基

離開 scope 時 local objects 依反序析構，即使由 exception 離開亦然。資源應由 vector、string、smart pointer、fstream 或專用 handle class 擁有，避免在每個 catch 手動清理。

## 何時不用 exception

- 沒找到資料等常態分支：optional。
- 需要高頻、可預期的錯誤值：error code/expected。
- destructor 與 noexcept 邊界：記錄錯誤或採其他策略，不能讓例外逃出。

## Catch 原則

通常以 <code>const std::exception&amp;</code> 捕捉，避免 slicing。只 catch 能實際處理的錯誤；否則讓它往上傳播。<code>catch (...)</code> 適合邊界記錄與清理後重新拋出，不應吞掉未知錯誤。

## 範例索引

本章共 10 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_basics.cpp`](01_basics.cpp)
- [`02_standard_exceptions.cpp`](02_standard_exceptions.cpp)
- [`03_what_to_throw.cpp`](03_what_to_throw.cpp)
- [`04_catch_by_ref.cpp`](04_catch_by_ref.cpp)
- [`05_noexcept.cpp`](05_noexcept.cpp)
- [`06_raii_safety.cpp`](06_raii_safety.cpp)
- [`07_function_try_block.cpp`](07_function_try_block.cpp)
- [`08_nested_exception.cpp`](08_nested_exception.cpp)
- [`09_pitfalls.cpp`](09_pitfalls.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 讓批次加入在任何輸入不合法時完全不改原資料。
2. 在 custom error 中加入 error code，而不是解析 <code>what()</code>。
3. 寫一個 scope-bound mutex lock，確認拋例外後仍解鎖。
