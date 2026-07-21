# Codex C++ by Examples 工作規範

本目錄由 Codex 獨立維護。它與同一個 repository 內的 Claude 教材並存，但不是其副本，也不互相覆寫。

新增或加強教材前，必須先讀 `CODEX_CPP_TEXTBOOK_TEMPLATE.md`。該檔是教科書內容、
LeetCode／實務案例、面試問答、編譯命令與預期輸出的完整格式契約；本檔負責工作邊界
與驗證基線。兩者若有衝突，以較嚴格且不破壞 C++ 正確性的要求為準。

## 邊界

1. 只修改 <code>Codex C++by Examples/</code> 內的教材與工具。
2. 不修改任何 <code>Claude ...</code> 目錄，也不以 Codex 版本「修正」Claude 原作。
3. 所有說明使用繁體中文；程式識別字使用清楚的英文。
4. 每個範例必須自包含、可編譯、可執行，而且不能依賴網路或互動輸入。
5. 建置產物只能放在 <code>/tmp</code> 或工具建立的暫存目錄，不得寫在原始碼旁。
6. C++11、C++14、C++17 章使用對應標準；其餘章以 C++20 為教學基準。
7. 每次改動後至少執行 <code>tools/check_all.sh</code>；重要變更再以 Clang 交叉驗證。

## 教學品質

- 先說明問題與選擇準則，再介紹語法。
- 清楚標示生命週期、iterator/reference 失效、例外安全、複雜度與 undefined behavior。
- 危險寫法只能放在註解中，不能讓預設可執行路徑觸發 UB、資料競爭或資源洩漏。
- `assert` 只用於測試與內部 invariant；外部輸入、空範圍、索引、數值溢位及生命週期
  契約必須有 release build 仍存在的檢查。必要操作不可只放在 `assert(...)` 內。
- `*.cpp` 相對路徑必須與 `Claude Code C++by Examples/` 維持 1:1；只參考題目與
  檔名，不複製 Claude 的文字或程式內容。Claude 新增或移除題目後，必須執行
  `tools/audit_textbook.sh` 找出漂移。
- 每個 `.cpp` 本身就是離線教科書章節：即使讀者沒有 README、書籍或網路，也要能
  從繁體中文註解學到基本原理、API/複雜度/生命週期、易錯處、面試重點與練習。
  自動守衛目前要求一般檔至少 70 行且至少 12 行含中文，`summary.cpp` 至少 150 行且
  至少 35 行含中文；這只是最低門檻，不能以拆行或重複文字代替實質內容。
- 每個 `.cpp` 都要有可執行的基礎示範、真正實作並測試的短 LeetCode 題，以及真正
  實作並測試的工作案例；不能只寫題號或一句「可用在哪裡」。
- 每個 `summary.cpp` 是面試前獨立速讀章：涵蓋同章所有檔案的規則、選型、複雜度、
  陷阱、面試問答與整合程式，不得只是數十行索引或單一小範例。
- 範例保持小而完整；README 負責學習順序，source code 同時負責講義與可操作做法。
- 不把特定編譯器的偶然行為描述成 C++ 標準保證。

## 交付門檻

1. `tools/audit_textbook.sh`：路徑 1:1、教材段落、summary 深度、無 ELF、無直接複製。
2. 新增/移除 `.cpp` 後執行 `tools/update_readme_indexes.sh`，再用 `--check` 驗無漂移。
3. `tools/check_all.sh --run`：GCC 全部編譯並執行。
4. `tools/check_all.sh --release`：以 `-O2 -DNDEBUG` 確認 assert 移除後仍可完整執行。
5. `CXX=clang++ tools/check_all.sh --run`：Clang 交叉驗證。
6. 建置輸出只在暫存目錄；交付前確認 repository 無新 executable。

## 最近完整驗證基線（2026-07-20）

本節用來避免後續 session 在內容未變時，從零重跑 405 個範例。教材內容基線是
GitHub repo commit `e8cdf8a9010d77d782a600112cc08d997fcaf4ad`；新增本節的
AGENTS-only commit 不算教材變更。

已完成並通過：

- `tools/audit_textbook.sh`：Claude/Codex 各 405 個 `.cpp`、29 個對應目錄，
  相對路徑 1:1；教材段落、summary 深度、無 ELF、無逐位元組複製均通過。
- `tools/update_readme_indexes.sh --check`：20 個頂層主題索引無漂移。
- `tools/check_all.sh --run`：GCC debug 編譯與執行 405/405 通過。
- `tools/check_all.sh --release`：GCC `-O2 -DNDEBUG` 編譯與執行 405/405 通過。
- `CXX=clang++ tools/check_all.sh --run`：Clang 編譯與執行 405/405 通過。
- ASan+UBSan：405/405 通過；Codex 執行環境受 ptrace 影響，LeakSanitizer 會固定報
  `LeakSanitizer does not work under ptrace`，因此本輪以 `detect_leaks=0` 驗 ASan/UBSan。
  真正 LSan 仍應在不受 ptrace 的一般終端執行 `tools/check_all.sh --sanitize`。
- 面試題宣告已回 commit `ff8349e` 重算：29 份 summary 新增 331 題；24 個單課檔案
  新增 72 題。現況可辨識的 summary 深挖題頭共有 369，README 的「新增 331」是
  歷史新增量，不是現況總題數。

後續重驗規則：

1. 先執行
   `git diff --quiet e8cdf8a9010d77d782a600112cc08d997fcaf4ad -- 'Codex C++by Examples' ':(exclude)Codex C++by Examples/AGENTS.md'`。
   若無差異，可引用本基線，不必重跑全部 405 檔。
2. 若只有少數 `.cpp` 改動，先跑 `audit_textbook.sh`、索引 `--check`，再用該章應有的
   C++ 標準對變更檔做 GCC/Clang debug、release 與 sanitizer；提交前仍至少跑一次
   `tools/check_all.sh --run`。
3. 若 `tools/`、共用編譯旗標、教材映射規則或大量檔案改動，完整重跑上述 GCC、
   release、Clang、sanitizer 與結構稽核，並以新 commit 取代本基線。
