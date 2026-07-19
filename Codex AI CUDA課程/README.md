# Codex AI CUDA 課程

這是 Codex 依 [`CUDA_AI_完整課程大綱_v2.0.6.md`](CUDA_AI_完整課程大綱_v2.0.6.md) 獨立產出的 AI/CUDA 課程。目錄內含自己的規則、進度、工具、範本與參考資源，不依賴父層共用檔案，也不讀寫 Claude 軌。

## 兩份副本

| 角色 | 路徑 |
|---|---|
| 實驗場 | `/home/hsinan/AI/AI_Course/AI_CUDA/Codex AI CUDA課程` |
| 發佈基準 | `/home/hsinan/AI/github/ClaudeClass2026/Codex AI CUDA課程` |

實驗場可以暫時修改與測試；正式交付時必須同步到發佈基準並驗證一致。Claude 軌不在同步範圍內。

## 目錄

```text
Codex AI CUDA課程/
├── .codex-course-root             # 防止工具誤用其他根目錄
├── CLAUDE.md                      # Codex 軌規則
├── AGENTS.md -> CLAUDE.md
├── README.md
├── PROGRESS.md                    # Codex 軌唯一進度來源
├── CUDA_AI_完整課程大綱_v2.0.6.md
├── resources/
├── templates/
├── tools/
└── 第 XX.YY 課：課名/
    ├── NOTE.md
    └── src/
```

## 常用工作流

在課程根目錄稽核單課的所有 CUDA 目標架構：

```bash
tools/check_build.sh --lesson "第 01.01 課：為什麼需要 GPU"
```

在課程目錄內編譯本機版本；預設輸出到該課 `build/`，不污染 `src/`：

```bash
cd "第 01.01 課：為什麼需要 GPU"
../tools/build.sh src/latency_throughput.cu --native
./build/latency_throughput
```

驗證兩份 Codex 副本：

```bash
tools/sync_lesson.sh
```

先預覽、再正式發佈單課：

```bash
tools/sync_lesson.sh --publish "第 01.01 課：為什麼需要 GPU" --dry-run
tools/sync_lesson.sh --publish "第 01.01 課：為什麼需要 GPU"
```

完整交付流程：

```bash
tools/finish_lesson.sh \
  --lesson "第 01.01 課：為什麼需要 GPU" \
  --message "課程 1.1：完成 GPU 平行運算基礎"
```

每支工具的完整選項、退出碼與安全邊界請以其 `--help` 為準。
