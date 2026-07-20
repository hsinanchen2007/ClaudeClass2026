# Smart Pointers：用型別表達所有權

Smart pointer 不是「比較安全的 pointer」這麼簡單；它把誰負責釋放資源寫進型別。先決定 ownership，再選 pointer。

## unique_ptr

唯一擁有者的預設。可移動、不可複製，大小通常等於 raw pointer（stateless deleter 時）。Factory 回傳 unique_ptr 能明確移交所有權，也讓失敗路徑自動清理。

## shared_ptr

多個 owner 共同控制物件生命週期。它有 reference-count control block，複製涉及計數操作；這不是普通 raw pointer 的免費替代品。優先用 <code>make_shared</code>，除非需要特殊 deleter、獨立配置或特殊生命週期。

## weak_ptr

觀察 shared object 而不增加 strong count。它用來打破 cycle，或表示 cache/observer。每次使用前透過 <code>lock()</code> 取得暫時 shared owner；先 <code>expired()</code> 再操作有 TOCTOU，不能代替 lock。

## Non-owning pointer/reference

若函式不接管 ownership，可用 reference（不可空）或 raw pointer（可空）。不要為了「看起來現代」把所有借用都改成 shared_ptr，否則 ownership 反而不清楚。

## Custom deleter

unique_ptr 可管理 FILE、socket、GPU handle 等非 new 資源。deleter 型別是 unique_ptr 型別的一部分；shared_ptr 的 deleter 則 type-erased 在 control block。

## 範例索引

本章共 6 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_unique_ptr.cpp`](01_unique_ptr.cpp)
- [`02_shared_ptr.cpp`](02_shared_ptr.cpp)
- [`03_weak_ptr.cpp`](03_weak_ptr.cpp)
- [`04_enable_shared_from_this.cpp`](04_enable_shared_from_this.cpp)
- [`05_custom_deleter.cpp`](05_custom_deleter.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 將 unique_ptr 放入 vector，再移動整個 vector。
2. 建立 parent/child 關係，讓 child 以 weak_ptr 指回 parent。
3. 為一個假的 C handle API 寫 custom deleter。
