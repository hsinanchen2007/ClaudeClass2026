# Codex AI CUDA 課程專屬規則

> 本檔只管理 `Codex AI CUDA課程/`。`AGENTS.md` 指向本檔，供不同代理讀取同一套規則。
> 一律以繁體中文回覆使用者，教材文字與程式碼註解也以繁體中文為主。

## 所有權與邊界

1. 本目錄是 **Codex 專屬、自足的課程根目錄**。規則、進度、工具、範本與資源都以本目錄版本為準。
2. 不讀取、比較、修改或「修正」`Claude AI CUDA課程/` 的內容；兩軌可有不同教法與實驗。
3. 父層的 `CLAUDE.md`、`PROGRESS.md`、`tools/`、`templates/`、`resources/` 不是 Codex 軌的執行依賴，也不是 Codex 進度的權威來源。
4. Codex 軌有兩份副本：
   - 實驗場：`/home/hsinan/AI/AI_Course/AI_CUDA/Codex AI CUDA課程`
   - 發佈基準：`/home/hsinan/AI/github/ClaudeClass2026/Codex AI CUDA課程`
5. 每次正式交付時，兩份 Codex 副本必須逐位元組一致；實驗中的暫時漂移必須在交付前消除。
6. Git 只加入本 Codex 目錄內的必要變更，不可用會順帶提交另一軌或使用者其他變更的全域 `git add -A`。

## 黃金來源

- 課程大綱：`CUDA_AI_完整課程大綱_v2.0.6.md`（185 課、13 Parts）
- Codex 進度：`PROGRESS.md`
- 課程範本：`templates/NOTE_template.md`
- NVIDIA/AMD 對照：`resources/NVIDIA_AMD_術語與概念對照表.md`
- 編譯與交付工具：`tools/`

## 授課規則

1. 使用者一次指定一課；未獲明確指示不得自行推進下一課。
2. 每課至少包含可獨立閱讀的 `NOTE.md`。有程式實驗時放在該課 `src/`，不得依賴另一軌的檔案。
3. `NOTE.md` 必須交代學習目標、核心原理、可重現步驟、實際輸出、結果解讀、硬體限制、自我檢核與參考來源。
4. 範例必須實際編譯；能在本機執行者也必須執行並核對結果。不得把「可編譯」寫成「已在目標 GPU 驗證」。
5. CUDA 程式至少稽核 `sm_75`、`sm_89`、`sm_120` 編譯；本機只能對 `sm_75` 做真實 GPU 執行驗證。未來平台的數值只能標為預期、推導或待實測。
6. 基準採 C++20。效能數據需列硬體、軟體版本、資料規模、暖機、重複次數、計時邊界與同步位置。
7. 優先使用 NVIDIA 官方文件、CUDA Toolkit 隨附 header/sample、正式論文與專案官方文件；時效性規格須重新查證並附日期。
8. 每課完成後在本地 `PROGRESS.md` 新增一行「我證明了什麼」，再同步到發佈基準。

## 本機與目標硬體

| 用途 | 環境 |
|---|---|
| 主要實驗機 | Ubuntu 26.04、Quadro RTX 4000 Mobile、Turing `sm_75`、8GB VRAM |
| CUDA 基準 | NVIDIA open driver 610.43.02、CUDA Toolkit 13.3、GCC 15、C++20 |
| 未來桌機 | RTX 4090 `sm_89`、RTX 5090 `sm_120`；取得前只能做編譯與文件層驗證 |

## 常用命令

在本 Codex 課程根目錄執行：

```bash
tools/check_build.sh --lesson "第 01.01 課：為什麼需要 GPU"
tools/sync_lesson.sh
tools/sync_lesson.sh --publish "第 01.01 課：為什麼需要 GPU" --dry-run
```

在單一課程目錄執行：

```bash
../tools/build.sh src/example.cu --native
```

`tools/finish_lesson.sh` 會串接編譯、同步、驗證、兩個 repo 的範圍限定 commit，以及有遠端時的 push 驗證；正式使用前先看 `--help`。
