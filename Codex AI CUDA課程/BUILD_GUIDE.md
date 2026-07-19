# Codex AI CUDA 課程編譯指南

本文件說明如何在
`~/AI/AI_Course/AI_CUDA_Codex` 編譯 Codex 課程中的範例程式。
全部命令都可在一般終端執行，不需要啟動 VS Code 或 Cursor，也不要用
`sudo` 執行課程工具。

> **三份一致**：本檔是本機主檔；`Codex AI CUDA課程/BUILD_GUIDE.md` 與
> GitHub Codex 課程內同名檔是發布副本，由 `sync_lesson.sh --publish
> BUILD_GUIDE.md` 同步。課程大綱也以根層 `outline/` 版為主檔。建置工具
> `build.sh`、`check_build.sh` 同樣以工作區根層 `tools/` 為主檔，由
> `sync_lesson.sh --publish build-tools` 發布到本機與 GitHub 課程內的 `tools/`。
> `sync_lesson.sh`、`finish_lesson.sh` 涉及本機雙 repo 交付，維持私有、不發布。

## 1. 工具分工

| 工具 | 用途 | 是否保留產物 |
|---|---|---|
| `tools/check_build.sh` | 掃描全部課程或指定一課，稽核 C++/CUDA/Python 並執行每課 `verify.sh` | 否；使用 `/tmp` 後自動清理 |
| `tools/build.sh` | 安全編譯單一 `.cu/.cpp/.cc/.cxx`，供本機實驗或建立多架構 binary | 是；預設放在該課 `build/`；發布到 GitHub |
| `tools/sync_lesson.sh` | 比較或發布兩份 Codex 課程內容 | 不負責編譯 |
| `tools/finish_lesson.sh` | 驗共用 README、單課編譯、發布、雙 repo commit 與 GitHub push | 依流程執行 |

`build/`、object、fatbin 等可重建產物已由根層 `.gitignore` 排除，不應提交。

## 2. 前置檢查

~~~bash
cd ~/AI/AI_Course/AI_CUDA_Codex
nvidia-smi
nvcc --version
g++ --version
~~~

目前課程基準是 CUDA 13.3、GCC 15、C++20。本機 GPU 是 sm_75；sm_89、sm_90 與
sm_120 只能在本機做交叉編譯，執行正確性與效能仍須在相應實體 GPU 驗證。

## 3. 編譯全部課程程式碼

先預覽會執行哪些命令，不實際編譯：

~~~bash
cd ~/AI/AI_Course/AI_CUDA_Codex
tools/check_build.sh --dry-run
~~~

正式稽核所有課程：

~~~bash
tools/check_build.sh
~~~

只有 GitHub 的 `Codex AI CUDA課程/` clone 時，建置工具就在該目錄內，命令
相同：

~~~bash
cd "Codex AI CUDA課程"
tools/check_build.sh
~~~

預設行為：

- 遞迴掃描 `Codex AI CUDA課程/第 * 課：*/src/`。
- 每個 `.cu` 分別以 `sm_75`、`sm_89`、`sm_90`、`sm_120` 編譯。
- 每個 `.cpp/.cc/.cxx` 以 C++20 + warnings 編譯；`.py` 以 `py_compile` 做語法驗證。
- 課程有可執行 `verify.sh` 時，會再執行 runtime/數值/錯誤路徑等專屬測試。
- 有內容精確為 `codex-custom-verify-v1` 的 `.codex-custom-verify` 時，只執行該課 `verify.sh`，用於多檔、CMake、Triton 或特殊函式庫。
- 編譯產物放在短命暫存目錄，結束時自動刪除。
- 任一來源或 verifier 失敗即回傳 1；掃描權限/find 失敗則 fail-closed 回 2。

只稽核指定課程：

~~~bash
tools/check_build.sh \
  --lesson "第 01.01 課：為什麼需要 GPU"
~~~

暫時只驗特定架構：

~~~bash
tools/check_build.sh \
  --lesson "第 01.01 課：為什麼需要 GPU" \
  --arch "75 120"
~~~

## 4. 建立單一 C++ 或 CUDA 範例

從該課目錄執行。預設建立包含 `sm_75 + sm_89 + sm_90 + sm_120` SASS 與
`compute_120` PTX 的多架構 binary：

~~~bash
cd "$HOME/AI/AI_Course/AI_CUDA_Codex/Codex AI CUDA課程/第 01.01 課：為什麼需要 GPU"
../tools/build.sh src/latency_throughput.cu
./build/latency_throughput
~~~

只針對目前 GPU 編譯，適合本機快速測試：

~~~bash
../tools/build.sh src/latency_throughput.cu --native
./build/latency_throughput
~~~

指定輸出位置：

~~~bash
../tools/build.sh src/latency_throughput.cu \
  --native -o /tmp/latency_throughput
/tmp/latency_throughput
~~~

只預覽完整 `nvcc` 命令：

~~~bash
../tools/build.sh src/latency_throughput.cu --native --dry-run
~~~

同一支工具也能編譯純 C++。預設使用 C++20、`-O3` 與完整 warnings；加
`--native` 時會額外使用 `-march=native`，產物不保證能搬到不同 CPU：

~~~bash
../tools/build.sh src/scaling_laws.cpp
./build/scaling_laws

../tools/build.sh src/scaling_laws.cpp --native -o /tmp/scaling_laws
/tmp/scaling_laws
~~~

預設輸出規則會保留 `src/` 下的相對子目錄並移除來源副檔名。例如
`src/chapter/a.cpp` 會輸出到 `build/chapter/a`，因此同名但位於不同子目錄的
範例不會互相覆蓋。

### 4.1 輸出安全保證

`build.sh` 不會直接把 compiler output 指向最終檔。它會先在最終目錄建立
`.codex-build.XXXXXX` 短命檔，編譯成功且重新通過安全檢查後，再以同檔案系統
的原子 rename 發布；編譯失敗時，既有 binary 保持不變。

即使使用 `-o`，工具也會在啟動 compiler 前拒絕：

- 來源檔本身或任何課程的 `src/` 來源樹。
- symlink、具有多個 hardlink 的既有檔案，以及與來源相同 inode 的檔案。
- Codex repo 內任何 Git tracked file 或 `.git/` metadata。
- 常見原始碼、腳本、設定與文件副檔名，例如 `.cu`、`.cpp`、`.sh`、`.md`。

因此不要用 `-o src/example.cpp`；正確目的地應是無來源副檔名的 executable，
例如 `build/example` 或 `/tmp/example`。`--dry-run` 也會執行這些路徑檢查，但
不建立輸出目錄或短命檔。

## 5. 手動等價編譯命令

GitHub 發布副本已包含安全版 `tools/build.sh` 與 `tools/check_build.sh`，一般不必
手寫 compiler 命令。以下仍列出透明的等價命令，供除錯、理解旗標，或處理尚未
納入工具的特殊環境。針對目前 GPU 建立 CUDA binary：

~~~bash
mkdir -p build
nvcc -std=c++20 -O3 -arch=native \
  -Xcompiler=-Wall,-Wextra \
  src/latency_throughput.cu -o build/latency_throughput
./build/latency_throughput
~~~

使用 CUDA 13.x 建立本課程的多架構 binary：

~~~bash
nvcc \
  -gencode arch=compute_75,code=sm_75 \
  -gencode arch=compute_89,code=sm_89 \
  -gencode arch=compute_90,code=sm_90 \
  -gencode arch=compute_120,code=sm_120 \
  -gencode arch=compute_120,code=compute_120 \
  -std=c++20 -O3 -Xcompiler=-Wall,-Wextra \
  src/latency_throughput.cu \
  -o build/latency_throughput
~~~

較舊 Toolkit 可能不認得 `sm_120`；不要在不理解相容性影響時直接刪掉架構，
應先看該課 `NOTE.md` 與大綱硬體門檻。純 C++ 範例可直接使用：

~~~bash
g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic \
  src/scaling_laws.cpp -o build/scaling_laws
./build/scaling_laws
~~~

## 6. 純 C++ 與特殊依賴課程

`tools/build.sh` 接受單一 `.cu/.cpp/.cc/.cxx`，適合不需要額外 linker flags 的
獨立範例；`tools/check_build.sh` 仍會自動稽核全部一般單檔來源。手動建立純 C++
範例可直接使用：

~~~bash
../tools/build.sh src/scaling_laws.cpp
./build/scaling_laws
~~~

需要 CMake、額外 linker flags、cuBLAS、cuDNN、NCCL、PyTorch extension、Triton 或其他
函式庫的課程，不能假設通用單檔命令足夠。該課必須在 `NOTE.md` 提供可重現的
建置與執行命令，並提供支援 `--dry-run` 的可執行 `verify.sh`。若通用單檔掃描
本身不適用，建立：

~~~bash
printf '%s\n' codex-custom-verify-v1 > .codex-custom-verify
chmod 755 verify.sh
~~~

`verify.sh` 就成為該課的權威驗證入口；它必須自行涵蓋建置、runtime 正確性與
必要的錯誤路徑。課 1.1 的 `verify.sh` 是可直接參考的最小範例。

## 7. 正式完成一課

先確認根層 `PROGRESS.md` 與
`~/AI/github/ClaudeClass2026/README.md` 的 Codex 進度區都已更新，再執行：

~~~bash
cd ~/AI/AI_Course/AI_CUDA_Codex
tools/finish_lesson.sh \
  --lesson "第 01.01 課：為什麼需要 GPU" \
  --message "課程 1.1：完成 GPU 平行運算基礎"
~~~

這支工具會：

1. 對照 `PROGRESS.md` 驗證共用 README 的完成數、下一課與該課表格列，並確認
   Claude 區及 Codex 區外的內容沒有未提交變更。
2. 驗證課名、`NOTE.md`、編譯指定課的來源，並執行專屬 `verify.sh`。
3. 只把該課發布到
   `~/AI/github/ClaudeClass2026/Codex AI CUDA課程/`。
4. 驗證兩份完整 Codex 課程內容一致。
5. 在本機 repo 提交該課與根層 `PROGRESS.md`；在 GitHub repo 只提交該課的
   Codex 路徑與根層 `README.md`。兩邊都以 pathspec 和 commit 後 allowlist 限定。
6. 以 Git CLI push，並驗證遠端 branch 與本地 HEAD 相同。

整個流程不依賴任何編輯器。若只想預演，可加 `--dry-run`；若只想 commit 而
暫不 push，可加 `--no-push`。

共用 README 不由 `sync_lesson.sh` 鏡像，也不複製進 Codex 課程目錄；它是
`ClaudeClass2026` 的共同入口。Codex 只能改自己的進度區。交付工具在編譯前與
stage 後都會檢查區塊邊界，避免夾帶 Claude 或其他共用內容。

更新本指南時只編輯工作區根層主檔，再精確發布，不必手動維護另外兩份，也
不必鏡像整個課程：

~~~bash
tools/sync_lesson.sh --publish BUILD_GUIDE.md --dry-run
tools/sync_lesson.sh --publish BUILD_GUIDE.md
~~~

建置工具也只能修改根層主檔，再發布兩支 allowlist 工具：

~~~bash
tools/sync_lesson.sh --publish build-tools --dry-run
tools/sync_lesson.sh --publish build-tools
~~~

課程大綱同樣只編輯根層 `outline/` 版：

~~~bash
tools/sync_lesson.sh --publish CUDA_AI_完整課程大綱_v2.0.6.md --dry-run
tools/sync_lesson.sh --publish CUDA_AI_完整課程大綱_v2.0.6.md
~~~

同步與交付工具會驗證兩個 Codex 目錄的 `.codex-publish-target`、目錄名與
realpath，並使用交付鎖。任一 marker 不正確都不會執行 `rsync --delete`。

## 8. 退出碼與判讀

| 退出碼 | 一般意義 |
|---|---|
| 0 | 所選流程成功 |
| 1 | 編譯、同步、Git 或 push 發生實際失敗 |
| 2 | 參數、路徑、工具鏈、輸出碰撞或其他安全前置條件錯誤 |

請以各工具的 `--help` 為最終準則：

~~~bash
tools/build.sh --help
tools/check_build.sh --help
tools/sync_lesson.sh --help
tools/finish_lesson.sh --help
~~~

最後要分清三件事：**編譯成功不等於執行正確，執行正確不等於效能最佳，兩份
檔案一致也不等於 GitHub 遠端已 push**。正式交付流程會分別驗證這些邊界。
