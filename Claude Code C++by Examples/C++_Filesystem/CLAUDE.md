# C++ Filesystem 學習專案 (C++17)

## 使用者需求 / 規則

> 5 條專案永久規則。

1. **永遠用繁體中文回答**。
2. **每個主題加入「簡單實用」的範例**。
3. **每個程式只談一個主題**。
4. **範例要簡單直接**。
5. **參考來源** —
   - https://en.cppreference.com/w/cpp/filesystem
   - https://cplusplus.com/reference/filesystem/

## 編譯需求

* C++17 起 `<filesystem>` 標準化（命名空間 `std::filesystem`）
* GCC 8 / Clang 9 之前要連結 `-lstdc++fs`（GCC 9+ / Clang 11+ 不必）
* 本專案 Makefile 預設 C++17，現代 GCC 不需特別 link

## 專案結構

```
C++_Filesystem/
├── CLAUDE.md
├── Makefile
├── 01_path.cpp                  # path 基本：建構、轉字串、append
├── 02_path_ops.cpp              # parent_path / filename / extension / replace_extension / lexically_normal
├── 03_exists_and_status.cpp     # exists / is_directory / file_size / status
├── 04_create_remove.cpp         # create_directory(s) / remove / remove_all / copy / rename
├── 05_directory_iterator.cpp    # 列目錄
├── 06_recursive_iterator.cpp    # 遞迴列目錄 + skip permission denied
├── 07_with_fstream.cpp          # 配合 fstream 讀寫
└── 08_practical_tools.cpp       # 工作小工具：找最大檔、依副檔名分類
```

## 速查表

| 想做 | API |
|---|---|
| 建立 path | `fs::path p{"/a/b"}` |
| 接 path | `p / "sub" / "file.txt"` |
| 是否存在 | `fs::exists(p)` |
| 檔案大小 | `fs::file_size(p)` |
| 建多層目錄 | `fs::create_directories(p)` |
| 刪掉檔案 | `fs::remove(p)` |
| 遞迴刪資料夾 | `fs::remove_all(p)` |
| 拷貝 | `fs::copy(src, dst, options)` |
| 列目錄 | `fs::directory_iterator` |
| 遞迴列 | `fs::recursive_directory_iterator` |
| 取目前工作目錄 | `fs::current_path()` |
| 取暫存目錄 | `fs::temp_directory_path()` |

## 編譯

```bash
make
make run
make clean
```
