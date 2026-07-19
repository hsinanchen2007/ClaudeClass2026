# ClaudeClass2026

Claude 與 Codex 各自獨立授課的學習筆記與範例程式碼合集。

| 課程 | 內容 |
|---|---|
| `Claude C++面向對象/` | 35 課，面向對象基礎到移動語意 |
| `Claude C++ STL課程/` | 40 課，STL 六大組件、vector/deque/list |
| `Claude C++ STL多線程課程/` | 5 階段，thread / mutex / 共享資料 |
| `Claude C++11 到 C++26 課程/` | 15 章，型別推導、初始化、移動語意、完美轉發 |
| `Claude Code C++by Examples/` | 依主題分類的範例（容器、多執行緒、智慧指標、模板、工具） |
| `Claude Python課程/` | 第 1~32 課，基礎語法與四大資料結構 |
| `Claude AI CUDA課程/` | 185 課 / 13 Parts，GPU 硬體到 AI 推論引擎系統（**進行中，見下方進度**） |
| `Codex AI CUDA課程/` | 185 課 / 13 Parts，Codex 獨立版本的 GPU/CUDA/AI 課程（**進行中，見下方進度**） |

## `Claude AI CUDA課程/` 進度

**2 / 185 課**　｜　大綱 `CUDA_AI_完整課程大綱_v2.0.6.md`（185 課 / 13 Parts）
下一課：**課 1.3 — NVIDIA GPU 架構演進史**

編譯方式見 [`Claude AI CUDA課程/BUILD.md`](Claude%20AI%20CUDA%E8%AA%B2%E7%A8%8B/BUILD.md)。
所有 CUDA 範例都已驗證可編到 **sm_75（本機 Quadro RTX 4000）＋ sm_89（4090）＋ sm_120（5090）**；
本機只有 sm_75 實體卡，其餘架構**僅保證編得過、執行結果未經驗證**。

### Part 1 — GPU 與 CUDA 硬體基礎

| 課號 | 課名 | 完成日 | 這堂課證明了什麼 |
|---|---|---|---|
| 1.1 | 第 01.01 課：為什麼需要 GPU | 2026-07-18 | 本機實測 GPU 峰值 **7.07 TFLOPS／384 GB/s／machine balance 18.4 FLOP/byte**；用 Amdahl(10×) vs Gustafson(36864×) 解釋 40960 條執行緒為何用得掉；踩到 CUDA 13.x 移除 `cudaDeviceProp::clockRate` 的真實 API 變更 |
| 1.2 | 第 01.02 課：GPU 硬體架構總覽 | 2026-07-19 | 用 `%smid` 實測 160 個 block 均勻落在全部 **40 個 SM**；暫存器壓力掃描實測 occupancy 兩個轉折點（128 regs → 50%、255 regs → spill 608 B），驗證 `65536 ÷ 1024 = 64 暫存器/thread` 的資源預算公式 |

## `Codex AI CUDA課程/` 進度

**2 / 185 課**　｜　大綱 `CUDA_AI_完整課程大綱_v2.0.6.md`（185 課 / 13 Parts）
下一課：**課 1.3 — NVIDIA GPU 架構演進史**

編譯方式見 [`Codex AI CUDA課程/BUILD_GUIDE.md`](Codex%20AI%20CUDA%E8%AA%B2%E7%A8%8B/BUILD_GUIDE.md)。
可攜 CUDA 範例都會編譯驗證 **sm_75（本機 Quadro RTX 4000）＋ sm_89（4090）＋ sm_90（Hopper）＋ sm_120（5090）**；
本機只有 sm_75 實體卡，其餘架構**僅保證編得過、執行結果未經驗證**。

### Part 1 — GPU 與 CUDA 硬體基礎

| 課號 | 課名 | 完成日 | 這堂課證明了什麼 |
|---|---|---|---|
| 1.1 | 第 01.01 課：為什麼需要 GPU | 2026-07-18 | 用 Amdahl/Gustafson 定量區分固定與擴張問題；本機 SAXPY 實測顯示大資料的 GPU kernel 最快約為 CPU 的 **12.74×**，但每次跨 PCIe 往返時端到端仍較 CPU 慢，證明 kernel throughput 不等於應用程式加速 |
| 1.2 | 第 01.02 課：GPU 硬體架構總覽 | 2026-07-19 | 用 Runtime、PTX `%laneid/%smid/%nsmid` 探針驗證本機 **40 SM、32-lane warp、384.064 GB/s 理論頻寬**；160 個 sample blocks 本次觀察涵蓋 40 個 SM、validation PASS，並明確區分一次觀察與 CUDA 規格保證 |

## 說明

- 本倉庫**只收原始碼與文件**（`.cpp` / `.py` / `.md` / `.pdf`）。
  建置產物（`.exe` / `.obj` / `.pdb` / `.ilk` 等）由 `.gitignore` 擋下，請勿提交。
- 註解與說明一律使用**繁體中文**。
- 2026-07-18 由舊倉庫重建為乾淨版（舊版 `.git` 因混入編譯產物膨脹至 1.9 GB）。
