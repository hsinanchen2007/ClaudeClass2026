# Filesystem：路徑是結構，不是字串

<code>std::filesystem::path</code> 會依平台表示 native path syntax，並提供 parent、filename、extension 與組合操作。用 <code>/</code> operator 組合 path，避免手工串接 separator。

## 查詢與競態

先 <code>exists()</code> 再開檔存在 TOCTOU：兩次呼叫間 filesystem 可改變。安全性或正確性依賴時，直接執行目標操作並處理結果。<code>error_code</code> overload 適合預期失敗；throwing overload 適合把 I/O failure 交給上層。

## Directory traversal

巡覽順序未由標準保證。需要穩定輸出或測試時先收集再排序。Symbolic link、permission denied、recursive cycle 與跨 filesystem 邊界都要先定 policy。

## 寫檔與「原子」

常見流程是在同一目錄寫 temporary、flush/close，再 rename 到目的名稱。同 filesystem rename 可提供名稱切換的原子性，但：

- 跨 filesystem 不成立。
- 不同 OS 對覆寫既有 destination 的規則不同。
- stream flush/close 不等於 durable 到實體媒體；需要 crash durability 時還涉及 fsync file 與 parent directory。

所以範例稱為 atomic publication pattern，不誇大成斷電絕對安全。

## 範例索引

本章共 9 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`01_path.cpp`](01_path.cpp)
- [`02_path_ops.cpp`](02_path_ops.cpp)
- [`03_exists_and_status.cpp`](03_exists_and_status.cpp)
- [`04_create_remove.cpp`](04_create_remove.cpp)
- [`05_directory_iterator.cpp`](05_directory_iterator.cpp)
- [`06_recursive_iterator.cpp`](06_recursive_iterator.cpp)
- [`07_with_fstream.cpp`](07_with_fstream.cpp)
- [`08_practical_tools.cpp`](08_practical_tools.cpp)
- [`summary.cpp`](summary.cpp)

</details>

## 練習

1. 列出目錄內所有 regular .cpp files 並依 path 排序。
2. 設計是否跟隨 directory symlink 的 policy。
3. 查 POSIX fsync 規則，補出 Linux 專用 durable publish 版本。
