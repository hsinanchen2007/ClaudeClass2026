# 編譯與驗證指南

## 1. 需求

- GCC 11 以上或近期 Clang，且支援 C++20。
- Bash、find、sort、mktemp、timeout。
- Linux 上的執行緒範例需要 pthread；工具會自動加入 <code>-pthread</code>。

本機驗證基準為 GCC 15 與 Clang 21。程式只使用標準 C++ 與標準函式庫。

## 2. 單檔編譯

工具會依目錄選標準：

| 目錄 | 編譯標準 |
|---|---|
| <code>C++_Cpp11</code> | <code>-std=c++11</code> |
| <code>C++_Cpp14</code> | <code>-std=c++14</code> |
| <code>C++_Cpp17</code> | <code>-std=c++17</code> |
| 其餘 | <code>-std=c++20</code> |

手動範例：

    g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread \
      C++_Algorithm/binary_search/binary_search.cpp \
      -o /tmp/codex_cpp_example
    /tmp/codex_cpp_example

不要把 <code>-o</code> 指向 source file，也不要把 extensionless executable 留在課程目錄。

## 3. 全部編譯

    ./tools/check_all.sh

成功時退出碼為 0；任一檔案編譯失敗時退出碼為 1；參數或環境錯誤為 2。

先做教材與 1:1 映射稽核：

    ./tools/audit_textbook.sh

此工具會動態比較同一 repository 的 Claude/Codex `.cpp` 相對路徑，也會檢查每檔
必要教學段落、所有 summary 深度、是否誤留 ELF，以及是否有逐位元組複製。

新增或移除教材後，重建並驗證 20 份章節索引：

    ./tools/update_readme_indexes.sh
    ./tools/update_readme_indexes.sh --check

## 4. 編譯並執行

    ./tools/check_all.sh --run

    # 以 -O2 -DNDEBUG 執行全部範例，確認必要操作沒有誤放在 assert(...) 內
    ./tools/check_all.sh --release

每個範例有 10 秒上限。範例不讀 stdin、不連網，filesystem 範例只操作自己的暫存目錄並清理。
範例以 `assert` 驗證多數結果；學習與一般回歸測試時不要加入 `-DNDEBUG`，否則
assertion 與其 expression 都會被編譯器移除。`--release` 的目的不同：確認移除測試
斷言後，程式仍不會因遺漏必要操作而失效、卡住或觸發 UB。一般模式仍把所有 warning
視為錯誤；release 模式只抑制由 assertion 消失自然造成的 unused local/function warning。

教材本身仍遵守一條更重要的規則：`assert` 只能驗證內部不變量與測試結果，不能作為
外部輸入的唯一防線，也不能把會影響安全性的必要操作只寫在 `assert(...)` 裡。公開
函式的長度、範圍、空值與 overflow 前置條件必須在 release build 仍以 `if` 驗證。

## 5. 預覽命令

    ./tools/check_all.sh --dry-run

這只列出命令，不建立暫存目錄、不編譯、不執行。

## 6. Clang 交叉驗證

    CXX=clang++ ./tools/check_all.sh --run

GCC 與 Clang 都通過，可降低漏寫標頭、依賴編譯器 extension 或誤用語言標準的機率。

## 7. Sanitizer

全課以 AddressSanitizer 與 UndefinedBehaviorSanitizer 編譯、執行：

    ./tools/check_all.sh --sanitize

工具沿用各目錄的 C++11/14/17/20 標準，並在第一個 sanitizer 錯誤時讓該程式
失敗；其他檔案仍會繼續檢查，最後統一回報成功/失敗數。

GCC 15 在 `-O1` 與 sanitizer 同時啟用時，會對 libstdc++ 的 `<regex>` 內部實作
產生 `-Wmaybe-uninitialized` 假陽性。工具只在 **GCC sanitizer 模式**加入
`-Wno-maybe-uninitialized`；一般 GCC、Clang 與 release 模式仍保留完整的
`-Werror`。這個例外不會關閉 ASan/UBSan，也不代表教材可忽略未初始化資料。

對正在修改的單一範例，可另外執行：

    g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow -pthread \
      -fsanitize=address,undefined -fno-omit-frame-pointer \
      C++_Algorithm/binary_search/binary_search.cpp \
      -o /tmp/codex_cpp_sanitized
    /tmp/codex_cpp_sanitized

多執行緒資料競爭需用 ThreadSanitizer 單獨建置，不要和 AddressSanitizer 混用。
