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
