# ClaudeClass2026 — C++ / Python / CUDA 學習倉庫

> **倉庫**：https://github.com/hsinanchen2007/ClaudeClass2026（public，預設分支 `main`）
> **本機路徑**：`~/AI/github/ClaudeClass2026`
> **本倉庫為 2026-07-18 重建的乾淨版**（前身 `ClaudeClass` 因歷史中混入 2838 個編譯產物、`.git` 膨脹到 1.9 GB；舊倉庫暫留 `~/AI/github/ClaudeClass` 當安全網，驗證後才會刪）。
> ⚠️ 舊 repo 刪除後，本 repo **可能改名回 `ClaudeClass`**——屆時所有路徑引用（含 `~/AI/AI_Course/AI_CUDA/CLAUDE.md`）都要一起更新。
> 本版**只收原始碼與文件**（`.cpp`/`.py`/`.md`/`.pdf`）；`.exe`/`.obj`/`.pdb`/`.ilk` 等建置產物一律由 `.gitignore` 擋下，**不要提交**。

## 倉庫結構

本倉庫包含多個 C++、Python 與 CUDA 課程。C++/Python 課程每個目錄下有編號子目錄，各含多個範例檔和一個 summary 總結檔；**CUDA 課程慣例不同，見下方專節**。

### C++ 課程

| 目錄 | 課數 | 主題 | summary 狀態 |
|------|------|------|------|
| `Claude C++面向對象/` | 36 課 | 面向對象基礎到移動語意 | 36/36 全部完成 ✅ |
| `Claude C++ STL課程/` | 40 課 | STL 六大組件、vector、deque、list、forward_list | 40/40 全部完成 ✅ |
| `Claude C++ STL多線程課程/` | 5 階段 | thread、mutex、共享資料 | 5/5 全部完成 ✅ |
| `Claude C++11 到 C++26 課程/` | 15 章 (2 單元) | 型別推導、初始化、移動語意、完美轉發 | 15/15 全部完成 ✅ |

### Python 課程

| 目錄 | 課數 | 主題 | summary 狀態 |
|------|------|------|------|
| `Claude Python課程/` | 第 1~24 課 | 基礎語法（print、變數、型別、運算子、條件、迴圈） | 全部有 summary.py ✅ |
| `Claude Python課程/` | 第 25~27 課 | 串列 List（建立、存取、增刪改、排序、推導式） | 全部有 summary.py ✅ |
| `Claude Python課程/` | 第 28 課 | 元組 Tuple（不可變、拆包） | 有 summary.py ✅ |
| `Claude Python課程/` | 第 29~30 課 | 字典 Dictionary（鍵值對、操作方法） | 全部有 summary.py ✅ |
| `Claude Python課程/` | 第 31 課 | 集合 Set（去重、集合運算） | 有 summary.py ✅ |
| `Claude Python課程/` | 第 32 課 | 資料結構綜合比較與選擇 | 有 summary.py ✅ |
| `Claude Python課程/` | 第 2 課 | 安裝 Python | 無 .py 檔（純安裝說明，不需 summary） |

### CUDA 課程（慣例與上面不同，勿混用）

| 目錄 | 課數 | 主題 | 狀態 |
|------|------|------|------|
| `Claude AI CUDA課程/` | 185 課 / 13 Parts | GPU 硬體、CUDA 程式模型、記憶體階層、平行模式、函式庫、除錯、效能優化、AI 推論引擎系統 | 進行中（**2/185**，最新進度見 README.md） |
| `Codex AI CUDA課程/` | 同上 | **同一份大綱的 Codex 授課版**（獨立撰寫，非 Claude 版的翻譯或改寫） | 進行中（0/185） |

**⚠️ 雙軌並存**：兩軌用同一份大綱與課名，但**各自獨立產出、互不寫入**——使用者的目的是同一主題看兩種講法，不是比較或挑戰。改動任一軌時只碰自己那一軌。

- **課程大綱（黃金來源）**：`Claude AI CUDA課程/CUDA_AI_完整課程大綱_v2.0.6.md`
- **每課目錄**：`第 <補零課號> 課：<課名>/`，例 `第 01.01 課：為什麼需要 GPU/`
  - 補零是**刻意的**（`01.01` 而非 `1.1`）——既有課程用 `第 1 課`／`第 10 課` 會讓 `第 10 課` 排在 `第 2 課` 前面，185 課不能重蹈覆轍
- **每課內容**：`NOTE.md`（課堂筆記，自足、可整份貼進 Evernote）＋ `src/`（可編譯執行的程式碼）
  - **不適用** 上面的 `summary.cpp`／`summary.py` 規範
- **⚠️ 本 repo 是「原始基準」**：CUDA 課同時存在於 `~/AI/AI_Course/AI_CUDA_Claude/Claude AI CUDA課程/`（實驗場，可編譯、可改、允許漂移；2026-07-18 由 `AI_CUDA` 拆分而來，Codex 那軌另在 `AI_CUDA_Codex`）。
  本 repo 保持交付原樣，**只有發現內容真的有錯才修**；不要把實驗場的改動同步回來。

### 跨課程合集 summary 檔

位於各課程根目錄下：

**C++ 面向對象合集：**
- `Claude C++面向對象/summary1-5.cpp` — 第 1~5 課合集
- `Claude C++面向對象/summary6-12.cpp` — 第 6~12 課合集
- `Claude C++面向對象/summary13-19.cpp` — 第 13~19 課合集
- `Claude C++面向對象/summary20-26.cpp` — 第 20~26 課合集
- `Claude C++面向對象/summary27-35-拷貝語義.cpp` — 拷貝語義合集
- `Claude C++面向對象/summary27-35-移動語義.cpp` — 移動語義合集

**C++11-26 單元級深度教學（書級）：**
- `Claude C++11 到 C++26 課程/第 1 單元：型別推導與初始化 summary.cpp` — auto、decltype、統一初始化
- `Claude C++11 到 C++26 課程/第 2 單元：右值參考與移動語意 summary.cpp` — 15 章完整教學
- `第 2 單元：右值參考與移動語意 summary.cpp`（根目錄副本）

**Python 合集 summary（書級深度教學）：**
- `Claude Python課程/summary1-32.py` — Python 完全指南（基礎語法 + 資料結構，4 大 Part、21 章 + 附錄速查表）
- `Claude Python課程/summary25-27.py` — 串列 List 完全指南（15 章，含別名陷阱、深複製、推導式）
- `Claude Python課程/summary28.py` — 元組 Tuple 完全指南（含拆包、frozenset）
- `Claude Python課程/summary29-30.py` — 字典 Dictionary 完全指南（含計數、分組、反轉、巢狀、defaultdict）
- `Claude Python課程/summary25-32.py` — 資料結構完全指南（list/tuple/dict/set 四合一）

## summary.cpp 規範（C++）

每個課程子目錄都應有一個 `summary.cpp`，滿足以下條件：
- 自包含、可用 `g++ -std=c++17` 編譯
- 所有範例放在一個 `main()` 中（或按主題分函式）
- 繁體中文註釋覆蓋該課所有重點
- 範例簡潔但完整，看 summary.cpp 即可複習，不需看其他 .cpp 檔

## summary.py 規範（Python）

每個課程子目錄都應有一個 `summary.py`，滿足以下條件：
- 繁體中文註釋，用 `# 【重點 N】` 分區
- 每個語法點附帶可執行範例
- 用 `✅ ❌ ⚠️ ★` 標記正確/錯誤/注意/重點
- 結尾有 `# 小結` 整理重點清單

## 書級 summary 檔規範（C++ & Python 通用）

跨多章的「書級」summary 檔（如 summary1-32.py、第 2 單元 summary.cpp）：
- 從零教起，由淺入深，像一本迷你教科書
- 為不熟悉該主題的讀者設計，用最簡單但完整的範例
- 每個語法點帶詳細註釋解說
- 包含實戰模式、常見陷阱、方法速查表
- 可以不完全按課程原始範例，重點是教學效果
- 長度不限，內容完整優先

## 工作慣例

- 使用者偏好繁體中文註釋和說明
- 使用者自稱對部分進階主題（移動語意、完美轉發等）不太熟，要求用最簡單但完整的範例
- 建立 summary 時可以不完全照原始檔案的範例，重點是教學效果
- 檔案結構靈活：可以單一 summary 或拆分多個主題檔案
- 不需要每次都問使用者確認，直接建立/修改即可

### Python 課程工作流程

1. **掃描新課程**：使用者會說「再掃瞄一遍」，流程為：
   - Glob `**/*.py` 找出所有目錄
   - 比對哪些目錄有 `.py` 檔但沒有 `summary.py`
   - 讀取原始 `.py` 檔，建立 `summary.py`
2. **每課 summary.py**：放在各課程目錄下，精確涵蓋該課重點
3. **書級合集 summary**：放在 `Claude Python課程/` 根目錄，檔名如 `summary25-27.py`
   - 當使用者說「寫成一本書」時，要從零教起、由淺入深、極度詳細
   - 每個語法點獨立範例 + 詳細註釋，不要一個很長的範例
   - 結尾附速查表（方法比較、常見錯誤、選擇指南）
4. **Python 課程目前進度**：第 1~32 課（基礎語法 + 四大資料結構）全部完成
5. **舊檔改名慣例**：重寫合集時將舊檔改名為 `_old.py` 保留

## 整體進度統計

| 課程 | 子目錄數 | summary 完成 | 完成率 |
|------|----------|-------------|--------|
| C++ 面向對象 | 36 | 36 | 100% |
| C++ STL | 40 | 40 | 100% |
| C++ STL 多線程 | 5 | 5 | 100% |
| C++11-26 | 15 | 15 | 100% |
| Python | 32 | 31 | 97% |
| **總計（C++/Python）** | **128** | **127** | **99%** |
| CUDA（新，另一套慣例） | 185 課 | — | Claude 2/185（進度見 README.md） |

待處理：無（各課 summary 已補齊；`Claude Code C++by Examples/` 各主題子目錄亦皆已有 summary.cpp）

---

## 📋 教材增強工程紀錄（2026-07-19 ~ 07-20）

> **給未來的 session：先讀這節，再決定要做什麼。**
> 這裡記錄「已經對哪些檔案做過哪些加工」，以及**怎麼用指令找出還沒做的檔案**，
> 這樣下次只要處理增量，不必從頭重跑一遍（那會浪費好幾小時）。
>
> 📄 **完整作業規格與模板 → `教材增強_作業規格與模板.md`（同目錄）。**
> 使用者說「像上次那樣加工」「same request」「照模板做」時，直接讀那份，不必再問需求。

### ✅ 完成基準線（baseline）——下次從這裡接續，只做增量

- **2026-07-20 commit `44a3eb6`：全部 6 個 Claude 課程目錄 100% 完成。**
  1229 支 C++ 教材 + 71 支 Python，三種加工（面試題／編譯指令／預期輸出）全數到齊。
  最終稽核：1230 支 `.cpp` 全編過 0 失敗、71 支 Python `py_compile` 全過、
  全庫重複面試題／重複輸出區塊皆 0、輸出抽查 0 真實不符。
- **下次要更新時**：**不要整個重掃**。只處理「這個 baseline 之後**新增或改動**」的檔案——
  用下面〈找出還沒加工的檔案〉的指令即可挑出缺標記的檔（新檔一定缺 `【面試題】`）；
  已完成的檔（A/B/C 三標記各恰好一次）**跳過不動**。
  若某支被使用者要求「重做／加強」，那是個案，不影響其餘檔的 baseline。

### 已完成的三種加工

| 加工 | 標記（用來偵測是否已做） | 說明 |
|---|---|---|
| **A. 面試題區塊** | 檔案含 `【面試題】` | 每檔 2–5 題，分 🔥高頻／進階／⚠️陷阱，每題含「答／追問」 |
| **B. 編譯指令** | 檔案含 `// 編譯:`（Python 為 `# 執行:`） | 該檔**真正需要**的那條指令（含 `-std=` 與必要的 `-pthread`／`-ltbb`） |
| **C. 預期輸出** | 檔案含 `=== 預期輸出` | **實際編譯執行取得**，不是手寫 |

### 各目錄狀態

| 目錄 | 檔數 | A 面試題 | B 編譯指令 | C 預期輸出 |
|---|---:|---|---|---|
| `Claude Code C++by Examples/` | 405 | ✅ 全部 | ✅ 全部 | ✅ 全部 |
| `Claude C++面向對象/` | 281 | ✅ 全部 | ✅ 全部 | ✅ 全部 |
| `Claude C++ STL課程/` | 308 | ✅ 全部 | ✅ 全部 | ✅ 全部 |
| `Claude C++ STL多線程課程/` | 153 | ✅ 全部 | ✅ 全部 | ✅ 全部 |
| `Claude C++11 到 C++26 課程/` | 82 | ✅ 全部 | ✅ 全部 | ✅ 全部 |
| `Claude Python課程/` | 71 | ✅ 全部 | ✅ 全部 | ✅ 全部 |
| `Claude AI CUDA課程/` | — | ❌ 不適用 | 用 `BUILD.md` | 在 `NOTE.md` 內 |

> CUDA 課刻意採不同慣例（`NOTE.md` + `src/`），**不要**把面試題區塊加進去。
> `Claude AI CUDA課程/` 裡唯一的 `.cpp`（`amdahl_gustafson.cpp`）是純 host 範例，不屬 C++ 教材體系，**不加**面試題。

> **2026-07-20 全數完工**：1300 支教材檔全部三種加工到齊（1229 支 C++ 教材 + 71 支 Python；
> `Claude Code C++by Examples/` 405 支為 07-19 完成）。最終稽核：**1230 支 `.cpp` 以
> `g++ -std=c++17/20 -Wall -Wextra`（多執行緒加 `-pthread`）全數編過、0 失敗**；71 支 Python
> `py_compile` 全過；輸出抽查 0 真實不符（計時／位址／stdin 驅動皆已加但書或非決定性標註）；
> 全庫重複面試題／重複預期輸出區塊皆為 **0**。

### 🔍 找出「還沒加工」的檔案（下次從這裡開始）

```bash
cd ~/AI/github/ClaudeClass2026
# 缺面試題的 .cpp（排除 Codex 與 CUDA 課）
find . -name '*.cpp' -not -path './Codex*' -not -path './Claude AI CUDA*' \
  -exec grep -L '【面試題】' {} +
# 缺編譯指令 / 缺預期輸出
find . -name '*.cpp' -not -path './Codex*' -exec grep -L '^// 編譯:' {} +
find . -name '*.cpp' -not -path './Codex*' -exec grep -L '=== 預期輸出' {} +
# Python 版
find "Claude Python課程" -name '*.py' -exec grep -L '【面試題】' {} +
```

### 題庫位置（**不要重做研究**）

`~/Downloads/cpp_interview_research/`（不在 repo 內，屬本機工作資產）

- 我的研究：7 個主題檔，536 題；`_README.md` 為索引
- `_claude_compile_verification.md`：本機編譯器實測紀錄
- `Python_course.md`：Python 75 題（CPython 3.14.4 實測）
- `_gemini_gap_analysis.md`／`_grok_gap_analysis.md`／`_codex_gap_analysis.md`：
  與另三家題庫比對的結果（採納／駁回理由都寫在裡面）

四家比對結論：**我的題庫覆蓋率最高**（Grok 578 題有 94.5% 已被涵蓋），
Gemini 補 3 條、Grok 補 11 條、Codex 補 25 條。**投入產出已明顯遞減，不建議再找第五家。**

### ⚠️ 已知的「刻意示範」——不要把它們當 bug 修掉

有些檔案**故意**會 abort／卡住／崩潰，那就是課程要教的東西：

- `第 27 課：淺拷貝與深拷貝` → 故意 double free
- `課程 5.4：互斥鎖的常見錯誤` → 故意死鎖（會逾時）
- `課程 4.4：資料競爭範例分析` → 故意 data race
- `第 12 課：vector 元素存取` → 故意越界（libstdc++ 15 預設開 hardened assertion 會 abort）
- `C++_MultiThread/14_thread_sanitizer.cpp` → 故意保留 race，內含 spin loop 不會結束
- `18_VirtualDestructor.cpp`／`02_static_cast.cpp` → UB 示範，已改成 `-DDEMONSTRATE_UB` opt-in

**判準**：UB 不可以被描述成固定結果（不可寫「一定會洩漏」「一定 crash」）；
會卡住的程式必須明講它不會自己結束。

### 🔧 這輪修掉的真缺陷（別讓它們回來）

- **成員初始化順序**：`char* m_data;` 宣告在 `std::size_t m_len;` 之前，卻寫
  `: m_len(...), m_data(new char[m_len+1])` → 用未初始化的 `m_len` 配置記憶體。
  `bad_alloc` 只是堆積損毀的下游症狀，ASan 才看得到真因。**`-Wreorder` 早就警告了。**
- **self-deadlock**：以位址排序的轉帳函式在 `&a == &b` 時對同一把非遞迴 mutex 連鎖兩次
  （`scoped_lock(m, m)` 一樣會卡）。TSan 抓不到，因為那不是 data race。
- **`<cctype>` 前置條件**：`char` 必須先轉 `unsigned char`（UTF-8 下就是 `tolower(-61)`）。
- **`high_resolution_clock`**：libstdc++ 上別名是 **`system_clock`**（`is_steady=false`），
  不是常說的 `steady_clock`。
- 完整清單見 `~/Downloads/cpp_interview_research/_codex_gap_analysis.md`。

### 🧰 可重用的工具（在 `/tmp`，會被清掉，必要時照下面重建）

| 工具 | 用途 |
|---|---|
| `runone.sh` | 對單檔逐一嘗試 `c++17→20→23`（+`-pthread`／`-ltbb`），回報可用組合並取得輸出 |
| `collect*.sh` | 全量平行編譯執行、把輸出存到 `/tmp/out*/` |
| `verify_lines.sh` | 驗證「檔案裡貼的每一行輸出是否真的出現在實際執行結果中」 |
| `shacheck.sh` | 去註解後比對 SHA-256，證明只動註解、程式碼未變 |

**踩過的坑（重建時注意）**：
1. `-ltbb` **必須放在來源檔之後**，否則 undefined reference。
2. 每支都要在**獨立暫存目錄**執行——有些範例會寫檔（`raii_demo.txt`）污染 repo。
3. 驗證輸出**不能只比第一行**：節錄區塊本來就可能從中段開始。
4. 位址／執行緒 id／耗時每次不同 → 要加「每次執行都不同」但書，不要假裝可重現。
5. **不可把收集時的暫存路徑寫進檔案**（`/tmp/tmp.XXXX`），要換成 `<執行目錄>`。

### 🚧 共用 repo 的鐵則

本 repo 由 Claude 與 Codex 共用。**commit 一律要帶 pathspec**：

```bash
git add -- "Claude Code C++by Examples" "Claude Python課程" ...   # 列出自己的路徑
git commit -- <同樣的路徑>
```

`git add -A` 或無範圍的 `git commit` 會把對方**正在暫存中**的工作一起提交。
2026-07-20 曾差點發生（對方當時有 405 個檔案 staged），只是時間差躲過。
commit 後務必複核：`git show --name-only --format= HEAD | grep -c '^Codex'` 應為 0。
