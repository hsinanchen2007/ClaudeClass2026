# 編譯與交付指南

本工作區所有課程範例的編譯方式。**四支工具都在工作區的 `tools/`**（本機才有；從 GitHub clone 下來不會有這個目錄——
見〈五、不用工具，手動編譯〉），全部支援 `--help` 與 `--dry-run`。

> 📍 **路徑說明**：本文的 `tools/...` 指的是**工作區根目錄**下的 `tools/`。
> 本檔位於 `Claude AI CUDA課程/`，所以從這裡執行要寫 `../tools/...`，
> 或先 `cd` 回工作區根目錄再執行（下方範例皆以**工作區根目錄**為工作目錄）。

- **本機環境**：Ubuntu 26.04｜Quadro RTX 4000（**sm_75**, 8GB）｜CUDA 13.3（nvcc V13.3.73）｜Driver R610｜gcc 15.2
- **C++ 標準**：**C++20**（`build.sh` 與 `check_build.sh` 皆已對齊）
- **目標架構**：`sm_75`（本機）＋ `sm_89`（4090）＋ `sm_120`（5090）— 所有範例都必須能編到這三個

---

## 一、單一檔案：`tools/build.sh`

日常寫程式、跑單一範例用這支。

```bash
# 只為當下這台機器編（最快，開發時用）
tools/build.sh "Claude AI CUDA課程/第 01.01 課：為什麼需要 GPU/src/gpu_vs_cpu_profile.cu" --native

# 三架構 fatbin（sm_75 + sm_89 + sm_120 + PTX）→ 換機器不用重編
tools/build.sh <檔案.cu>

# 指定輸出檔名
tools/build.sh <檔案.cu> --native -o /tmp/myprog
```

**兩種模式的差別**：

| 模式 | 產出 | 什麼時候用 |
|---|---|---|
| `--native` | 只含當前 GPU 的 SASS | 開發迭代（編譯最快） |
| 預設（fatbin） | sm_75 + sm_89 + sm_120 + PTX | 要在多台機器跑、或交付前 |

> 💡 fatbin 把多個架構的機器碼打包進同一個執行檔，runtime 自動挑對應的那份。
> 所以**同一個執行檔**在本機（sm_75）與未來桌機（sm_120）都能直接跑，不用重編。

**純 host 程式（`.cpp`）不需要 nvcc**：
```bash
g++ -std=c++20 -O2 <檔案.cpp> -o <輸出>
```

---

## 二、全部範例一次稽核：`tools/check_build.sh`

**交付前必跑**。掃描所有課程的 `src/`，`.cu` 對三個架構各編一次、`.cpp` 用 g++ 編。

```bash
tools/check_build.sh                                     # 稽核全部課程
tools/check_build.sh --lesson "第 01.01 課：為什麼需要 GPU"   # 只查單一課
tools/check_build.sh --arch "75 120"                     # 只編指定架構
tools/check_build.sh --dry-run                           # 只列出會編哪些檔
```

實際輸出：
```
課程    ：Claude AI CUDA課程
架構    ：75 89 120
C++ 標準：c++20

── 第 01.01 課：為什麼需要 GPU
     ✅ amdahl_gustafson.cpp             (g++ host)
     ✅ gpu_vs_cpu_profile.cu            sm_75
     ✅ gpu_vs_cpu_profile.cu            sm_89
     ✅ gpu_vs_cpu_profile.cu            sm_120

═══ 結果：成功 4 ／ 失敗 0 ／ 略過 0 ═══
✅ 全部編譯成功（含 sm_120，未來桌機可直接編）
```

**退出碼**：`0` 全過｜`1` 有編譯失敗｜`2` 用法錯誤

> ⚠️ **誠實邊界**：這只保證**編得過**。本機沒有 sm_89／sm_120 的實體卡，
> 那些架構的**執行結果未經驗證**——拿到桌機後仍要在該機重跑各課的「動手做」。

---

## 三、雙寫驗證：`tools/sync_lesson.sh`

課程內容存在兩個位置，這支確認它們一致。

| 路徑 | 角色 |
|---|---|
| `~/AI/AI_Course/AI_CUDA_Claude/Claude AI CUDA課程/` | **實驗場**——你在這裡編譯、改、亂試（**允許漂移**） |
| `~/AI/github/ClaudeClass2026/Claude AI CUDA課程/` | **原始基準**——保持交付原樣，push 到 GitHub |

```bash
tools/sync_lesson.sh                        # 驗證兩邊是否一致（唯讀）
tools/sync_lesson.sh --publish "<課別>"     # 發佈某課到原始基準
tools/sync_lesson.sh --publish "<課別>" -n  # 先預覽
```

**輸出怎麼讀**：

| 符號 | 意思 |
|---|---|
| 🟡 尚未發佈 | 實驗場有、基準沒有 → 多半是新課還沒發佈 |
| 🔴 只存在於基準 | 基準有、實驗場沒有 → 實驗場被誤刪？要查 |
| ⚠️ 內容有差異 | 兩邊都有但內容不同 → **你的實驗漂移，通常正常** |

> ⚠️ 報出差異**不一定是錯**——實驗場的漂移是刻意設計。
> 只有「新課還沒發佈」或「確認內容真的有錯」才需要 `--publish`。

---

## 四、完整交付：`tools/finish_lesson.sh`

**上完一課用這支**，它把 6 個步驟串起來，任一步失敗就中止並指出位置。

```bash
tools/finish_lesson.sh --lesson "第 01.01 課：為什麼需要 GPU" -m "<commit 訊息>"
tools/finish_lesson.sh --lesson "<課別>" --dry-run    # 先走一遍不 commit/push
```

流程：

```
① 編譯稽核（sm_75/89/120 全過）      ← 呼叫 check_build.sh
② 發佈到原始基準（雙寫）             ← 呼叫 sync_lesson.sh --publish
③ 雙寫驗證（逐位元組）
④ 兩個 repo 各自 commit
⑤ push 到 GitHub
⑥ 驗證 push 真的成功（比對本地 HEAD 與 origin/main）
```

**⑥ 是關鍵**：不是「push 指令沒報錯」就算數，而是實際確認 GitHub 上真的更新了。
這樣**不需要開 VS Code/Cursor**，新課就會自動出現在 GitHub 上。

> `AI_CUDA_Claude` 是純本機 repo（無遠端），工具會自動略過它的 push；
> `ClaudeClass2026` 才是 GitHub 上那份。

---

## 五、不用工具，手動編譯（**從 GitHub clone 時用這節**）

工具只是包裝，底下就是標準 nvcc。
**GitHub 上的 `ClaudeClass2026` 只收筆記與範例、不含 `tools/`**，所以在那邊 clone 下來時
（例如 Windows 筆電、或未來的 RTX 5090 桌機）直接用下面的指令即可：

```bash
# 本機 sm_75
nvcc -std=c++20 -O3 -arch=sm_75 src/xxx.cu -o xxx

# 未來桌機 sm_120（RTX 5090）
nvcc -std=c++20 -O3 -arch=sm_120 src/xxx.cu -o xxx

# 自動偵測當前 GPU
nvcc -std=c++20 -O3 -arch=native src/xxx.cu -o xxx

# 三架構 fatbin（一個執行檔到處跑）
nvcc -std=c++20 -O3 \
     -gencode arch=compute_75,code=sm_75 \
     -gencode arch=compute_89,code=sm_89 \
     -gencode arch=compute_120,code=sm_120 \
     -gencode arch=compute_120,code=compute_120 \
     src/xxx.cu -o xxx

# 純 host 程式
g++ -std=c++20 -O2 src/xxx.cpp -o xxx
```

**確認產出的架構**：
```bash
cuobjdump --list-elf xxx      # 列出裡面有哪些架構的 cubin
```

---

## 六、踩雷備忘

**① `-ccbin` 不需要**（CUDA 13.3 + Ubuntu 26.04）

網路上很多資料說 26.04 要 `-ccbin g++-14`。**在 CUDA 13.3 上是錯的**：
- `crt/host_config.h` 的門檻是 `__GNUC__ > 15` 才報錯 → **支援 gcc ≤ 15**
- 26.04 預設就是 gcc-15，剛好在範圍內，本機實測直接編過

只有把 host gcc 切到 **>15**（26.04 庫內有 gcc-16）才需要：
```bash
nvcc -ccbin g++-15 ...              # 或設環境變數 NVCC_CCBIN
```

**② `cudaDeviceProp::clockRate` 在 CUDA 13.x 已移除**

舊教材都寫 `p.clockRate`，13.x 會編不過：
```
error: class "cudaDeviceProp" has no member "clockRate"
```
改用：
```cuda
int khz;
cudaDeviceGetAttribute(&khz, cudaDevAttrClockRate, dev);
```

**③ 編譯產物不要進 git**

`.gitignore` 已涵蓋 `*.o`／`*.exe`／`*.ptx`／`*.cubin`／`*.ncu-rep` 等。
測試時建議輸出到 `/tmp`：
```bash
tools/build.sh <檔案.cu> --native -o /tmp/test && /tmp/test
```

---

## 七、快速參考

| 我想… | 指令 |
|---|---|
| 編一支程式來跑 | `tools/build.sh <檔案.cu> --native -o /tmp/x && /tmp/x` |
| 確認全部範例都還能編 | `tools/check_build.sh` |
| 確認 5090 也編得過 | `tools/check_build.sh --arch 120` |
| 看看我改了哪些東西 | `tools/sync_lesson.sh` |
| 交付一堂課（含上傳 GitHub） | `tools/finish_lesson.sh --lesson "<課別>" -m "<訊息>"` |
| 任何工具的完整說明 | `tools/<工具>.sh --help` |
