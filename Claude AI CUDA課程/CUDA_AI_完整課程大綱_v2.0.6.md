# CUDA × AI 完整學習課程大綱
### 從硬體、環境、第一行程式碼，到函式庫、除錯、效能優化，再到 AI 推論引擎系統

> **版本**：v2.0.6（2026-07 修訂；課程數 **185**；Claude × Codex × Gemini × Grok 四家互審合併版，第十七輪 Codex／Grok 複審已納）。**v2.0–v2.0.6 變更摘要見文末〈附錄 F〉；完整逐版修訂紀錄（v1.1–v1.9.1）規劃隨教材 repo 建立 `CHANGELOG.md`，尚未建立——引用前請確認。**
> **設計對象**：資深 C/C++ / EDA 工程師，轉向 AI/ML、CUDA、推論引擎開發
> **主要實驗機（現況）**：Ubuntu 26.04 + NVIDIA Quadro RTX 4000（Turing, **sm_75**, 8GB）——目前手上的筆電，多數基礎課可實跑。
> **目標／遠端平台**：桌機 RTX 4090（Ada, sm_89）／RTX 5090（Blackwell, sm_120）；資料中心 sm_100/103 與 AMD MI300X 一律標「雲端 lab」。
> **基準工具鏈**：CUDA Toolkit 13.3 **Update 1**（13.3 GA 2026-05、U1 2026-06，日期以官方 release notes 為準；含 CUDA Tile C++、CUDA Python 1.0、CompileIQ、NVCC 支援 `-std=c++23`、CCCL 3.3；**對應／建議 R610 driver**——U1 對應的開發 driver 為 610.43.02，但 **Windows 自 CUDA 13.1 起 Toolkit 已不隨附 display driver、Linux 亦僅部分發行方式包含**；13.x minor-version compatibility 最低為 R580，本課完整 13.3／Tile／JIT 環境以 **R610 為驗證基準**）、Nsight Systems / Compute、**PyTorch 課程驗證版本 2.12（cu130）**——自 2.11 起 **cu130 為 PyPI 預設 stable、cu128 已於 2.12 自 binary matrix 移除**；sm_120 仍以 `get_arch_list()`＋實際 kernel 執行＋extension build 三項測試確認（詳見課 9.4）。**⚠️【v2.0】版本數字統一收斂於〈附錄 B.6 版本鎖定矩陣〉，正文各處版本敘述以該表為準。** **C++ 基準：專案可攜性基準採 C++20**；CUDA 13.3 起選定的 device-side C++23 功能可用，但需依 host compiler／NVCC／CCCL 支援矩陣逐項驗證（CCCL 正式測試 dialect 仍以 C++17/20 為主）。
> **硬體門檻慣例**：凡需 sm_80+／sm_89+／sm_90+／Blackwell 專屬特性的課，於課內與〈附錄 E〉標明「可實跑／有 fallback／只能閱讀或雲端」。**Triton（9.7 起的對照版）需 sm_80+**——筆電 sm_75 以 **CUDA C++ 為黃金實作路線**，Triton 只用 interpreter 讀語法。


---

## 如何使用這份大綱


這份大綱共 **13 個大主題（Part）**、**185 堂課（Lesson）**（v2.0 起；v1.9 為 169、v1.8 為 167）。每一堂課對應一個可以獨立教學的細部主題；每一個大主題對應一個完整的知識領域。

- **【v2.0.1】課程分級標記**：v2.0 新增的 16 堂課均標 **Core／Elective／Reference**，並在適用處標明 sm_75 可跑性／API 穩定度（逐課欄位於教材製作時補齊）；既有課分級以〈附錄 D〉〈附錄 E〉為準，逐課標籤於教材製作時補齊。**⚠️「Core」＝2026 推論系統的「核心知識地圖」，不等於「現在就要在筆電上實跑」**——不少 Core 課（9.31 CuTe DSL、6.20/6.21、9.33、9.35…）的 lab 需 sm_80+／多卡／雲端，筆電階段以「讀懂＋畫架構」交付；能否實跑一律以〈附錄 E〉硬體分層為準（**9.37/9.38 在附錄 E 標「視目標選讀」，即 Core-but-optional**）。
- **【v2.0】課程定位聲明**：本課程的 AI 面以 **LLM／多模態「推論系統」為主軸**（v2.0 新增 VLM、hybrid 架構、RL rollout、rack-scale serving 補齊廣度）；訓練側刻意僅到 9.28／9.38 的層級，需要完整訓練或資料科學課程請另行搭配。

- **想按部就班**：從 Part 1 一路往下，硬體 → 環境 → 第一個程式 → 模型 → 函式庫 → 除錯 → 優化 → AI 系統。
- **想跳著學**：**部分課程**開頭已標「**前置**」（建議先讀哪幾課），其餘在教材製作時逐課補齊；你可以直接挑想學的主題切入，補前置即可。
- **每堂課的結構**（之後實際開課時會長這樣）：
  1. **學什麼**：這堂課的目標，一兩句講清楚。
  2. **核心概念**：關鍵字 / 觀念清單。
  3. **動手做**：完整、有註解的程式碼範例（能跑、能改）。
  4. **圖解**：架構圖 / 流程圖 / 記憶體佈局圖。
  5. **延伸參考**：對應的書章、官方文件、影片、PDF。

> ⚠️ **關於時效**：CUDA 與 AI 工具鏈更新非常快。本大綱版本資訊以 **2026-07** 查到的為準（CUDA 13.3、RTX 5090 = compute capability 12.0 / sm_120）。實際開課時若版本有變，會以當下最新為準並標註差異。凡是我不確定或可能過時的（例如某個函式庫的最新 API），我會誠實標示，請你以官方文件交叉驗證。

---

## 課程地圖（依賴關係）

```
Part 1  硬體基礎 ─────────┐
                          ├──► Part 3  第一個程式 / 程式設計模型
Part 2  環境建置 ─────────┘            │
                                       ▼
                              Part 4  執行模型 & 記憶體階層（核心中的核心）
                                       │
                 ┌─────────────────────┼─────────────────────┐
                 ▼                     ▼                     ▼
        Part 5  平行模式        Part 6  函式庫         Part 7  除錯
                 │                     │                     │
                 └──────────┬──────────┴──────────┬──────────┘
                            ▼                     ▼
                   Part 8  效能優化  ◄────────────┘
                            │
                            ▼
                   Part 9  AI / 深度學習 CUDA 系統  ◄── 你的職涯主軸
                            │
                            ▼
                   Part 10 進階主題 & Capstone
                            │
              ┌─────────────┴─────────────┐
              ▼                            ▼
   Part 12 系統程式設計          Part 13 Production 系統
   (Driver/JIT Runtime)         & 資料管線
              └─────────────┬─────────────┘
                            ▼
                   Part 11 職涯、面試、持續學習
```

> **Part 12 / 13 是「系統與生產」雙翼**：Part 12（Driver API、NVRTC、IPC、CUPTI、NVML/DCGM）給你底層系統控制力；Part 13（資料管線、DALI、nvCOMP、MPS/MIG、K8s/Slurm、observability）給你把推論引擎真正「上線營運」的能力。兩者都直接對應 Staff/Principal 的 AI 基礎設施職務。

**最短路徑（若你只想盡快寫出第一個有用的 AI kernel）**：
Part 3 → Part 4 → 課 5.1~5.3 → 課 6.2(cuBLAS) → Part 8（重點 8.1~8.6）→ 課 9.6(FlashAttention，sm_75 讀為主) → 課 9.7(Triton，⚠️ **GPU lab 需 sm_80+；筆電只能讀語法，效能走 CUDA C++**)。
但因為你最終目標是 Staff/Principal 等級的推論引擎開發，**強烈建議完整走過 Part 4 與 Part 8**——這兩部分是把人和「會用 GPU」和「能設計 GPU 系統」區分開來的地方。

> **【v1.9】上面只講「順序」，沒講「要多久」。** 若你是有全職工作的工程師（例：每週約 7 小時），請看文末 **〈附錄 E：7 小時／週的時間軸學習計畫〉**——把 185 課切成三階段、標明哪些可以先跳、哪些筆電跑不了。

---
---

# Part 1 — GPU 與 CUDA 硬體基礎

> 目標：在寫任何程式前，先建立對 GPU **硬體**的正確心智模型。你的 EDA 背景在這裡是巨大優勢——你本來就懂時序、記憶體階層、晶片架構。我們把這些直覺對應到 GPU。

### 課 1.1 — 為什麼需要 GPU：平行運算的本質
**學什麼**：CPU 與 GPU 的設計哲學差異，為何 AI 需要 GPU。
**核心概念**：latency-oriented (CPU) vs throughput-oriented (GPU)、Dennard scaling 終結、暗矽 (dark silicon)、Amdahl 定律 vs Gustafson 定律、SIMD/SIMT、heterogeneous computing。
**圖解**：CPU vs GPU 晶片面積分配圖（控制邏輯/cache vs ALU）。
**延伸**：PMPP 第 5 版 Ch.1；NVIDIA CUDA C++ Programming Guide §1。

### 課 1.2 — GPU 硬體架構總覽
**學什麼**：一顆現代 NVIDIA GPU 由什麼組成。
**核心概念**：Streaming Multiprocessor (SM)、CUDA Core、Tensor Core、RT Core、warp scheduler、register file、L1/Shared、L2 cache、記憶體控制器、GPC/TPC 階層。
**圖解**：GPU → GPC → TPC → SM → CUDA Core 的階層樹。
**延伸**：Blackwell 架構白皮書（見附錄 A）；PMPP Ch.4。

### 課 1.3 — NVIDIA GPU 架構演進史
**學什麼**：從 Tesla 到 Blackwell，每代架構解決了什麼問題，為什麼這對寫程式有影響。
**核心概念**：Tesla → Fermi → Kepler → Maxwell → Pascal → Volta(首個 Tensor Core) → Turing → Ampere → Ada Lovelace → Hopper(TMA/WGMMA) → Blackwell(FP4) → **Rubin（【v2.0 更新】2026-05-31 NVIDIA 宣布進入全面量產爬坡（ramping into full production）、合作夥伴產品預計 H2 2026 上市：288 GB HBM4、~22 TB/s；「R100」為媒體慣稱，官方僅稱 Rubin GPU）**。每代的 compute capability、記憶體技術、Tensor Core 世代。**【v2.0】路線圖 case study：Rubin CPX**——2025-09 官方發表的 prefill 專用 GPU（GDDR7）；**GTC 2026 的新平台資料未再列出 CPX、後續定位待官方確認（「已移除」為媒體推論，非官方聲明）**；「專用化 vs 通用性」的取捨論述（接 9.33 disaggregated serving 脈絡），面試層級的好題目。
**圖解**：架構時間軸 + 關鍵特性對照表。
**延伸**：各代架構白皮書；維基「CUDA」條目的 compute capability 對照表。

### 課 1.4 — Compute Capability、PTX/SASS 與編譯 target
**學什麼**：sm_xx 是什麼、Blackwell 其實有多個 CC、以及 `a`/`f` 編譯 target 的差別。
**核心概念**：compute capability (CC)、virtual architecture (`compute_xx`/PTX) vs real architecture (`sm_xx`/SASS)、JIT 編譯、向後/向前相容、fatbin。
**⚠️ 重點一：Blackwell 不是單一 CC，別看到 Blackwell 就假設 sm_120**——
- GeForce RTX 50 / RTX PRO Blackwell（workstation/consumer）= **CC 12.0 / sm_120**（你的**目標卡** 5090／未來桌機在這；現況筆電是 sm_75）
- GB10 / DGX Spark = **CC 12.1 / sm_121**
- B200 / GB200（資料中心）= **CC 10.0 / sm_100**
- B300 / GB300（Blackwell Ultra）= **CC 10.3 / sm_103**
- RTX 4090 / Ada = **CC 8.9 / sm_89**；Hopper H100/H200 = 9.0
編譯/tuning 必須依**實際 GPU** 給正確的 `-arch`/`-gencode`。
**⚠️ 重點二：CUDA 13.x / Blackwell 有三種編譯 target**——
- **baseline**（如 `compute_120`/`sm_120`）：一般相容性。
- **architecture-specific `a`**（如 `sm_90a`、`sm_100a`、`sm_120a`）：只在該確切架構跑、無前向相容，但解鎖架構專屬加速。**注意各架構解鎖的是不同東西**：`sm_90a`→Hopper WGMMA；`sm_100a` / `sm_103a`（datacenter Blackwell，**各自架構專屬、互不前向相容**）→ tcgen05/TMEM；`sm_120a`（必要時另有 `sm_121a`）→RTX Blackwell 的架構專屬功能——**⚠️【v2.0 精確化】PTX ISA 8.8 起，dense NVFP4/MXFP4 block-scaled `mma.sync` 已可用 family target `sm_120f` 取得，只有 sparse 版等少數功能才需 `sm_120a`，別把 FP4/block-scaled 一律綁 `a`**（詳見課 8.17）。另：WGMMA 已標 **deprecated**——**【v2.0.5 精確化】分兩種 Blackwell 講**：資料中心 10.x（sm_100/103）由 **tcgen05 接手**；消費級 sm_120 **沒有 TMEM/tcgen05**，是「WGMMA 不可用 → 改走擴充 `mma.sync`／CUTLASS／CUDA Tile」，別寫成「被 tcgen05 取代」。
- **family-specific `f`**（如 `compute_120f`、`compute_100f`，CUDA 12.9 起）：在**同一 family 內**前向相容。
**兩個 Blackwell family 不互通**：12.x family = `compute_120f`（sm_120 RTX 50 + sm_121 GB10）；10.x family = `compute_100f`（sm_100 B200 + sm_103 B300）。`sm_100f` 的 binary **不能**在 sm_120 上跑、反之亦然。
**重點三**：很多 2026 初的預編譯套件只編到 sm_90 (Hopper)，遇到 sm_120 會 fallback 或報 `no kernel image is available`。
**圖解**：原始碼 → PTX → SASS 的編譯/JIT 流程圖；Blackwell CC family（10.x vs 12.x）對照。
**延伸**：CUDA Compiler Driver NVCC 文件；CUDA GPU Compute Capability 官方表；附錄 A。

### 課 1.5 — Blackwell 架構深入剖析（目標卡專題，以 RTX 5090 為例）
**學什麼**：這張**目標卡**（RTX 5090，未來桌機）的內部結構與數字代表什麼。
**核心概念**：GB202 die、NVIDIA 客製 TSMC 4nm 製程（旁註：GB202 的 4N/4NP 細分在第三方資料中有分歧、官方未明文——對 CUDA 課程無實質影響，不必糾結）、92.2B 電晶體、170 SM（每 SM = 128 CUDA core + 4 個第 5 代 Tensor Core + 1 個第 4 代 RT Core）、21,760 CUDA core、680 Tensor Core、**96 MB L2（= 98,304 KiB，故有些表格寫成 ~98 MB；NVIDIA 官方文字為 96 MB；完整 GB202 die 為 128 MB）**、32 GB GDDR7、512-bit、1,792 GB/s 頻寬、PCIe Gen5、**575 W TGP（Total Graphics Power，非 TDP）**、第 5 代 Tensor Core 原生 FP4。對照 RTX 4090（Ada、sm_89、16,384 core、24GB GDDR6X、~1,008 GB/s）。
**圖解**：RTX 5090 SM 內部方塊圖；5090 vs 4090 規格對照表。
**延伸**：NVIDIA RTX Blackwell GPU Architecture 白皮書（附錄 A）。

### 課 1.6 — GPU 記憶體系統硬體
**學什麼**：頻寬與延遲從哪來，為什麼「記憶體」幾乎決定一切。
**核心概念**：GDDR7 vs HBM、記憶體頻寬計算、burst、memory bank、coalescing 的硬體根源、L2 cache 行為、暫存器檔案大小、各層記憶體的延遲/頻寬數量級（**【v2.0】數量級簡化**：register 是延遲最低的 on-chip 儲存，但實際 latency/throughput 受依賴鏈、bank、指令排程影響、並非固定 1 cycle；shared ≈ 數十 cycle；global ≈ 數百 cycle）。
**圖解**：記憶體階層金字塔（容量 vs 頻寬 vs 延遲）。
**延伸**：PMPP Ch.5；Best Practices Guide「Memory Optimizations」。

### 課 1.7 — GPU 選購與多卡配置
**學什麼**：怎麼為「AI 開發 + 推論」選硬體。
**核心概念**：VRAM 容量 vs 模型大小對應（8B FP16≈16GB、13B FP16≈26GB、32B Q4≈20GB——**注意這些只是 model weights 的下限**，實際推論還要加 KV cache、activation/workspace、framework allocator 碎片、CUDA graph capture buffer、batch/併發 overhead，後面 Part 9 會細談）、5090 vs 4090 vs 資料中心卡（H100/B200）的取捨、消費卡無 NVLink 的限制、PCIe lane、雙卡散熱與電源、blower vs open-air。
**圖解**：模型大小 vs 所需 VRAM 對照表。
**延伸**：附錄 A 的 RTX 5090 LLM 指南。

### 課 1.8 — AI 工作站整機規劃（對應你的 Ubuntu workstation）
**學什麼**：把 GPU 放進一台能長時間跑訓練/推論的機器。
**核心概念**：CPU（PCIe lane 與核心數）、系統 RAM（建議 ≥ VRAM×2）、NVMe（資料集/權重 I/O，主碟 2TB + 次碟）、電源瓦數計算與餘裕、機殼風道、噪音/散熱、UPS。把你已決定的 5090/4090 + Ubuntu + NVMe 配置做一次完整 sanity check。
**圖解**：整機配置 checklist。
**延伸**：你之前做過的硬體研究結論可直接套用。

### 課 1.9 — 資料中心 Blackwell vs 消費級 Blackwell（為什麼你的卡跟 B200 不一樣）
**學什麼**：同叫 Blackwell，但 GB202（**目標卡** 5090）與 GB100/GB200（資料中心）差很多——面試與選型都要懂。
**核心概念**：消費級 GB202（GeForce/RTX，**CC 12.0 / sm_120**）vs 資料中心 GB100/GB200（HGX/DGX，**CC 10.0 / sm_100**）、B300/GB300（Blackwell Ultra，**CC 10.3 / sm_103**）的差異：FP64 算力（消費卡刻意閹割）、HBM3e vs GDDR7、NVLink/NVSwitch（資料中心有、消費卡無或受限）、MIG 支援（**【v2.0 更正】限官方支援 SKU：資料中心卡＋部分 RTX PRO Blackwell 工作站卡；GeForce 不支援**）、ECC、多 die（**【v2.0 更正】GB200 Superchip＝2 顆 B200 GPU＋1 顆 Grace CPU；每顆 B200 自身由 2 個 reticle-size die 以 10 TB/s C2C 組成——兩個層級別混為一談**）、cluster 行為（B200 可開非可攜 cluster size 16）、shared memory（**B200 / CC 10.0：228 KB per SM、單一 thread block 最多 227 KB**；**RTX 50 / CC 12.0：128 KB per SM、單一 block 最多 99 KB**——數字依 CUDA 13.x Blackwell Tuning Guide）、功耗與散熱規模；**【v2.0】補充**：GB200 的 L2 為 **126 MB**（≠完整 GB202 的 128 MB）；GB10/DGX Spark（sm_121）為 48 SM／128 GB LPDDR5x 統一記憶體 @273 GB/s——同屬 12.x family 但記憶體架構與 RTX 50 完全不同；**10.x 與 12.x 是不互通的兩個 family**（見課 1.4）；哪些課的技術（如 MIG、NVSwitch、Fabric Manager）你在本機跑不了、只能概念理解（會在 Part 13 標明）。
**圖解**：消費級 vs 資料中心 Blackwell 對照表。
**備註**：你目標公司多在資料中心場景，這課幫你把「本機能練的」與「面試要懂的」分清楚。

**📚 Part 1 參考**：PMPP 5e Ch.1,4,5｜NVIDIA Blackwell 架構白皮書（消費級 RTX 與資料中心版）｜CUDA C++ Programming Guide 前幾章｜NVIDIA CUDA GPU Compute Capability 列表。

---
---

# Part 2 — 開發環境建置（Ubuntu Linux）

> 目標：在 Ubuntu 上建立一個**乾淨、可重現、不會打架**的 CUDA 開發環境。這一段「踩雷」最多，我們把雷一次掃完。

### 課 2.1 — Ubuntu 安裝與系統準備
**前置**：—
**學什麼**：為 CUDA 準備一個健康的 Ubuntu。
**核心概念**：Ubuntu LTS（**本機為 26.04**；示範也相容 24.04）、kernel 版本、Secure Boot（會擋未簽署的 NVIDIA 模組，要處理）、`build-essential`、headers、停用 `nouveau` 開源驅動。
**動手做**：系統更新、安裝編譯工具鏈、停用 nouveau 的設定檔。
**延伸**：CUDA Installation Guide for Linux（附錄 A）。

### 課 2.2 — NVIDIA 驅動程式安裝
**學什麼**：正確裝驅動，理解「驅動版本」與「CUDA runtime 版本」的關係。
**核心概念**：proprietary vs **open kernel module（本課程／本機黃金決策：走 NVIDIA 官方 repo 的 `nvidia-open`，13.3 正式環境用 R610+）**、`ubuntu-drivers`（方便但易裝到過舊或 proprietary 版；Blackwell／13.x 情境**建議改用官方 repo + nvidia-open**）、driver/CUDA 相容矩陣、`nvidia-smi` 顯示的「CUDA Version」是**驅動支援的最高 runtime**，不等於你裝的 toolkit 版本。
**動手做**：安裝驅動、重開機、`nvidia-smi` 驗證看到**本機 GPU**（目前筆電是 Quadro RTX 4000／sm_75；未來桌機才是 5090／4090）。
**踩雷提醒**：Secure Boot 簽署、與舊驅動殘留的清除；本機黃金決策走 **NVIDIA 官方 repo + `nvidia-open`**（見 2.2 核心概念）。

### 課 2.3 — CUDA Toolkit 安裝（13.x）與驅動版本
**學什麼**：三種安裝法的取捨、Blackwell 的版本下限、以及驅動版本要配多新。
**核心概念**：runfile vs `apt`(NVIDIA repo) vs `conda`/`pip`(nvidia channel)；**重大踩雷**：Ubuntu 內建的 `nvidia-cuda-toolkit` apt 套件版本**遠舊於官方 13.x**（實際版本以 `apt-cache policy nvidia-cuda-toolkit` 當下為準，隨發行版而異），不足以支援 Blackwell／CUDA Tile——千萬別用它。Blackwell 需要 **CUDA ≥ 12.8**，建議直接上 **13.x**。**⚠️ 驅動版本**：CUDA 13.x 的「minor-version compatibility 最低 driver 需求」與「CUDA 13.3 **對應的 development driver**」**不完全等同**（【定稿修正】不再說「隨附」——**Windows 自 CUDA 13.1 起 Toolkit 不再附 display driver、需另外安裝；Linux 是否包含 driver 依安裝方式而定**）；若要把 CUDA 13.3 + Blackwell + CUDA Tile C++ / JIT / PyTorch cu130 當正式環境，**建議直接用 R610+ driver**，實際以官方 release notes / compatibility matrix 為準，避免「toolkit 裝得起來，但 JIT / Tile / 某些 wheel / Blackwell 行為不穩」。
**動手做**：用 NVIDIA 官方 repo 安裝 CUDA 13.3（**建議 Update 1；U1 對應的 Linux development driver 為 610.43.02**——driver 需按課 2.2 另行安裝確認，別假設裝 toolkit 就有）、確認 driver ≥ R610、把環境變數寫進 **`/etc/profile.d/cuda.sh`**（系統級、一處生效），**不要在 `~/.bashrc` 疊多套 CUDA 的 `PATH`/`LD_LIBRARY_PATH`**（多 toolkit 打架的常見根因；要多版本共存見 2.8）。
**【v2.0.4】host compiler 相容性（26.04 實務，本機實測）**：CUDA 13.3 的 nvcc 支援 **gcc ≤ 15**（`crt/host_config.h` 的門檻是 `__GNUC__ > 15` 才報 `unsupported GNU version`）。**26.04 預設就是 gcc-15，剛好落在支援範圍、不需 `-ccbin`**（本機實測 `nvcc -arch=sm_75` 直接編過並執行）。⚠️ 但 26.04 套件庫也有 **gcc-16**，若把預設 host gcc 切到 >15，nvcc 會擋 → 逃生口：`sudo apt install g++-15` 後加 `-ccbin g++-15`、或設環境變數 `NVCC_CCBIN`、或 `-allow-unsupported-compiler`（風險自負）。**別**沿用舊教材「CUDA 13.1 必須 pin gcc-14」的說法——那在 13.3 已不成立（見附錄 B.6「Host GCC」列）。
**延伸**：CUDA Toolkit Downloads / Archive、CUDA 13.x Release Notes（附錄 A）。

### 課 2.4 — 驗證安裝與第一次體檢（含環境驗證 checklist）
**學什麼**：確認整套裝對了——並建立一份可在每台機器重跑的驗證清單（你有多台機器：現在的筆電 + 未來桌機，這很實用）。
**核心概念**：`nvcc --version`（toolkit 版本）、`nvidia-smi`（driver 版本與 GPU；**注意右上角的「CUDA Version」是 driver 支援的最高 runtime，不是已裝的 toolkit 版本**）、`deviceQuery`、`bandwidthTest`（CUDA samples）、看懂 deviceQuery 每一欄（SM 數、CC、記憶體、warp size、每 block 最大 thread）。**環境驗證 checklist（拆成多個獨立層，避免假通過）**：① **driver**：`nvidia-smi` 看到卡且 driver ≥ R610（for 13.3）② **toolkit**：`nvcc --version` 顯示 13.3；**JIT 靜態庫要用 `find` 找檔、不要 `ldconfig`**——`find "$CUDA_HOME" -name 'libnvptxcompiler_static.a'`（⚠️ **Linux Toolkit 的 PTX compiler 是靜態庫 `libnvptxcompiler_static.a`，不會出現在 `ldconfig -p`**；舊版寫 `ldconfig | grep -E "libcuda|libnvptxcompiler"` 會因為只配到 `libcuda.so` 就**假通過**）③ **SASS 執行**：編譯一個 vector add 並跑，**確認沒有 `no kernel image is available`**，依**本機實際 CC** 給 target（筆電 `-arch=sm_75`；桌機再加 `sm_89`／`sm_120`，或 `-arch=native`）——⚠️ **注意：`-arch=sm_75` 直接跑的是已產生的 SASS，不等於證明 PTX driver JIT 正常**。④ **PTX driver JIT**：載入一個**只含 PTX**的 module（或編 `compute_75` 讓 driver 在載入時 JIT 成 cubin），驗證 driver 端 JIT 能跑。⑤ **NVRTC / nvPTXCompiler**：跑一次執行期編譯（見 12.3/12.4）。⑥ **PyTorch extension**：`cpp_extension.load_inline` JIT + AOT 各一次（見 9.3/9.4）——⚠️ **【v2.0】分層要精確：cpp_extension 的「JIT」＝ Python→Ninja→host compiler＋nvcc（ptxas）的建置流程，與 ④ 的 driver PTX JIT、⑤ 的 NVRTC／nvPTXCompiler 程式化 API 是完全不同的路徑**——四層各自獨立測，別混為一談；④⑤⑥ 合起來才算把各層煙霧測試跑完，別用 ③ 的 SASS 執行冒充。
**動手做**：把上面 checklist 寫成一支環境驗證腳本，在你每台機器上跑一次；記錄**本機 GPU 的實測頻寬**（現在是 sm_75 筆電；桌機到位後再記 4090/5090）。
**延伸**：`github.com/NVIDIA/cuda-samples`。

### 課 2.5 — 編輯器 / IDE 設定（VS Code）
**學什麼**：把開發體驗弄順（你已有跨多機 VS Code 工作流，這裡接上 CUDA）。
**核心概念**：VS Code + Nsight Visual Studio Code Edition、CUDA syntax highlight、`clangd`/IntelliSense、`.clangd` 設定讓它認得 CUDA、`compile_commands.json`。
**動手做**：設定一個能跳轉/補全 CUDA API 的 VS Code 專案。
**備註**：可用 Claude Code 幫你生成/維護 build 設定與範例骨架。

### 課 2.6 — 建置系統：nvcc、Makefile、CMake
**學什麼**：從命令列到工程化建置。
**核心概念**：`nvcc` 常用旗標（`-arch`/`-gencode`/`-O`/`-G`/`-lineinfo`/`--use_fast_math`）、`-gencode` 同時打包多架構（fatbin）、CMake 的 `CUDA` 語言支援與 `CMAKE_CUDA_ARCHITECTURES`、`find_package(CUDAToolkit)` 連結 cuBLAS 等；**【v1.9】雙機建置實務**——設 `CMAKE_CUDA_ARCHITECTURES "75;89;120"`（筆電 Quadro RTX 4000 sm_75 ＋桌機 4090 sm_89／5090 sm_120）產生單一 fatbin，或用 `native` 讓 CMake 偵測當下這台；用 `75-real;89-real;120-real;120-virtual` 精確控制 SASS/PTX 產出；搭配 CMake presets 讓同一份專案在兩台機器都能一鍵建置（對照課程 repo 的 `tools/build.sh`——**【v2.0.5 更正】該檔已存在**：`AI_CUDA/tools/build.sh`，支援三架構 fatbin 與 `--native`；**其 `-std=` 已於 2026-07-18 對齊本大綱的 C++20 基準**）。**【v2.0.4】host compiler**：若 host 預設 gcc >15（13.3 上限）需指定 nvcc 用哪個 host 編譯器，CMake 設 `CMAKE_CUDA_HOST_COMPILER=/usr/bin/g++-15`（等同 nvcc 的 `-ccbin`）；26.04 預設 gcc-15 則免設，詳見 2.3。
**動手做**：用 CMake 建一個可同時編 **sm_75 + sm_89 + sm_120 + PTX** 的專案，在筆電與（未來）桌機各跑一次，驗證同一個 binary 兩台都能執行。
**延伸**：CMake CUDA 文件；Best Practices Guide。

### 課 2.7 — 容器化開發（Docker + NVIDIA Container Toolkit）
**學什麼**：讓環境可重現、可搬移（呼應你的多機與雲端需求）。
**核心概念**：NVIDIA Container Toolkit、`--gpus all`、NGC（NVIDIA GPU Cloud）官方映像（CUDA、PyTorch、TensorRT）、版本鎖定。
**動手做**：跑一個 NGC CUDA 映像並在容器內 `nvidia-smi`、編譯 kernel。
**延伸**：NVIDIA Container Toolkit 文件。

### 課 2.8 — cuDNN 與函式庫安裝、多版本共存
**學什麼**：把 AI 需要的函式庫裝好且不互相打架。
**核心概念**：cuDNN 安裝（與 CUDA 版本對應）、`update-alternatives`、用 conda/venv 隔離不同專案的 CUDA/PyTorch 版本、環境變數管理策略。
**動手做**：建立兩個互不干擾的環境（例如一個給 CUDA C++ 開發，一個給 PyTorch）。

### 課 2.9 —（進階）WSL2 / 雲端 / 遠端開發的取捨
**學什麼**：何時用本機、何時用雲。
**核心概念**：你的 Ubuntu workstation 為何最適合做 CUDA profiling（裸機、可鎖時脈、Nsight 完整）；WSL2 的 GPU passthrough 注意事項（驅動裝在 Windows 端、容易「看起來能跑其實沒用到 Blackwell」）；雲端 GPU（RunPod/Vast 等）適合臨時擴充。
**延伸**：附錄 A 的 WSL2 sm_120 setup 指南。

**📚 Part 2 參考**：CUDA Installation Guide for Linux｜CUDA Toolkit Archive｜NVIDIA Container Toolkit docs｜cuDNN docs。

---
---

# Part 3 — 第一個 CUDA 程式與程式設計模型

> 目標：寫出、編譯、跑起來你的第一批 kernel，並徹底搞懂 CUDA 的程式設計模型。對你來說語法不是問題，**心智模型**才是重點。

### 課 3.0 — 從 CPU C++ 到 GPU device code 的思維轉換【v1.9 新增】
**前置**：Part 1–2（至少 1.1–1.2）。**這是 Part 3 的第一課，建議不要跳。**
**學什麼**：你有 20+ 年的 C++ 直覺——這課講**哪些直覺可以帶上 GPU、哪些會在 GPU 上害你**。目的是在寫第一支 kernel 之前先把腦袋切換完成，而不是 debug 三天後才發現。
**核心概念**：
- **可以保留的資產**：RAII（包裝 device 記憶體）、template 與 `constexpr`、值語意、`__host__ __device__` 通用程式碼、對 cache／記憶體階層的敏感度（老手的最大優勢）。
- **必須重學（多數是「能用但成本高／有限制」，不是語言禁止——這區別在面試會被追問）**：**例外**（device 端**真的不支援** → 錯誤處理回到 error code）；**虛擬函式／RTTI／函式指標**（device 端**有限度支援**；【v2.0 精確化】vtable／間接跳轉**可能**造成 divergence、指令快取壓力與最佳化受限——是否昂貴要量測，熱路徑預設避免、但別當「絕對災難」背誦）；**遞迴**（stack 有限，深遞迴會爆）；**device 端動態配置 `new`/`malloc`**（**允許但極貴、heap 小、易碎片** → 儘量在 host 配置好）；iostream／多數 STL 容器（device 端不可用；【v2.0 精確化】**libcu++ 提供的是 atomic/barrier/mdspan 等 utility，並沒有一般用途的 device-side `std::vector`/`std::string` 替代品**——容器需求靠 Thrust（host 側容器＋演算法）或自行管理的 device storage）。
- **心智模型的四個切換**：一條 thread 做完一件事 → **32 條一束（warp）一起做**；分支很便宜 → **divergence 讓兩條路都跑**；隨手 malloc → **kernel 裡只算、不配置**；單執行緒正確就對了 → **race／同步是預設風險**。
- **常見 migration 錯誤目錄**（照著避雷）：host/device 指標混用（把 device 指標丟給 `memcpy`、在 host 解參考 device 指標）；忘記同步就讀結果；以為 kernel launch 失敗會丟例外（實際是**非同步**且要自己查 error code）；kernel 裡 `printf` 沒同步就看不到；拿 `volatile` 當同步手段；以為 `__syncthreads()` 能跨 block；struct 對齊／padding 在 host/device 不一致。
**動手做**：拿一段「很 C++」的 host 程式碼（含虛擬函式、例外、`std::vector`、遞迴），逐項改寫成可在 device 執行的版本，並產出「每項為什麼要改、改成什麼」對照表——這張表就是你往後的 checklist。
**延伸**：CUDA C++ Programming Guide「C++ Language Support」；libcu++／CCCL 文件。
**備註**：本課只建立**轉換心法與避雷**；深入的現代 C++ 整合（RAII 工具類、C++20 Concepts 約束 kernel template）在 **10.6**、`mdspan` 型別安全索引在 **10.7**、記憶體模型與 scoped atomics 在 **4.10**。

### 課 3.1 — Hello GPU：第一個 kernel
**前置**：Part 2 裝好環境。
**學什麼**：kernel 長什麼樣、怎麼啟動、怎麼編譯執行。
**核心概念**：`__global__`、kernel launch 語法 `<<<grid, block>>>`、host 與 device 的分界、`cudaDeviceSynchronize()`、為何 GPU 上的 `printf` 是非同步的。
**動手做**：`hello_gpu.cu`——每個 thread 印出自己的 id；觀察輸出順序。

### 課 3.2 — 執行緒階層（Thread Hierarchy）
**學什麼**：thread / block / grid 怎麼組織與定位。
**核心概念**：`threadIdx`、`blockIdx`、`blockDim`、`gridDim`、全域索引公式 `idx = blockIdx.x * blockDim.x + threadIdx.x`、1D/2D/3D 配置、為何 block 大小常用 128/256。
**圖解**：grid → block → thread 的方格圖 + 索引計算動畫式說明。

### 課 3.3 — 主機與裝置記憶體模型
**學什麼**：資料怎麼在 CPU 與 GPU 之間搬。
**核心概念**：`cudaMalloc` / `cudaMemcpy`(H2D/D2H/D2D) / `cudaFree`、為何要 `h_`/`d_` 命名慣例、PCIe 傳輸是瓶頸的直覺。
**動手做**：手動搬一筆陣列到 GPU、處理、再搬回來。

### 課 3.4 — 完整範例：向量加法（工程化寫法）
**學什麼**：把前三課組成一個「正確、健壯」的程式。
**核心概念**：grid-stride loop（讓任意大小資料都能跑）、邊界檢查 `if (i < n)`、完整錯誤檢查、計時、與 CPU 結果比對驗證。
**動手做**：`vector_add.cu`——含 CPU reference、容差比對、計時，這是之後所有範例的模板。

### 課 3.5 — 函式限定詞與 kernel 限制
**學什麼**：`__global__` / `__device__` / `__host__` / `__host__ __device__` 的差別與規則。
**核心概念**：哪些能在 device 上呼叫、遞迴/函式指標/動態配置的限制、`__forceinline__`、device 端可用與不可用的 C++ 特性。

### 課 3.6 — 多維資料與索引（影像處理範例）
**學什麼**：處理 2D/3D 資料（矩陣、影像、張量）。
**核心概念**：2D grid/block、row-major 線性化 `row*width+col`、pitch / `cudaMallocPitch`、邊界處理。
**動手做**：影像灰階化或模糊（每個 pixel 一個 thread）。

### 課 3.7 — 錯誤處理基礎
**學什麼**：CUDA 的錯誤怎麼抓（這裡先建立習慣，Part 7 再深入）。
**核心概念**：`cudaGetLastError` vs `cudaPeekAtLastError`、同步 vs 非同步錯誤、`cudaError_t`、自製 `CUDA_CHECK(call)` 巨集。
**動手做**：把向量加法包上錯誤檢查巨集，故意製造錯誤觀察行為。

### 課 3.8 — 統一記憶體（Unified Memory）
**學什麼**：用 `cudaMallocManaged` 簡化記憶體管理，以及它的代價。
**核心概念**：unified memory、page fault / migration、`cudaMemPrefetchAsync`、`cudaMemAdvise`、何時方便、何時會傷效能；**【v2.0】進階層**：HMM（Heterogeneous Memory Management——開啟後連 `malloc` 的 system-allocated memory 都可被 GPU 直接存取）、ATS（Grace 平台的位址轉換服務）、CPU/GPU coherency 模型、first-touch 放置——單機 PCIe 平台與 GH200/GB200（NVLink-C2C）的行為差異（接 10.4、13.13）——**⚠️【v2.0.2】別把兩者混為一談**：x86＋PCIe 消費卡上 UM 的 fault/migration **在熱路徑上可能很貴**（隨存取模式而定，需 prefetch/advise 才不致拖垮）；GB200 的 CPU↔GPU 是 NVLink-C2C（~900 GB/s）＋硬體 ATS coherent，UM 好用很多，**但仍非「免費零拷貝」——TLB、資料放置與存取局部性都可能明顯降速，一樣要 profiling＋memory advice**。重點是同一份 UM 程式碼在兩種物理架構上的效能落差很大、不能一體看待。
**動手做**：把向量加法改成 unified memory 版，profiling 比較。

### 課 3.9 — 編譯流程深入（NVCC 內部）
**學什麼**：`.cu` 到底經歷了什麼。
**核心概念**：host/device 程式碼分離、PTX 中間碼、SASS 機器碼、fatbin、JIT、`-arch` vs `-code`、separable compilation 與 device linking (`-dc`/`-dlink`)、**【v2.0】Device LTO**（`-dlto` 產 LTO-IR、跨 translation unit 的 device 端最佳化、fatbin 同時打包 LTO-IR 的策略、與 12.4 `nvJitLink` runtime LTO 的對應）、`cuobjdump`/`nvdisasm` 看反組譯。
**動手做**：對一個 kernel `cuobjdump --dump-sass` 看實際指令。
**備註**：你做 EDA 對「看底層」不陌生，這課會很對胃口。

**📚 Part 3 參考**：PMPP 5e Ch.2,3｜CUDA C++ Programming Guide §2-5｜GPU MODE Lecture 1-3｜NVIDIA DLI「Fundamentals of Accelerated Computing with CUDA C/C++」。

---
---

# Part 4 — GPU 執行模型與記憶體階層深入

> 目標：這是**整個課程的核心**。理解 warp、divergence、記憶體存取模式、shared memory、occupancy——之後所有的優化都建立在這。

### 課 4.1 — Warp 與 SIMT 執行模型
**前置**：Part 3。
**學什麼**：GPU 的**主要排程／發射（issue）單位是 warp**；thread 仍是程式語意單位——這個分工（而非「執行單位不是 thread」的絕對化說法）才是理解 SIMT 的關鍵。
**核心概念**：32-thread warp、SIMT、warp scheduler、lockstep（及 Volta 後的 independent thread scheduling）、latency hiding（用大量 warp 掩蓋記憶體延遲）、active/eligible/selected warp。
**圖解**：warp 在 SM 上排程、用切換 warp 掩蓋延遲的時間軸。

### 課 4.2 — 控制流分歧（Warp Divergence）
**學什麼**：`if/else` 在 GPU 上為何可能很貴。
**核心概念**：branch divergence、執行序列化、predication、如何重組資料/分支避免 divergence、`__ballot`/mask。
**動手做**：寫一個有 divergence 的 kernel，重構成無 divergence，比較。

### 課 4.3 — 記憶體階層全貌
**學什麼**：六種記憶體空間各自的用途。
**核心概念**：register / shared / local / global / constant / texture(surface)、各自的 scope、生命週期、cache 行為、延遲/頻寬。
**圖解**：一張完整的 CUDA 記憶體空間對照表（誰可見、誰快、誰大）。

### 課 4.4 — 全域記憶體合併存取（Coalescing）
**學什麼**：讓 warp 的 32 個存取合併成最少的記憶體交易。
**核心概念**：coalesced vs uncoalesced、cache line / sector、對齊 (alignment)、stride 的影響、AoS vs SoA、向量化載入 (`float4`)。
**動手做**：同一運算的 coalesced 與 strided 版本，用 Nsight 看記憶體效率差異。

### 課 4.5 — 共享記憶體（Shared Memory）與 Tiling
**學什麼**：用 on-chip 記憶體做 data reuse，這是優化的第一大武器。
**核心概念**：`__shared__`、static vs dynamic shared memory、bank（32 banks）、bank conflict、padding 消除 conflict、tiling 的通用模式、shared memory 與 L1 的容量取捨 (`cudaFuncSetAttribute`)。
**動手做**：用 shared memory tiling 加速矩陣乘法（為 Part 5 鋪路）。

### 課 4.6 — 常數、紋理與 Surface 記憶體【v2.0 改版｜Reference】
**學什麼**：三種特殊記憶體路徑各自的定位——並修正舊教材的兩個常見誤解。
**核心概念**：**constant memory**（`__constant__`、唯讀、warp broadcast 最快）；**texture**（唯讀「取樣」路徑：空間區域性 cache、硬體內插/過濾、address mode 邊界處理；⚠️ texture reference API 已棄用，**一律用 texture object**）；**surface**（⚠️ **【v2.0 更正】可讀也可寫**——CUDA array 的讀寫介面，不是唯讀）；`__ldg` / read-only data cache（**歷史/架構相關技巧**：現代架構上編譯器多能自動推導，不應再當成通用優化建議）；**誠實界線**：現代 GPU 上 texture/surface 對一般非圖形計算**不再保證**傳統教材宣稱的效能優勢——以 profiling 為準，AI 主線用途有限。
**動手做**：用 constant memory 放卷積核；用 texture object 做有邊界處理的取樣；用 surface 寫回一張影像。

### 課 4.7 — 暫存器與本地記憶體
**學什麼**：register 是最快的資源，也是最稀缺的。
**核心概念**：register file 容量、每 thread register 用量、register spilling 到 local memory（其實在 global）、register pressure 與 occupancy 的取捨、`-maxrregcount` / `__launch_bounds__`。
**動手做**：觀察一個 kernel 的 register 用量（`nvcc --ptxas-options=-v`），調整後看 occupancy 變化。

### 課 4.8 — 佔用率（Occupancy）
**學什麼**：SM 上同時駐留多少 warp，為何「越高不一定越快」。
**核心概念**：theoretical occupancy 的三個限制因子（register、shared memory、block 數）、Occupancy API (`cudaOccupancyMaxPotentialBlockSize`)、occupancy vs ILP 的取捨、achieved occupancy。
**圖解**：occupancy 受限因子示意。
**延伸**：Best Practices Guide「Occupancy」。

### 課 4.9 — 同步與記憶體一致性
**學什麼**：thread 之間怎麼安全協作。
**核心概念**：`__syncthreads()`（block 內 barrier、誤用會 deadlock）、`__syncwarp()`、memory fence (`__threadfence` 系列)、`volatile`、為何 Volta 後需要 `__syncwarp`、race condition 的根源。
**動手做**：用 shared memory + `__syncthreads` 做 block 內歸約，示範漏同步的 bug。

### 課 4.10 — 現代 CUDA C++ 記憶體模型與 Scoped Atomics【v1.9 新增】
**前置**：4.9（`__threadfence`／`volatile`／race 的經典層）；5.9 Atomics 可後補。
**學什麼**：4.9 教的是**經典層**；這課是**現代層**——用 libcu++ 的 `cuda::atomic` 搭配明確的 memory order 與 scope，正確設計 producer-consumer、work queue、persistent kernel 與跨 host/device 同步。這是推論引擎與 persistent kernel 的地基，也是 Staff/Principal 面試會挖到的深度。
**核心概念**：`cuda::atomic<T, Scope>` / `cuda::atomic_ref` / `cuda::std::atomic`（libcu++／CCCL）；**memory order**：`relaxed` / `acquire` / `release` / `acq_rel` / `seq_cst`——**atomicity ≠ ordering**（`atomicAdd` 保證不遺失計數，但**不保證**別人看得到你在那之前寫的資料）；**thread scope**：public `cuda::thread_scope` 列舉**只有四個**——`thread_scope_thread` / `_block` / `_device` / `_system`（**【v2.0.1 更正】本機 CUDA 13.3／CCCL 3.3 實測：`cuda::atomic<T, cuda::thread_scope_cluster>` 直接編譯失敗「namespace "cuda" has no member "thread_scope_cluster"」——v2.0 曾把 v1.9 正確的四值改成「五值含 _cluster」，本輪改回四值。cluster 級 atomic 要走 compiler builtin `__nv_atomic_*` ＋ `__NV_THREAD_SCOPE_CLUSTER`（另一套 API、非 `cuda::atomic`）；cluster 級 barrier「同步」則走 cooperative groups 的 `cluster.sync()`，與 atomic scope 是兩件事**）——scope 開太大＝付不必要的 fence 成本、開太小＝直接壞掉；release-acquire 的 producer-consumer 語意 vs `__threadfence()` 的差別；**system scope atomic** 與 host↔device coherent 溝通（配合 pinned/managed memory）；cluster scope atomics（sm_90+，走 `__nv_atomic_*` builtin、非 public `cuda::atomic`）；lock-free ring buffer／work queue 實作與 **ABA problem**、false sharing、atomic contention 與 **warp-aggregated atomics**。
**動手做**：① 寫一個**故意錯**的 producer-consumer queue（用 `volatile` 或無 ordering 的裸 atomic），觀察 consumer 讀到 stale data；② 用 `release`/`acquire` 修正並證明問題消失；③ 改寫成 persistent kernel 用的 device work queue；④ 比較「naive atomic contention」vs「warp-aggregated atomic」的吞吐。
**延伸**：CUDA C++ Programming Guide「CUDA C++ Memory Model」；libcu++／CCCL `cuda::atomic` 文件。
**備註**：筆電（sm_75）可跑 ①–④（cluster scope 除外，需 sm_90+）。往後接：**8.16** persistent kernel／Cluster Launch Control、**9.17** Paged KV cache 的 block manager、**12.5** IPC、**13.9** production reliability。

**📚 Part 4 參考**：PMPP 5e Ch.4,5,6｜CUDA C++ Programming Guide §5（Memory Hierarchy, SIMT）｜GPU MODE Lecture 4,8｜Best Practices Guide。

---
---

# Part 5 — 平行運算模式與演算法

> 目標：掌握可重複使用的「平行 pattern」。會了這些，你看任何 GPU 演算法都能拆解。每課都是一個經典案例 + 完整程式碼。

### 課 5.1 — 矩陣乘法（Naive GEMM）
**前置**：Part 4。
**學什麼**：最基本、但最重要的運算。
**核心概念**：每個 thread 算一個輸出元素、arithmetic intensity、為何 naive 版是 memory-bound。
**動手做**：naive GEMM + 與 cuBLAS 對比正確性。

### 課 5.2 — 矩陣乘法（Shared Memory Tiled GEMM）
**學什麼**：用 tiling 大幅提升 GEMM。
**核心概念**：tile 載入到 shared memory、data reuse、register tiling（thread coarsening）、逐步逼近 cuBLAS。
**動手做**：tiled GEMM，profiling 看 compute/memory 比例變化。
**延伸**：PMPP「matrix multiplication」章；GPU MODE Lecture 5。

### 課 5.3 — 平行歸約（Parallel Reduction）— 經典 7 步優化
**學什麼**：sum/max/min 這類 reduce 的高效寫法（最經典的優化教材）。
**核心概念**：interleaved vs sequential addressing、避免 bank conflict、避免 divergence、unroll last warp、`__shfl_down_sync` warp shuffle、grid-level reduction。
**動手做**：從 v1 到 v7 逐步優化，每步測速。
**延伸**：Mark Harris「Optimizing Parallel Reduction in CUDA」經典投影片。

### 課 5.4 — 前綴和 / 掃描（Scan / Prefix Sum）
**學什麼**：scan 是無數演算法的基礎（壓縮、排序、稀疏）。
**核心概念**：inclusive/exclusive scan、Hillis-Steele（work-inefficient）vs Blelloch（work-efficient, up/down sweep）、block 級 + 跨 block scan。
**動手做**：實作 Blelloch scan，與 CUB `DeviceScan` 對比。

### 課 5.5 — 直方圖（Histogram）
**學什麼**：處理「多個 thread 寫同一位置」的衝突。
**核心概念**：atomic 衝突、privatization（per-block 私有直方圖再合併）、shared memory atomics、aggregation。
**動手做**：naive atomic 版 → privatized 版，比較。

### 課 5.6 — 卷積 / Stencil
**學什麼**：影像、PDE、CNN 的核心運算。
**核心概念**：1D/2D convolution、halo / ghost cells、用 shared memory 載入 tile+halo、constant memory 放 filter、separable convolution。
**動手做**：2D 卷積（含邊界），對比直接版與 tiled 版。

### 課 5.7 — 排序（Sorting）
**學什麼**：GPU 上怎麼排序。
**核心概念**：bitonic sort、radix sort（GPU 上最常用）、為何比較式排序在 GPU 上吃虧、用 CUB/Thrust 的 device sort。
**動手做**：用 `thrust::sort` 與 `cub::DeviceRadixSort`，理解何時自己寫、何時用庫。

### 課 5.8 — 稀疏運算（Sparse）
**學什麼**：稀疏矩陣在 GPU 上的挑戰（不規則存取）。
**核心概念**：CSR/CSC/COO/ELL/BSR 格式、SpMV、load imbalance、warp/vector per row。
**動手做**：CSR 格式 SpMV，與 cuSPARSE 對比。

### 課 5.9 — 原子操作深入（Atomics）
**學什麼**：atomic 的種類、成本與技巧。
**核心概念**：`atomicAdd`/`atomicCAS`/`atomicExch`、float/double atomic、warp-aggregated atomics、atomic 在 shared vs global 的成本差、用 atomic 實作自訂操作。
**動手做**：用 `atomicCAS` 實作一個自訂 atomic max（float）。

### 課 5.10 — 合作群組（Cooperative Groups）
**學什麼**：現代化、可組合的執行緒協作 API。
**核心概念**：`cooperative_groups` namespace、`thread_block` / `tiled_partition` / `coalesced_group`、grid-wide sync（cooperative launch）、取代手寫 warp 技巧。
**動手做**：用 cooperative groups 重寫 reduction，比較可讀性。
**延伸**：PMPP 5e 新增的 cooperative groups 內容；CUDA C++ Programming Guide。

### 課 5.11 — 圖在 GPU 上的表示與 BFS（不規則存取的試煉）
**學什麼**：把圖（netlist、社群網路、GNN 鄰接）搬上 GPU，從 BFS 開始。
**核心概念**：CSR 鄰接表表示、frontier-based BFS、不規則記憶體存取（先天 uncoalesced）、load imbalance（高分支度頂點）、用 atomic 維護 frontier、push vs pull、bitmap visited。為何這是 warp divergence + coalescing 最嚴苛的試煉。
**動手做**：CSR 圖上的 frontier BFS，量測不規則存取對效能的衝擊。
**備註**：EDA netlist 本質就是大型圖——這課直接用你的領域直覺。

### 課 5.12 — SSSP、負載平衡策略與 GNN 的橋樑
**學什麼**：最短路徑與更一般的圖運算，並把它接到 GNN。
**核心概念**：Bellman-Ford / delta-stepping、work-efficiency、connected components；thread-per-vertex vs warp-per-vertex vs CTA-per-vertex 的負載平衡策略、frontier 壓縮；GNN 的 message passing 本質是稀疏聚合（≈ SpMM / segmented reduction），與課 5.8、6.7 直接相連。
**動手做**：實作 delta-stepping SSSP（或用 cuGraph 跑），對比不同負載平衡策略。
**延伸**：cuGraph、Gunrock 論文。

**📚 Part 5 參考**：PMPP 5e Part II（Parallel Patterns，含 filtering、wavefront、進階 GEMM）｜Mark Harris reduction slides｜GPU MODE lectures repo 範例碼｜cuGraph / Gunrock（圖演算法）。

---
---

# Part 6 — CUDA 函式庫完整實戰

> 目標：把 NVIDIA 的 math/AI 函式庫與 GPU 生態一個一個學會、跑過範例、知道何時用庫、何時自己寫 kernel。**多數課一個函式庫**（6.19 為 Python 端 GPU 生態 CuPy/Numba/RAPIDS 總覽，非單一函式庫）。**【v2.0】新增 6.20/6.21 兩課「GPU-initiated 通訊」**——這是 v1.9 最大的結構性缺口之一：v1.9 只有 host-initiated 的 NCCL collective，完全沒有「通訊下沉到 kernel 內」這一層。

### 課 6.1 — 函式庫生態總覽與選用策略
**學什麼**：CUDA-X 全景、決策樹。
**核心概念**：math libraries（cuBLAS/cuFFT/cuRAND/cuSPARSE/cuSOLVER）、primitives（Thrust/CUB）、AI（cuDNN/CUTLASS/TensorRT/NCCL）、影像（NPP/nvJPEG）。原則：**先用庫，profiling 找到瓶頸再手寫**。
**圖解**：CUDA-X 函式庫地圖。

### 課 6.2 — cuBLAS（稠密線性代數）
**學什麼**：最常用的庫，GEMM 的黃金標準。
**核心概念**：BLAS Level 1/2/3、`cublasHandle_t`、column-major 慣例（C/C++ 常見坑）、`cublasSgemm`/`cublasGemmEx`、stream 綁定、math mode（TF32）；**【v2.0】cuBLAS 13.x 亮點**：grouped GEMM（FP8/BF16，配 CUDA Graphs 免 host sync——MoE 推論直接受益，接 9.35）、FP64 以 Tensor Core 浮點模擬加速。
**動手做**：用 cuBLAS 做 GEMM，與你 Part 5 的手寫版比效能（看差多少）。

### 課 6.3 — cuBLASLt（進階 GEMM：descriptor、epilogue、混合精度）【v2.0 收斂範圍】
**學什麼**：更彈性的 GEMM、混合精度、fusion。
**核心概念**：cuBLASLt 的 matmul descriptor、epilogue fusion（bias+activation）、FP8/FP4 mixed precision matmul、heuristics/autotune 查詢；**device 端的 cuBLASDx 統一移至課 6.18（MathDx 家族）**，本課不再重複。
**動手做**：用 cuBLASLt 做帶 bias+GELU 的 fused matmul。
**備註**：FP4/FP8 對應你 5090 的第 5 代 Tensor Core，這是推論優化重點。

### 課 6.4 — cuDNN（深度學習原語）
**學什麼**：CNN/RNN/Transformer 的底層運算庫。
**核心概念**：convolution（多種演算法與 autotune）、pooling、activation、normalization、cuDNN Graph API（v8+ 的 fusion）、**【v2.0】cuDNN Frontend（開源，現行正門）與 SDPA attention API**（flash attention 全系列、FP8/MXFP8 SDPA、內建 autotune——**常是 Blackwell 上最快的現成 attention**，是「何時不用手寫 attention」的判斷依據，接 9.6）、與 PyTorch 的關係。
**動手做**：用 cuDNN 做一次 forward convolution。

### 課 6.5 — cuFFT（快速傅立葉變換）
**學什麼**：訊號處理、頻域運算。
**核心概念**：1D/2D/3D FFT、plan（`cufftPlan`/`cufftPlanMany`）、real-to-complex、batched FFT、in-place vs out-of-place。
**動手做**：對訊號做 FFT → 頻域處理 → IFFT。

### 課 6.6 — cuRAND（亂數）
**學什麼**：GPU 上產生大量亂數（蒙地卡羅、初始化、dropout）。
**核心概念**：host API vs device API、產生器類型（XORWOW/Philox/MRG）、`curandState` 在 kernel 內、種子與序列。
**動手做**：device API 在 kernel 內產亂數做蒙地卡羅 π 估計。

### 課 6.7 — cuSPARSE（稀疏線性代數）
**學什麼**：稀疏矩陣運算庫。
**核心概念**：稀疏格式、SpMV/SpMM、generic API（descriptor）、與 cuBLAS 搭配。
**動手做**：cuSPARSE SpMM，與課 5.8 手寫版對比。

### 課 6.8 — cuSOLVER（數值求解）
**學什麼**：線性系統、分解、特徵值。
**核心概念**：dense/sparse solver、LU/QR/Cholesky/SVD、特徵值問題、`cusolverDn`。
**動手做**：解一個線性系統 Ax=b、做一次 SVD。

### 課 6.9 — Thrust（高階 STL 風格）
**學什麼**：用近似 C++ STL 的方式寫 GPU 程式（你會非常熟悉）。
**核心概念**：`thrust::device_vector`、`transform`/`reduce`/`sort`/`scan`/`copy_if`、functor、iterator（`zip`/`counting`/`transform`）、execution policy。
**動手做**：用 Thrust 幾行完成 Part 5 裡某些 pattern，對比手寫成本。

### 課 6.10 — CUB（高效能 primitives）
**學什麼**：Thrust 底層、給你細粒度控制的高效原語。
**核心概念**：block-wide / warp-wide / device-wide primitives、`BlockReduce`/`BlockScan`/`DeviceRadixSort`、temp storage 慣例、可組合性；**現代定位（2026）**：libcu++／Thrust／CUB 自 CUDA 12.x 起已整併為 **CCCL（CUDA C++ Core Libraries）** 單一專案，現代寫法深度依賴 CCCL 的 memory model 與 device iterator——學 CUB 應放在「CCCL 生態」框架下看（與 6.9 Thrust、libcu++ 一脈相承）。
**動手做**：用 `cub::BlockReduce` 重寫 block 級歸約。

### 課 6.11 — NPP / nvJPEG（影像）
**學什麼**：影像處理與編解碼加速。
**核心概念**：NPP 影像/訊號函式、nvJPEG 的 JPEG decode/encode pipeline（**硬體 JPEG decoder 是否可用取決於 GPU/backend/library 支援**；視訊硬體編解碼另見課 13.10 NVENC/NVDEC/PyNvVideoCodec）、影像前處理 pipeline（給 CV 模型餵資料）。
**動手做**：用 nvJPEG 解碼一批 JPEG → NPP resize。

### 課 6.12 — CUTLASS / CuTe（模板化 GEMM 與 Tensor Core）
**學什麼**：自己寫高效能 GEMM/卷積的「半成品積木」，FlashAttention-3 就是用它寫的。
**核心概念**：CUTLASS template 階層（device/kernel/threadblock/warp/thread）、epilogue 自訂、CuTe 的 layout/tensor 抽象、Tensor Core (MMA) 程式設計、TMA、針對 sm_120 的 tile 設計。
**動手做**：用 CUTLASS 客製一個帶 epilogue 的 GEMM。
**備註**：這是通往「能寫 production kernel」的關鍵庫，對你的職涯目標極重要。
**延伸**：`github.com/NVIDIA/cutlass`；FlashAttention-2 用 CUTLASS 的論文（附錄 A）。

### 課 6.13 — NCCL（多 GPU 集合通訊）
**學什麼**：多卡之間怎麼高效交換資料（分散式訓練/推論的基礎）。
**核心概念**：collective（all-reduce/all-gather/reduce-scatter/broadcast）、ring/tree 演算法、communicator、與 PyTorch DDP 的關係、消費卡無 NVLink 時走 PCIe 的影響；**【v2.0】現代 NCCL 面貌**：LL/LL128/Simple protocol、topology 偵測、buffer/window registration、RAS——**GPU-initiated 通訊與 Device API 獨立成課 6.20/6.21**。
**動手做**：兩張卡跑 all-reduce（⚠️ **「單卡多 process」不是一般替代方案**——同一 GPU 多 rank 只有 **NCCL 2.30+ 的實驗功能**、需開 `NCCL_MULTI_RANK_GPU_ENABLE=1`，資源不足可能直接 hang，且**無法模擬真實跨 GPU 拓撲/效能**；筆電階段以「讀懂 collective 演算法」為主，實跑等雙卡或雲端）。
**延伸**：NCCL docs；PMPP 5e 新增的 NCCL/NVSHMEM 章。

### 課 6.14 — nvc++ stdpar（C++ 標準平行演算法上 GPU）
**學什麼**：用標準 C++17 `std::transform` 等直接跑在 GPU（最貼近你既有 C++ 習慣）。
**核心概念**：NVIDIA HPC SDK 的 `nvc++ -stdpar`、`std::execution::par`、何時可行、與 Thrust 的關係。
**動手做**：把一段標準 C++ parallel algorithm 編到 GPU 跑。

### 課 6.15 — cuTENSOR：張量收縮與 Einstein 求和
**學什麼**：多維張量的高效收縮（不只是 2D 矩陣）。
**核心概念**：tensor contraction、permutation、elementwise、reduction、Einstein notation（`einsum`）、與 cuBLAS 的差異（任意維度）、用於張量網路 / 高維運算 / 某些 attention 變體。
**動手做**：用 cuTENSOR 做一個多維張量收縮，對比手動 reshape+GEMM。

### 課 6.16 — cuSPARSELt：結構化稀疏與 Tensor Core 加速
**學什麼**：用 2:4 結構化稀疏在 Tensor Core 上加速（推論剪枝的硬體紅利）。
**核心概念**：2:4 structured sparsity（每 4 個元素保留 2 個）、Sparse Tensor Core、稀疏 GEMM、與量化/剪枝搭配、相容精度（**【v2.0 更新】FP16/BF16/INT8/FP8 之外，新增 FP32 組合、Blackwell block-scaled FP8、E2M1/FP4、UE8M0/UE4M3 scale format——不同精度對應不同架構，教材以官方支援表格呈現，不再單列一串 dtype**）；Blackwell 的稀疏支援。
**動手做**：把一個 dense GEMM 改成 2:4 稀疏，量測 Tensor Core 加速。
**備註**：這是「剪枝」真正換到速度的關鍵硬體機制，對推論優化很實用。

### 課 6.17 — cuDSS：直接稀疏求解器
**學什麼**：大型稀疏線性系統的直接解法（科學計算/某些 EDA 場景）。
**核心概念**：稀疏 LU/Cholesky/LDLᵀ、reordering、factorization vs solve 分離、與 cuSPARSE/cuSOLVER 的定位差異。
**動手做**：用 cuDSS 解一個大型稀疏線性系統。
**備註**：EDA 的電路矩陣常是大型稀疏系統——你的領域可能用得上。

### 課 6.18 — MathDx 裝置端函式庫家族（cuBLASDx / cuFFTDx / cuSOLVERDx / cuRANDDx / **nvCOMPDx**）【v2.0 更新】
**學什麼**：把 BLAS/FFT/solver/亂數/壓縮**直接內嵌進你自己的 kernel**（device-side，不用回 host 呼叫庫）。
**核心概念**：device function 形式的數學庫、kernel 內 fusion（例如在一個 kernel 裡做 GEMM + 後續運算，免落地中間結果）、與 host-side 庫（cuBLAS 等）的取捨、編譯需求（template 為主；**⚠️【v2.0】新版 cuBLASDx 已非純 header-only，部分功能走 LTO/fatbin**——與 3.9/12.4 的 device LTO 相接）。
**動手做**：用 cuBLASDx 在 kernel 內做小 GEMM 並 fuse 後續 elementwise。
**延伸**：NVIDIA MathDx 文件。

### 課 6.19 — Python 端 GPU 程式設計：CuPy / Numba CUDA / RAPIDS
**前置**：6.1、3.4。
**學什麼**：不寫 C++ 也能用 GPU——Python 生態的三條常用路。
**核心概念**：**CuPy**（NumPy 的 GPU 版直接替換、`cupy.ndarray`、自訂 `RawKernel`/`ElementwiseKernel`）；**Numba CUDA**（`@cuda.jit` 用 Python 寫 kernel、`cuda.grid()`、與 NumPy/CuPy 互通；**【v2.0 定稿】傳統 numba-cuda 已進入 maintenance mode——定位為既有程式的維護與遷移；新開發主線是 numba-cuda-mlir（見 12.9），教材將列出兩者支援範圍與遷移差異**）；**RAPIDS**（cuDF 取代 pandas、cuML 取代 sklearn、cuGraph 圖分析、cuVS 向量檢索）；**nvmath-python**（【v2.0 定稿更正】已出 **1.0 GA**：host+device 數學 API 統一入口，與 CuPy/MathDx 高度重疊——知道定位即可，不佔課時）；何時用 Python 路 vs CUDA C++（原型/資料科學 vs 極致效能）；與課 12.9 `cuda.core` 的關係、用 `__cuda_array_interface__`/DLPack（13.6）互通。
**動手做**：用 CuPy 改寫一段 NumPy 運算；用 Numba `@cuda.jit` 寫一個 kernel，對比課 3.4 的 C++ 版。

### 課 6.20 — NVSHMEM：GPU-initiated 通訊與 Symmetric Memory【v2.0 新增｜Core】
**前置**：6.13、4.10。
**學什麼**：把通訊「下沉到 kernel 內」——PGAS 程式模型，kernel 直接對遠端 GPU 做 one-sided 存取。這是 v1.9 完全缺席的一層（先前只在 9.28 一句帶過）。
**核心概念**：symmetric heap 與 `nvshmem_malloc`、one-sided put/get/atomic、signal/wait 同步、teams 與 collectives、**GPU-initiated RDMA（IBGDA）**、與 NCCL 的分工（collective vs one-sided）、與 4.10 system-scope atomic 的關係；**DeepEP V1 的 RDMA 底層正是它——【v2.0 定稿註】DeepEP V2 已改用 NCCL GIN（見 6.21），這個遷移本身是絕佳的通訊棧版本演進案例**（接 9.35）。
**動手做**：（需雙卡或雲端）兩卡用 `nvshmem_put` + signal 做 ring 傳遞並量測延遲；單卡階段：讀 DeepEP（V1 分支）原始碼中的 NVSHMEM 呼叫並畫出資料流。
**備註**：⚠️ 需多 GPU；筆電階段以讀為主。
**延伸**：NVSHMEM docs；PMPP 5e NVSHMEM 章。

### 課 6.21 — NCCL Device API 與通訊計算融合【v2.0 新增｜Core】
**前置**：6.13、6.20。
**學什麼**：現代 NCCL 的新世界——symmetric memory 與 device-side 通訊，讓通訊與計算在**同一個 kernel 裡**融合。這是 2025–26 MoE 推論與 TP 延遲優化的核心技術。
**核心概念**【定稿版本鎖定：symmetric memory＝**2.27** 世代、Device API＝**2.28** 起、GIN＝**2.28.7** 起、撰稿時最新文件 **2.30.7**——這組 API 變動快，各功能以附錄 B.6 與官方文件為準】：NCCL symmetric memory（小訊息延遲大幅下降）、**Device API 三模式**：LSA（P2P load/store）／Multimem（NVLink SHARP 硬體 multicast）／GIN（GPU-initiated networking）、copy-engine collectives（把通訊卸載出 SM）、PyTorch `symm_mem`（one-shot/two-shot all-reduce、MoE 用的 `all_to_all_vdev`）、何時值得融合（TP 小訊息延遲、MoE dispatch/combine）、vLLM/SGLang 的採用點。
**動手做**：（需雙卡/雲端）用 PyTorch `symm_mem` 跑 one-shot all-reduce 對比傳統 NCCL collective 的小訊息延遲；讀 NCCL Device API 範例。
**備註**：⚠️ 需多 GPU；概念與程式結構 sm_75 可讀。
**延伸**：NCCL 2.28 Device API docs；NVIDIA 官方部落格「Fusing Communication and Compute」。

**📚 Part 6 參考**：各庫官方 docs（docs.nvidia.com）｜CUTLASS GitHub + examples｜NVIDIA「CUDA Library Samples」repo｜MathDx / cuTENSOR / cuSPARSELt / cuDSS docs｜CuPy / Numba / RAPIDS docs｜NVSHMEM / NCCL Device API docs（v2.0）｜PMPP 5e（cuDNN/NCCL/NVSHMEM 相關章）。

---
---

# Part 7 — 除錯、測試與工程實踐（Debugging, Testing & CI）

> 目標：建立一套抓 CUDA bug 的系統方法，並把「正確性」延伸成「工程紀律」——單元測試、CI、效能回歸。能寫出快的 kernel 很好；能保證 kernel 永遠正確且不掉速，才是 Staff/Principal 的分水嶺（也正好對應你 EDA 的回歸測試直覺）。

### 課 7.1 — 除錯思維與常見 CUDA bug 類型
**前置**：Part 3-4。
**學什麼**：先認識 GPU 程式典型會壞在哪。
**核心概念**：記憶體越界、未初始化記憶體、race condition、漏同步、錯誤的 launch 配置、host/device 指標搞混、非同步錯誤被延後報出、silent wrong result（最可怕）。
**圖解**：bug 分類 → 對應工具 的對照表。

### 課 7.2 — cuda-gdb（原始碼級除錯器）
**學什麼**：像 gdb 一樣單步除錯 kernel。
**核心概念**：`-G`（device debug info）編譯、breakpoint in kernel、切換 focus 到特定 thread/warp/block (`cuda thread`/`cuda block`)、檢視變數、`info cuda` 系列指令。
**動手做**：在 kernel 設斷點，檢查某個 thread 的區域變數。

### 課 7.3 — compute-sanitizer（最重要的除錯工具）
**學什麼**：自動偵測記憶體與同步問題（取代舊的 cuda-memcheck）。
**核心概念**：四種工具——`memcheck`（越界/非法存取）、`racecheck`（shared memory race）、`initcheck`（未初始化讀取）、`synccheck`（同步誤用）。
**動手做**：對一個故意有越界與 race 的 kernel 跑 sanitizer，看它怎麼指出問題位置。
**延伸**：Compute Sanitizer docs。

### 課 7.4 — printf 除錯與 in-kernel assert
**學什麼**：最輕量的除錯手段及其限制。
**核心概念**：device `printf` 的緩衝/非同步性質、用 thread id 過濾輸出、`assert()` in kernel、條件式輸出避免洗版。
**動手做**：用過濾式 printf 觀察特定 warp 的中間值。

### 課 7.5 — 錯誤處理最佳實踐
**學什麼**：把錯誤檢查工程化（延續課 3.7）。
**核心概念**：`CUDA_CHECK` 巨集設計、kernel launch 後的 `cudaGetLastError()` + `cudaDeviceSynchronize()`、**【v2.0】錯誤分級（不是所有 CUDA error 都 sticky）**：可恢復的 API／參數／資源錯誤 vs 非同步 kernel 錯誤 vs **sticky/fatal error**（illegal address、misaligned address、illegal instruction、launch timeout 等——context 進入不一致狀態、通常整個 process 要重啟）、debug/release 雙模式；**【v2.0】CUDA 13.x Error Log Management**：`cuLogs*` API、`CUDA_LOG_FILE`——production 崩潰診斷的新標配（接 13.9）。
**動手做**：做一組 production 級的錯誤檢查 header。

### 課 7.6 — VS Code / Nsight 圖形化除錯
**學什麼**：在 IDE 裡視覺化除錯。
**核心概念**：Nsight Visual Studio Code Edition、CUDA-GDB 後端、warp watch、遠端除錯（在 workstation 上跑、筆電上看）。
**動手做**：設定一次圖形化 kernel 除錯 session。

### 課 7.7 — 競態條件偵測與修復
**學什麼**：race condition 的辨識與根治。
**核心概念**：shared/global memory race、漏 `__syncthreads`/`__syncwarp`、用 `racecheck` 定位、用同步或 atomic 修復、設計上避免共享可變狀態。
**動手做**：修一個 reduction 的 race bug。

### 課 7.8 — 記憶體錯誤偵測與修復
**學什麼**：越界、洩漏、未對齊。
**核心概念**：OOB 的常見成因（索引算錯、邊界沒檢查）、leak（忘了 `cudaFree`）、misaligned access、用 `memcheck` + `compute-sanitizer --leak-check`。
**動手做**：用工具找出並修掉植入的記憶體錯誤。

### 課 7.9 — 數值正確性驗證
**學什麼**：確認「跑得快」之前先「跑得對」。
**核心概念**：CPU reference 實作、浮點容差（為何不能用 `==`）、相對/絕對誤差、確定性（atomic/平行歸約導致的非確定性）、單元測試 GPU 程式。
**動手做**：為一個 kernel 建立 CPU reference + 容差比對的測試框架。

### 課 7.10 — GPU 單元測試框架（GoogleTest + CUDA）
**學什麼**：把 kernel 正確性測試工程化。
**核心概念**：GoogleTest 整合 CUDA、test fixture（配置/釋放 device 記憶體）、CPU reference 比對、浮點容差斷言、參數化測試（掃 size / dtype / 邊界 case）、`ASSERT` vs `EXPECT`、納入 CMake `ctest`；**【v2.0 定稿】host 端測試工具**：ASan/UBSan/TSan（scheduler、block manager、自訂 allocator、PyTorch extension 的大量 bug 發生在 **CPU orchestration 層**——GPU sanitizer 抓不到；**分工明確：TSan 只管 host 執行緒的 race，GPU 端 race 仍由 compute-sanitizer racecheck 負責，見 7.3**）、libFuzzer／property-based testing 對 host 邏輯的應用。
**動手做**：為 Part 5 的某個 kernel 寫一組 GoogleTest 測試（含邊界與隨機輸入）。

### 課 7.11 — GPU 的 CI/CD 策略（含「沒有 GPU 時能做什麼」）
**學什麼**：在持續整合裡守住 kernel 品質。
**核心概念**：**先講清楚界線**——CUDA kernel 沒有可用的 CPU 模擬器（舊的 device emulation 早已移除），所以「無 GPU 的 CI runner」只能做：跨架構**編譯檢查**（compile-only，可在無卡機器 `nvcc -arch=...`）、host 端邏輯/orchestration 測試、靜態分析 / lint；**真正執行 kernel 的正確性測試需要 self-hosted 或雲端 GPU runner**。分層 pipeline：PR 階段跑 compile + host 測試 +（GPU runner 上）compute-sanitizer；nightly 跑完整 GPU 正確性 + 效能。
**動手做**：設計一個兩階段 CI（compile-only gate + GPU 測試 job）的設定範例。

### 課 7.12 — 效能回歸測試（Performance Regression Testing）
**學什麼**：自動抓「改壞速度」。
**核心概念**：benchmark harness、鎖時脈確保可比性、warm-up、記錄 baseline（kernel 時間 / 頻寬 / 達成 occupancy）、回歸門檻（如慢 >5% 就 fail）、處理量測雜訊（多次取中位數）、效能歷史趨勢；**【v2.0】NVBench**：NVIDIA 官方 benchmark harness（warm-up、clock lock、統計分布、cache state、同步位置都幫你做對）——嚴謹量測方法論的落地工具，別只用 `cudaEvent` 土炮。對高階職位而言，這套自動化的價值高於單一快 kernel。
**動手做**：建一個記錄 baseline 並偵測回歸的腳本，接進 CI。
**備註**：直接對應你 EDA 的回歸測試經驗，是很好的能力展示點。

### 課 7.13 — 浮點語意與確定性（GPU 上的 IEEE-754 實務）【v2.0 新增｜Core】
**前置**：7.9、5.3。
**學什麼**：9.18 講的是 AI kernel 的數值穩定「技巧」；這課補完整的浮點「語意」地基——CPU/GPU 結果不一致的案子，八成從這裡來。
**核心概念**：rounding mode、ULP 與誤差量測、**FMA contraction**（`-fmad=true` 是 CPU/GPU 結果不同的頭號原因）、denormal／flush-to-zero、`--use_fast_math` 逐項拆解（它到底改了哪些東西）、平行歸約順序與 atomic 造成的非確定性、跨架構／跨版本重現性、determinism 的工程策略（固定 reduction 順序、integer 化、容差設計）；compute-sanitizer 的 `-fdevice-sanitize=memcheck` 編譯期插樁（13.x）；**【v2.0 定稿】數值重現性政策**：PyTorch deterministic algorithms、逐 dtype/operator 的容差表、Philox/dropout state 管理、stochastic rounding、跨 CUDA/ROCm 的 differential testing（接 10.15）、**「bitwise reproducible」vs「numerically acceptable」的區別**與何時該要求哪一種。
**動手做**：製造一個 CPU/GPU 因 FMA contraction 而結果不同的最小案例並解釋；把一個非確定性 reduction 改造成確定性版本，量測代價。
**備註**：sm_75 完全可跑。與 9.18（AI 數值穩定）互補、不重複。

**📚 Part 7 參考**：Compute Sanitizer User Manual｜CUDA-GDB docs｜Nsight VS Code Edition docs｜GoogleTest + CUDA 整合範例｜NVBench（v2.0）｜CUDA Floating Point 白皮書（v2.0）｜各 CI 平台的 self-hosted GPU runner 文件｜Mojo GPU Puzzles 除錯章（觀念通用）。

---
---

# Part 8 — 效能優化（一步一步）

> 目標：這是把「能跑」變「夠快」的部分，也是面試與實務最看重的能力。**方法論 → 工具 → 各層優化技巧 → 完整案例**。

### 課 8.1 — 優化方法論與 Roofline 模型
**前置**：Part 4 必修。
**學什麼**：別瞎調，先建立科學的優化流程。
**核心概念**：APOD 循環（Assess → Parallelize → Optimize → Deploy）、roofline model（arithmetic intensity vs 頻寬上限/算力上限）、memory-bound vs compute-bound 判定、「先量測再優化」、找 hotspot（80/20）。
**圖解**：roofline 圖；優化決策流程圖（Nsight Systems → Compute）。
**延伸**：CUDA C++ Best Practices Guide（整本就是這個）。

### 課 8.2 — Nsight Systems（系統層 profiling）
**學什麼**：先看大局——CPU/GPU 互動、資料傳輸、kernel 排程。
**核心概念**：timeline、CUDA/NVTX trace、找出 GPU idle、H2D/D2H 是否擋路、kernel 是否串行化、`nsys profile`、NVTX 自訂標記。
**動手做**：profiling 一個含多 kernel + 記憶體傳輸的程式，找出最大浪費。
**延伸**：Nsight Systems docs。

### 課 8.3 — Nsight Compute（kernel 層 profiling）
**學什麼**：放大單一 kernel，看它卡在哪。
**核心概念**：`ncu` CLI / `ncu-ui`、guided analysis、speed of light（SOL）、memory workload analysis、occupancy section、warp state、roofline section、baseline 比較、鎖時脈 (`--clock-control`) 量測穩定性。
**動手做**：對 tiled GEMM 跑 Nsight Compute，讀懂每個 section 並找改進點。
**延伸**：Nsight Compute Profiling Guide。

### 課 8.4 — 記憶體頻寬優化
**學什麼**：大多數 kernel 是 memory-bound，這裡榨頻寬。
**核心概念**：coalescing（複習 4.4）、向量化存取 (`float2/float4`、`__align__`)、減少冗餘 global 存取、L2 residency control (`cudaAccessPolicyWindow`)、`__restrict__` 幫助編譯器。
**動手做**：把一個 memory-bound kernel 的 `float` 改 `float4`，量測頻寬提升。

### 課 8.5 — Shared Memory 優化
**學什麼**：消除 bank conflict、用 double buffering。
**核心概念**：bank conflict 診斷（Nsight 指標）、padding、swizzling、double/triple buffering（prefetch 下一個 tile）、shared/L1 carveout 調整。
**動手做**：消除 GEMM 中的 bank conflict，量測差異。

### 課 8.6 — 佔用率與啟動配置優化
**學什麼**：選對 block size 與資源用量。
**核心概念**：occupancy 三限制因子取捨、`__launch_bounds__`、`-maxrregcount`、block size 掃描實驗、occupancy 不是越高越好（high ILP + low occupancy 有時更快）。
**動手做**：① 對某 kernel 做 block size sweep，畫出效能曲線；② **【v1.9】寫一個可重用的 autotuner**——用 Python 驅動「編譯 → 執行 → `ncu --csv` 收 metrics」，對 block size／tile size／`__launch_bounds__` 做網格搜尋，輸出最佳 config 與效能表。這支工具之後 8.14、9.7、10.15 都會再用到，本身也是作品集素材。

### 課 8.7 — 指令層優化
**學什麼**：在算術與指令層面榨效能。
**核心概念**：FMA、intrinsics（`__fmaf_rn`、`__sinf` 等）、`--use_fast_math` 的取捨、減少 divergence、loop unrolling (`#pragma unroll`)、整數除法/取模的成本、減少 register 使用。
**動手做**：對 compute-bound kernel 套用 intrinsics 與 unroll，量測。

### 課 8.8 — 串流與並行（Streams & Concurrency）
**學什麼**：讓「計算」和「資料傳輸」重疊、多 kernel 並行。
**核心概念**：CUDA stream、預設 stream 的同步語意、`cudaMemcpyAsync` + pinned memory、copy/compute overlap、event (`cudaEvent`) 計時與相依、multi-stream pipeline、stream priority。
**動手做**：把「搬入→算→搬出」改成 3-stage pipeline 重疊，量測整體加速。

### 課 8.9 — CUDA Graphs
**學什麼**：消除大量小 kernel 的啟動 overhead（推論常見）。
**核心概念**：launch overhead 問題、stream capture vs explicit graph API、graph instantiate/launch、適用場景（固定拓撲、重複執行）、與推論引擎的關係；**進階**：graph update（更新參數免重建）、graph recapture、conditional graph nodes（圖內條件分支）、device-side graph launch；以及在 PyTorch（`torch.cuda.graph`）/ TensorRT / ONNX Runtime 中使用 graph capture 的限制（靜態形狀、固定位址）；**【v1.9】Programmatic Dependent Launch（PDL，⚠️【v2.0 補標】需 sm_90+：筆電與 4090 皆不可跑、概念為主）**——`cudaGridDependencySynchronize()` / `cudaTriggerProgrammaticLaunchCompletion()` 讓後續 kernel 的 prologue 與前一個 kernel 尾段重疊，消除 launch 之間的氣泡（官方 CUDA Features 章節；對 decode 階段的小 kernel 鏈特別有用）。
**動手做**：把一串小 kernel 用 graph capture 起來比較啟動成本；再用 graph update 改參數重播。

### 課 8.10 — 非同步記憶體與 Copy Engine
**學什麼**：更細的記憶體傳輸控制。
**核心概念**：pinned (page-locked) memory、`cudaHostAlloc`、zero-copy、`cp.async`（Ampere+ 非同步 global→shared 拷貝）、memory pool (`cudaMallocAsync`)、TMA（Hopper/Blackwell 的 tensor memory accelerator 概念）；**【v1.9】現代非同步 API**——`cuda::barrier`（Asynchronous Barriers）與 `cuda::pipeline`（Pipelines）：用 arrive/wait 把 global→shared 搬運與計算組成多階段 producer-consumer pipeline，是 `cp.async` 的高階可組合寫法（官方 CUDA Features 章節；接 4.10 的 scoped atomics 與 8.16 persistent kernel）。
**動手做**：用 `cp.async` 做 shared memory prefetch；再用 `cuda::pipeline` + `cuda::barrier` 改寫成多階段 pipeline，比較可讀性與效能。

### 課 8.11 — Tensor Core 程式設計
**學什麼**：用 Tensor Core 把矩陣運算推到極限（AI 的核心算力）。
**核心概念**：Tensor Core 是什麼、支援精度（FP16/BF16/TF32/FP8/FP4/INT8）、WMMA API (`nvcuda::wmma`)、`mma.sync` PTX、fragment、針對 Blackwell 第 5 代 Tensor Core 的 FP4、為何要透過 CUTLASS。
**動手做**：用 WMMA 寫一個 16×16 Tensor Core GEMM。
**延伸**：CUDA C++ Programming Guide「Warp Matrix Functions」；CUTLASS。

### 課 8.12 — Warp-level Primitives
**學什麼**：warp 內 thread 直接交換資料，免用 shared memory。
**核心概念**：`__shfl_sync` 家族（shuffle）、`__ballot_sync`/`__any`/`__all`（vote）、`__reduce_*_sync`（**CC 8.0+ 硬體 warp reduce；⚠️ 你的 sm_75 沒有，要用 `__shfl_down` 或 CUB `WarpReduce` 手動 fallback**）、warp-level scan/reduction、應用在 reduction/sort/掃描。
**動手做**：用 shuffle 寫無 shared memory 的 warp reduction。

### 課 8.13 — Kernel Fusion 與減少記憶體往返
**學什麼**：合併多個運算成一個 kernel，省下中間結果的讀寫。
**核心概念**：為何 elementwise 鏈很浪費頻寬、fusion 的好處與限制、epilogue fusion、與 `torch.compile`/Triton 的 fusion 概念連結。
**動手做**：把「matmul → bias → activation」三個操作 fuse 成一個 kernel。

### 課 8.14 — 完整案例研究：從 naive 到接近 peak
**學什麼**：把整套方法用在一個真實 kernel 上，做出完整 before/after。
**核心概念**：選一個 GEMM 或 attention，從 naive 開始，依序套用 tiling → coalescing → bank conflict 消除 → 向量化 → Tensor Core → double buffering，每步用 Nsight 量測、記錄百分比，最後對比 cuBLAS/CUTLASS。
**動手做**：產出一份「優化日誌」——這正是面試時最有說服力的作品。
**延伸**：FlashAttention 系列論文（IO-aware 優化的典範）；GPU MODE Lecture 8。

### 課 8.15 — 自訂記憶體分配器（Memory Pool / Arena / Slab）
**學什麼**：自己管理 GPU 記憶體，避開原生分配 API 的高開銷——這是推論引擎核心競爭力的基礎技術。
**核心概念**：為何 `cudaMalloc`/`cudaFree` 在熱路徑上太貴（可能觸發 device 同步）；預先配置一大塊 global memory 再次分配（sub-allocate）；free-list、arena（bump allocator）、slab（固定大小塊池）、對齊與碎片化（internal/external）、`cudaMallocAsync` + memory pool (`cudaMemPool`) 作為現成積木（**註：`cudaMallocAsync`/stream-ordered mempool 早在 CUDA 11.2 就引入，非 13.x 新增；舊環境重現範例前先確認該 toolkit 版本是否含此 API**）；**更底層的 VMM（Virtual Memory Management，`cuMemCreate`/`cuMemMap`/`cuMemAddressReserve`）**（【v2.0.5】Driver API，**CUDA 10.2 起就有、比 11.2 的 `cudaMallocAsync` 更早**，是各種彈性記憶體池的基石）——可保留位址空間後再掛實體記憶體（vLLM 等用來做可成長 KV cache 的基礎）；多執行緒下的分配策略；**【v1.9】Memory Synchronization Domains（⚠️【v2.0 補標】此子節需 sm_90+；本課 allocator/pool/arena/VMM 主體 sm_75 完全可做）**（`cudaLaunchAttributeMemSyncDomain`）——把不同用途的流量（如計算 kernel vs 通訊 kernel）分到不同 domain，避免彼此的 memory fence 互相拖累（官方 CUDA Features 章節；多 GPU／通訊重疊時的實務調校點，接 Part 12/13）。
**動手做**：實作一個簡單的 GPU memory pool / arena allocator，benchmark 對比直接 `cudaMalloc`。
**備註**：9.17 會把這套概念接到推論引擎的 KV cache 專用版本。

### 課 8.16 — Thread Block Clusters、Distributed Shared Memory 與 Persistent Kernel（Hopper/Blackwell 進階執行）
**前置**：Part 4、8.10。
**學什麼**：2024 後 NVIDIA 主推的進階執行/排程特性，以及給不規則工作負載的 persistent kernel。
**核心概念**：Thread Block Cluster / CGA（Hopper sm_90 引入、Blackwell 延續）——多個 block 組成 cluster 並共享 **distributed shared memory**（跨 block 直接讀彼此 shared memory）；`cudaLaunchKernelEx` 設 cluster 維度、cluster 級同步；**persistent kernel** pattern（kernel 常駐、自己從 work queue 拉任務做 work-stealing，適合動態長度序列/稀疏等不規則 workload）；與 8.9 CUDA Graphs、8.10 TMA 的搭配；**【v1.9】Work Stealing with Cluster Launch Control (CLC)**——以硬體佇列做 thread block cancellation／動態取工作，是比手刻 atomic work queue 更現代的 persistent kernel 路線（官方 CUDA Features 章節：Cluster Launch Control API、thread block cancellation 的步驟與限制）。
**動手做**：用 thread block cluster + distributed shared memory 做跨 block 資料共享；或寫一個 persistent kernel 處理動態工作佇列。
**延伸**：CUDA C++ Programming Guide「Thread Block Clusters」「Distributed Shared Memory」。
**備註**：⚠️ cluster 需 **sm_90+**——**筆電 Quadro RTX 4000 (sm_75) 與 4090 (sm_89) 皆不支援**；**【v2.0.5 更正】可跑的是 sm_90+——H100/H200 雲端才是 cluster 的正宮實作場，未來 5090 (sm_120) 桌機亦可**（v2.0.4 誤寫成「僅 5090」）→ **本課的 cluster/CLC 部分需桌機或雲端**（CLC 另需 Blackwell）。persistent kernel 的 atomic work queue 版本（接 4.10）在 sm_75 可跑，觀念可先讀。

### 課 8.17 — Warp Specialization 與三條 Tensor Core MMA 路線（WGMMA / tcgen05 / mma.sync）
**前置**：8.11、8.16。
**學什麼**：最快的 GEMM/attention kernel 用的進階模式，以及**不同架構的 Tensor Core 指令其實互不相容**——這點搞錯就會編不過。
**核心概念**：warp specialization——同一 block 內不同 warp 分工（producer warp 用 TMA 搬資料、consumer warp 算 MMA），用 async pipeline 重疊（FlashAttention-3 的核心技巧）。**⚠️ 三條互不相容的 MMA 路線（按架構）**：
- **Hopper sm_90a → WGMMA**（`wgmma.mma_async`，warp-group 級、128 thread、async）。**這是 Hopper 專屬**，已**正式標 deprecated**；在資料中心 Blackwell 由 tcgen05 接手，在消費級 sm_120 則是**直接不可用**（無 TMEM/tcgen05，改走擴充 `mma.sync`）；**把 WGMMA 程式碼編到 sm_120 會直接報 `ptxas error: wgmma.fence not supported on sm_120`**。
- **資料中心 Blackwell 10.x family（B200/GB200 `sm_100`、B300/GB300 `sm_103`）→ tcgen05.mma（CUTLASS 稱 UMMA）**：單執行緒語意、操作數放 shared memory + **Tensor Memory (TMEM)**、設計目標是比 Hopper WGMMA 更高吞吐與更省的資料路徑（NVIDIA CUTLASS 文件列出在其 GEMM benchmark 上可達 2–4×，但實際加速取決於 dtype/shape/kernel/memory pipeline，需用 CUTLASS profiler / Nsight 實測）。**目標卡 5090 沒有 TMEM、不支援 tcgen05**。
- **消費級 Blackwell sm_120（目標卡 5090）→ 擴充版 `mma.sync`**：Ampere 以來同一指令家族、register-based（無 TMEM），加上 FP4/FP6 等新格式與 block-scaled MMA。**【v2.0 精確化】PTX ISA 8.8 起 dense block-scaled MMA 用 family target `sm_120f` 即可，sparse 版才需 `sm_120a`。**
**所以在 5090 上：用擴充 `mma.sync` / block-scaled MMA，或（更務實）透過 CUTLASS/CuTe、CUDA Tile（8.18/8.19）來用 Tensor Core，不要手寫 WGMMA。**
**動手做**：讀 FA-3 或 CUTLASS 的 warp-specialized GEMM 理解 producer/consumer 切分（概念，sm_75 可讀）；**（需 Blackwell 桌機／雲端）** 在 sm_120 上用 **CUTLASS/CuTe 或 cuTile** 寫一個 block-scaled 小 GEMM（**不是** WGMMA）——⚠️ **FP4/FP6 微縮放的 memory layout 極複雜，不要用裸 PTX `mma.sync` 手刻，一律靠 CUTLASS／cuBLASDx／CUDA Tile 封裝**。
**備註**：⚠️ 「SM90=WGMMA、SM100=tcgen05、SM12x=mma.sync」三者 kernel **不相容**；跨架構請靠 CUTLASS/CuTe 或 CUDA Tile 抽象，別假設同一份 MMA 程式碼到處跑。

### 課 8.18 — CUDA Tile C++ 實作（13.3 新模型，hands-on）
**前置**：8.5、8.11、9.8（DSL 全景，**可先略讀概念、之後補全**——避免 Part 8 卡在 Part 9）。
**學什麼**：用 CUDA 13.3 的 **CUDA Tile C++** 寫高階 tile kernel，讓編譯器自動處理平行/搬運/Tensor Core（而不必手刻 thread index）。
**核心概念**：**`--enable-tile`**（⚠️【v2.0 更正】雙破折號，官方寫法；另可把選項直傳 tile 最佳化組譯器 `tileiras`）編譯、tile 標頭與型別、**單 thread/block 啟動**（編譯器接管 thread 細節）、tile partition + mma、`__restrict__` / 16-byte 對齊 / masked load/store、用 Nsight Compute 看 tile 統計；對照手刻 CUDA C++ 與 Triton。**效能可攜性 caution**：tile abstraction 雖高階，但**不能假設同一份 tile kernel 在 B200/GB10/RTX 5090/H100 上都接近最佳**——仍需 profiling、架構特化設定、autotuning 與 fallback path。**CompileIQ / ACF**（13.3 編譯期自動調優；【v2.0 精確化】opt-in 流程＝`pip install compileiq` 產 ACF 檔 → `--apply-controls` 餵給 nvcc/ptxas；**除 CUDA C++ 外亦支援 Triton 與 PyTorch Helion（以當版 CompileIQ release notes 為準）**；官方宣稱關鍵 kernel 最高 ~15%）**預設關閉、只當進階實驗**，依官方文件有 caveats（限 offline 編譯流程、**不支援 NVRTC**（13.3 release notes 明文）、啟用時不保證 correctness），故須保留 non-ACF fallback + 正確性測試 + benchmark，**不承諾固定加速比例、不要在 Capstone 未驗證就開啟**。
**動手做**：用 CUDA Tile C++ 寫一個 tiled GEMM，對比課 5.2 手刻版與 cuBLAS。
**備註**：這是 NVIDIA 力推的新方向，會寫的人還少——對你是差異化亮點。**因為很新，實作時務必釘住版本**（CUDA Toolkit 13.3+、driver R610+ for JIT、支援 GPU、查當期 known issues），並準備 **CUTLASS / 手刻 CUDA 的 fallback path**——遇 Tile 編譯器 bug 或不支援情境可退回。

### 課 8.19 — cuTile Python 實作：用 Python 寫 tile kernel
**前置**：8.18、9.7（Triton 對照）、9.8（DSL 全景）——**9.7／9.8 可先略讀概念、之後補全**。
**學什麼**：用 **cuTile Python**（CUDA Tile 的 Python DSL）寫 tile kernel——門檻比 C++ 低，是進入 tile 程式設計最快的路。
**核心概念**：`import cuda.tile as ct`、**`@ct.kernel` 裝飾器**標記 kernel、`ct.launch()` 從 host 啟動、tile primitives（`ct.bid()` 取 block id、`ct.load(t, index=, shape=)` / `ct.store`、tile 上的運算）；自動處理 thread 映射/shared memory/Tensor Core(MMA/TMA)/bank conflict、跨架構**語意可攜（同一份碼能編能跑，但 ≠ 效能可攜——換架構仍需 profiling/特化，見 8.18 caution）**；與 CuPy/PyTorch 用 DLPack 互通（接 13.6）；何時用 cuTile Python vs Triton vs CUDA C++。
**動手做**（⚠️ **需 sm_80+ 卡，sm_75 筆電不可跑**）：用 cuTile 寫 `vector_add` → simple GEMM → fused attention，對比課 5.2 手刻 C++、課 9.7 Triton 版的程式碼量與效能。
**備註**：支援條件（依官方 quickstart / cutile-python 1.4.0+；**【v2.0】撰稿時最新 1.5.0（2026-07），一律以官方 release notes 為準**）：GPU **compute capability 8.x（Ampere/Ada，13.2+）、9.0（Hopper——CUDA 13.3 / cuTile Python 1.4.0 起新增）、10.x/11.x/12.x（Blackwell 及 Jetson Thor，13.1+）**。cuTile Python 1.4.0 同時加入 block-scaled MMA、新 float dtype、JAX 整合。driver **R580+**（執行用）；Nsight Compute 擷取 tile 詳細統計需 **580.126.09+**（Linux；仍屬 R580 系列，非 R590）；CUDA Tile C++ 的 JIT 程式需 **R610+**；以 quickstart / release notes 為準。Python 3.10–3.14（含 3.14t）。安裝 `pip install cuda-tile`，或 `pip install cuda-tile[tileiras]` 連 tileiras / nvcc / nvvm 一起裝（免裝整個 CUDA Toolkit；相關 package major.minor 版本需一致）；教學範例見 NVIDIA/TileGym 與 accelerated-computing-hub 的 cuda-tile tutorial。**⚠️ 修正常見誤傳**：裝飾器是 `@ct.kernel`（不是 `@cutile.kernel`）。

### 課 8.20 — LLM-in-the-loop Kernel 工程【v2.0 新增｜Core】
**前置**：7.10–7.12、8.3、8.6。
**學什麼**：2026 年的工程實務——用 LLM 產 kernel 第一版、用工具鏈把關。要教的是「把關能力」，不是「讓 AI 代寫」。
**核心概念**：correctness harness 設計（CPU reference＋容差＋隨機測試，接 7.9/7.10）、**profiler-in-the-loop**（把 Nsight Compute 指標回饋給 LLM 迭代）、KernelBench 與衍生 benchmark、LLM 的典型失效模式（「看似對但慢」「微妙錯」的 kernel、reward hacking 警世案例）、哪些層級可信 vs 必須人審（【v2.0 精確化】**在特定題型、agent 搜尋預算與 verifier 配置下**，elementwise／常見 pattern 可超越 torch.compile baseline——KernelBench 原始結果顯示整體成功率仍有限，別過度宣稱；warp-specialized／tcgen05 級一律人審）、與 8.6 autotuner 的組合。方法論優先於任何特定產品——該領域產品半衰期太短。
**動手做**：用你的 8.6 autotuner + 7.10 測試框架搭一個「LLM 產生 → 自動驗證 → profiling 回饋」迴圈，對一個中等難度 kernel 跑三輪迭代並記錄品質變化。
**備註**：sm_75 完全可跑。這課同時是 Capstone 的生產力加速器。
**延伸**：KernelBench（Stanford）；NVIDIA 部落格（DeepSeek-R1 + validator 生成 kernel）。

**📚 Part 8 參考**：CUDA C++ Best Practices Guide（核心）｜Nsight Systems / Compute docs + Profiling Guide｜CUDA C++ Programming Guide（Thread Block Clusters、Distributed Shared Memory、Warp Matrix / WGMMA）｜CUDA Tile C++ 與 cuTile Python 文件｜TileGym / cutile-python / accelerated-computing-hub（cuda-tile tutorial）｜CompileIQ｜CUTLASS / CuTe｜FlashAttention-3 論文｜KernelBench（v2.0）｜GPU MODE Lecture 1,4,8｜PMPP 5e Ch.6。

---
---

# Part 9 — AI / 深度學習的 CUDA 系統知識

> 目標：把前面的 CUDA 功力接到**現代 AI 系統**上。這是你職涯主軸（NVIDIA / Anthropic / Synopsys / MediaTek 的 Staff/Principal 推論引擎方向）最關鍵的一部分。**【v2.0】本 Part 新增 10 課**（9.0、9.30–9.38），補齊 v1.9 的兩大缺口：模型架構語彙（9.0/9.34/9.37）與機架級推論系統（9.32/9.33）。

### 課 9.0 — 現代 LLM 架構解剖【v2.0 新增｜Core】
**前置**：無（建議在 9.1 之前讀）。
**學什麼**：2026 年的模型為什麼長這樣——v1.9 教了「運算怎麼映射到 GPU」，卻沒有一課系統性回答「架構本身」。RoPE/GQA/RMSNorm/SwiGLU 是 Staff 面試假設你已內化的基本語彙；不懂 RoPE 的頻率結構，就無法正確把它 fuse 進 attention kernel。
**核心概念**：原始 Transformer → 2026 標準棧（LLaMA/Mistral/Gemma/Qwen 共同採用）的每個替換與其**推論影響**：**RoPE** 的數學與 kernel fusion、**YaRN／NTK-aware／LongRoPE** 長上下文擴展（serving 時必須正確處理的 metadata）、**RMSNorm vs LayerNorm**（bandwidth 差異）、**SwiGLU/GeGLU**（三矩陣 FFN 對 GEMM shape 的影響）、**MHA→MQA→GQA** 演進脈絡（深入在 9.24）、attention sink、logit softcapping、weight tying；讀懂 `config.json` 每個欄位。
**動手做**：拆解一個 Llama 級模型的 config 與 state_dict，畫出每層 tensor shape 與 FLOPs/bytes 帳；手算 RoPE 並驗證與 HF 實作一致。
**備註**：sm_75 可跑（分析為主）。本課是 9.1／9.6／9.24／9.29 的共同前置。

### 課 9.1 — 深度學習運算如何映射到 GPU
**前置**：9.0、Part 4 + 課 8.1–8.6（其餘 Part 8 可後補——**別讓「先碰一點 AI mapping」被整個 Part 8 堵住**）。
**學什麼**：神經網路到底在 GPU 上算什麼。
**核心概念**：DL = 大量 GEMM + conv + elementwise + reduction + attention；forward/backward 的運算圖；為何 attention 是現代 LLM 的瓶頸；compute vs memory-bound 的 layer 分類；batching 的意義。
**圖解**：Transformer block 的運算拆解圖。

### 課 9.2 — PyTorch 與 CUDA 的關係
**學什麼**：你最常用的框架底下發生什麼。
**核心概念**：tensor 與 device、`aten`/ATen dispatch、autograd、cuBLAS/cuDNN backend、CUDA stream 在 PyTorch 中、`torch.cuda` API、為何有時 PyTorch 慢（kernel launch、未 fuse）。
**動手做**：用 `torch.profiler` 看一個模型實際呼叫了哪些 CUDA kernel；並用 **NVTX 標記 + `torch.profiler`（Kineto）把 Python 呼叫堆疊對映到 Nsight Systems 時間軸**，揪出 DataLoader／GIL 造成的 GPU starvation（Python↔CUDA 交界的 CPU overhead——推論實務最常見的隱形瓶頸）。

### 課 9.3 — 在 PyTorch 中寫自訂 CUDA kernel
**學什麼**：把你寫的 kernel 接進 PyTorch（實務上極常見的工作）。
**核心概念**：`torch.utils.cpp_extension`（`load_inline` / setuptools build）、自訂 op 註冊、tensor↔指標、autograd 自訂 backward、`TORCH_CHECK`、與 `torch.compile` 的整合；**【v2.0】現代註冊流程**：`torch.library` / `TORCH_LIBRARY`、**fake/meta kernel**（讓 `torch.compile`/export 能追蹤你的 op）、autograd 註冊、`opcheck`/`gradcheck` 驗證、stable ABI；**⚠️ 實務雷——ABI 不相容**：官方 PyTorch wheel 是用特定 manylinux GCC 編的。**好消息**：走 `cpp_extension.load`／`load_inline`／`BuildExtension`（PyTorch 2.7）時，它會**自動幫你對齊 PyTorch 的 `_GLIBCXX_USE_CXX11_ABI` 旗標**，正常流程多半不會撞。**真正高風險**在：手工 CMake、預編譯的第三方 extension、或自己覆寫 flags——這時要用 `torch._C._GLIBCXX_USE_CXX11_ABI` 查值、加 `-D_GLIBCXX_USE_CXX11_ABI=0/1` 對齊，或用官方相容編譯環境。
**動手做**：把 Part 5/8 的某個 kernel 包成 PyTorch extension 並呼叫；**故意製造一次 ABI 連結錯誤再修好**，記住這個雷。
**延伸**：GPU MODE Lecture 1,3（load_inline / 自訂 kernel）。

### 課 9.4 — 在 Blackwell (sm_120) 上正確安裝 PyTorch（未來桌機／雲端 lab）
**學什麼**：選對 PyTorch wheel，避開 Blackwell 的「跑得起來但沒用到」陷阱。
**核心概念**：**【v2.0 現況】版本矩陣閱讀法**——**以 2.12 官方 install matrix 為準**：cu130（CUDA 13.0）為 PyPI 預設 stable（2.11 起轉換、2.12 定案）、**cu128 於 2.12 標記棄用**、2.12 另有實驗性 CUDA 13.2 build；`pip install torch` 預設即含 sm_120（v1.9「預設抓不到 Blackwell」的警語已失效）。課程驗證版本見〈附錄 B.6 版本鎖定矩陣〉；本課教的是讀官方 release compatibility matrix 的方法——「最低可用／課程驗證／最新可選」三欄思維，而非記版本號。**務必實測驗證**而非假設：`torch.cuda.get_device_capability()` 回 `(12, 0)`、`torch.cuda.get_arch_list()` 含 `sm_120`、實際跑一個 kernel、並做一次 extension build（JIT 與 AOT）測試。**工具鏈完整性雷（【v2.0】修正概念——v1.9 把兩層混為一談）**：`cpp_extension.load()` 的「JIT」實際是 **Python→Ninja→host compiler＋nvcc（ptxas）** 的建置流程，**不是**直接呼叫 nvPTXCompiler。四條路徑要分清：① extension build（Ninja＋nvcc）② driver PTX JIT ③ NVRTC ④ nvPTXCompiler 程式化 API（見 2.4 checklist 與 12.3/12.4）。安裝包/映像若缺 nvcc/ptxas/headers 等元件 → ① 失敗而 AOT（預編 wheel）正常；`libnvptxcompiler_static.a` 缺件影響的是 ④ 那層（nvJitLink／自建 JIT 系統）——症狀相似、根因不同，除錯時先分層。
**動手做**（目標＝Blackwell 桌機／雲端；筆電 sm_75 只做「看懂 install matrix」）：選對 wheel 後 **強制三項實測**——① `get_arch_list()` 含 `sm_120`、② 真跑一個 matmul、③ 真 build 一個自訂 extension（JIT + AOT 各一次）。**光看 `get_arch_list()` 不算通過**（wheel 列了 arch 仍可能 driver／PTX JIT 出事）。
**備註**：版本仍在演進（2.7 → 後續版本），開課時以當下 install matrix 為準；重點是「會看相容矩陣」而非記版本號。

### 課 9.5 — 混合精度與量化
**學什麼**：用低精度換速度與記憶體（推論的核心手段）。
**核心概念**：FP32/TF32/FP16/BF16/FP8(E4M3,E5M2)/FP4/INT8；數值範圍與精度取捨；AMP（自動混合精度）；訓練後量化 (PTQ) vs 量化感知訓練 (QAT)；GPTQ/AWQ/SmoothQuant；QLoRA；5090 第 5 代 Tensor Core 的 FP4 對推論的意義；per-tensor/per-channel scale；**weight-only 量化 kernel（W4A16）的底層實作**——bit-packing 解碼、dequant→GEMM 的 kernel fusion（Marlin 風格），這是推論引擎工程師的高頻面試題與實務核心；**【v2.0】生態更新**：PyTorch 量化主力已移往 **TorchAO**（`torch.ao.quantization` 淡出）；Marlin 針對 **Ampere** 最佳化，Hopper 主流是 **Machete**（CUTLASS mixed-input GEMM，vLLM 內建），Blackwell 則轉向 **NVFP4/MXFP4**（深度見 9.30）；checkpoint 產製標準工具為 TensorRT **Model Optimizer（ModelOpt）**。
**動手做**：對一個小模型做 FP16/INT8 量化並比較精度與速度；讀一個 W4A16（Marlin 風格）dequant+GEMM fused kernel，理解 bit-packing 解碼如何與 Tensor Core matmul 融合。

### 課 9.6 — FlashAttention 原理與實作
**學什麼**：現代 LLM 的關鍵 kernel，IO-aware 優化的典範。
**核心概念**：標準 attention 的記憶體瓶頸（materialize N×N）、tiling + online softmax 避免寫出大矩陣、recomputation、FA-1/FA-2/FA-3 演進（FA-3 用 warp specialization、async WGMMA、FP8）、為何能達 ~75% H100 peak；KV cache。**⚠️ FA-3 的原始 Hopper/WGMMA 路線不適用於 sm_120**（在 5090 上跑不了）——但 FA-style kernel 在 Blackwell 仍可行，改用 Blackwell 的 mma.sync/block-scaled、Triton、或 cuTile 實作（呼應課 8.17 三條 MMA 路線）。**【v2.0.2 更正】FlashAttention-4**（2026-03，arXiv:2603.05451）：**全部以 CuTe DSL（Python）撰寫**、**官方套件支援 Hopper＋Blackwell（README：optimized for Hopper and Blackwell GPUs，例 H100/B200）；論文的硬體設計與數據則聚焦資料中心 Blackwell B200 的非對稱硬體擴展**（切分：套件支援範圍 ⊋ 論文聚焦硬體）；論文實測 **B200/BF16 下比 cuDNN 9.13 快 ~1.3×、比 Triton 快 ~2.7×、達 1613 TFLOPs/s（71% 使用率）**（**原「比 FA-3 快 1.5–2×」查無論文出處，已刪**）；consumer sm_120 支援仍屬部分。本課升級為 **FA-1→4 演進史**：**階段 2 先精讀 FA-1/FA-2 與 online softmax（sm_75 可讀）；FA-4 的 CuTe DSL code-reading 屬進階，排在 9.31（CuTe DSL）之後（階段 3），別在還沒學 DSL 前硬讀**。
**圖解**：標準 attention vs FlashAttention 的記憶體流動對比。
**動手做**：讀 FA 的簡化實作；理解 online softmax 的數學。
**延伸**：FlashAttention 論文 + Tri Dao 的程式碼；GPU MODE 相關講次。

### 課 9.7 — Triton 入門與深入
**學什麼**：用 Python 寫高效 GPU kernel（門檻比 CUDA C++ 低，產業很流行）——並理解它如何 lower 到課 1.4 學過的 PTX，以及它的多後端架構。
**核心概念**：Triton 程式模型（program/tile 思維、`tl.load`/`tl.store` 與 mask、自動處理 coalescing 與 shared memory）、**編譯管線：Triton IR → TritonGPU IR → LLVM IR → PTX**（**Triton 的 codegen 到 PTX 為止**）→ 再由 NVIDIA 工具鏈的 `ptxas`／nvPTXCompiler 產 **SASS**（在 NVIDIA 上是**原生路徑**、不是轉接層；但別誤以為 Triton 自己實作了 SASS 產生器）、`num_warps`/`num_stages` 與 software pipelining、**autotune 深入**（`@triton.autotune` 搜尋空間設計、key 參數、autotune cache）、**dump 產出的 PTX/IR** 與課 1.4 對照、與 `torch.compile`（Inductor 後端產 Triton）的關係、何時用 Triton vs CUDA C++；【v2.0.3】Triton 兩種進階定址要分清：`tl.make_block_ptr`＋`tl.advance` 是**結構化區塊定址**（一般用途、**非** sm_90+ 專屬）；要吃到 Hopper/Blackwell 的 TMA 高頻寬則走**顯式 tensor descriptor**（`tl.make_tensor_descriptor` 及其 load/store，TMA backing、屬 sm_90+ TMA-capable GPU 路徑，別把 TMA 綁在 block pointer 上）；sm_75 兩者皆無 TMA、概念先懂；**多後端架構（v1.8）**：同一份 Triton 原始碼可由 AMD 後端編到 AMDGCN（實作見 10.14/10.15），但「**可攜 ≠ 效能可攜**」——換硬體需重新 autotune。
**動手做**（⚠️ **GPU 執行需 sm_80+ 卡**）：用 Triton 寫 vector add 與 fused softmax，對比 CUDA C++ 版；dump PTX 觀察 lowering；**待有 4090(sm_89)／5090(sm_120) 或雲端卡**，再用 autotune 在兩種架構各自搜尋最佳 config，體驗「同一 kernel、不同架構、不同最佳參數」。**筆電 sm_75 階段**：Triton 只用 interpreter 讀語法，效能實驗走 **CUDA C++ 黃金路線**。
**備註**：⚠️ **Triton 官方支援矩陣自 3.3 起最低 CC 8.0、已移除 Turing（sm_75）**（【v2.0】撰稿時上游已至 3.7.x，支援矩陣未變）——上游把 pre-Ampere 列為**未測試的 best-effort**（FMA 路徑仍在，部分簡單 kernel 或許偶然可跑），但 **sm_75 不可作為可靠教學／正確性／效能平台**；本課程把筆電的 Triton 視為「只用 interpreter 讀語法」，效能實驗一律走 **CUDA C++ 黃金路線**。**不建議為 sm_75 釘舊 Triton 3.2**（會與新 PyTorch/CUDA 形成版本債）。sm_120 需 torch ≥ 2.7（歷史最低門檻為 CUDA 12.8 wheel；**本課基準環境用 cu130**，見 9.4／附錄 B.6）。一律實測為準。
**延伸**：OpenAI Triton 官方 tutorials；GPU MODE Triton 講次；Triton 論文（Tillet et al., MAPL 2019）。

### 課 9.8 — 現代 Kernel DSL 全景
**學什麼**：2026 年寫 AI kernel 的幾種主流途徑與取捨。
**核心概念**【v2.0 改版為「2026 DSL 光譜」】：CUDA C++（最大控制）、CUTLASS/CuTe C++（模板積木、production GEMM/attention）、**CuTe DSL（Python，CUTLASS 4.x——效能天花板級 kernel 的新主流授權語言，FA-4／cuDNN 部分 backend 已採用，實作見 9.31）**、Triton（OpenAI，Python、快速迭代）、**Gluon（Triton 官方低階 dialect：顯式 layout／shared memory／warp specialization，Triton 使用者下探硬體控制的官方路徑）**、**Helion（PyTorch 高階 DSL→編譯到 Triton、自動 autotune）**、ThunderKittens（tile-based DSL、簡潔）、TileLang、Mosaic GPU（JAX Pallas 後端）、**cuda.lang（CUDA Python 的 SIMT kernel DSL，⚠️ Experimental、快速演進，見 12.9）**；**教學取捨**：實作課＝CUDA C++／Triton／cuTile／CuTe DSL（＋Gluon 半課 lab），其餘進地圖——這個生態仍在快速洗牌，**「怎麼評估一個新 DSL」比精通任何單一 DSL 保值**；**CUDA Tile（CUDA 13.1 起的全新 tile 程式模型，務必分清三層）**：① **CUDA Tile IR**（MLIR 後端，給寫編譯器/DSL/函式庫的人）② **cuTile Python**（Python DSL，CUDA **13.1 起**支援 Blackwell 10.x/12.x；CC 8.x Ampere/Ada 需 13.2+；CC 9.0 Hopper 13.3 起，`import cuda.tile`）③ **CUDA Tile C++**（13.3 新增，`--enable-tile`/tile 標頭、單 thread/block 啟動、**C++20+**——NVCC 13.3 另支援到 C++23、可用 Nsight Compute profiling）。各途徑的抽象層級、效能、可維護性與硬體支援（CUDA Tile C++ 需 CC 8.x+、driver R580+、toolkit 13.3+）。
**圖解**：抽象層級 vs 控制力的光譜圖（CUDA C++ ↔ CuTe DSL/CUTLASS ↔ Gluon ↔ Triton ↔ Helion/cuTile）。
**CUDA Tile 支援摘要（單一處集中，細節見 8.18/8.19）**：cuTile Python——CC 8.x（Ampere/Ada，13.2+）、9.0（Hopper，**CUDA 13.3 起**）、10.x/11.x/12.x（Blackwell，13.1+），driver R580+（Nsight tile 詳細 profiling 需 **580.126.09+**，仍屬 R580 系列）；CUDA Tile C++——CC 8.x+、toolkit 13.3+、driver R580+（JIT 需 R610+）。每個 Tile 範例在章節內標「已驗證環境」與 fallback（CUTLASS / 手刻 kernel）。
**延伸**：NVIDIA CUDA Tile 部落格與 `NVIDIA/cuda-tile`、cuTile Python（PyPI `cuda-tile`）、各專案 GitHub。

### 課 9.9 — 推論引擎架構（一）：TensorRT / TensorRT-LLM
**學什麼**：NVIDIA 官方的高效能推論引擎。
**核心概念**：TensorRT 的 build-time 優化（layer fusion、precision calibration、kernel auto-tuning、engine 序列化）、plugin 機制、TensorRT-LLM 的 LLM 專用優化（in-flight batching、paged KV cache、FP8/FP4）、與 Triton Inference Server 搭配部署。
**動手做**：把一個模型轉成 TensorRT engine，比較延遲/記憶體。
**延伸**：TensorRT / TensorRT-LLM docs + GitHub。

### 課 9.10 — 推論引擎架構（二）：vLLM / SGLang
**學什麼**：開源 LLM serving 的主流。
**核心概念**：PagedAttention（KV cache 分頁管理，解決記憶體碎片）、continuous/in-flight batching、prefix caching、tensor parallelism、speculative decoding、SGLang 的 RadixAttention 與結構化生成；**【v2.0】vLLM V1 架構**（V0 已於 2025 移除——**任何 V0 架構圖教材已失效**）：unified scheduler、chunked prefill、zero-overhead prefix caching、KV connector/transfer——講 vLLM 內部一律以 V1 為準。
**動手做**（**需 ≥16–24GB VRAM 桌機或雲端**；筆電 8GB 只能跑玩具級小量化模型）：用 vLLM 服務一個模型，觀察吞吐與 KV cache 行為。
**延伸**：vLLM / SGLang docs + 論文。

### 課 9.11 — 推論引擎架構（三）：llama.cpp / GGUF
**學什麼**：CPU+GPU、本地、量化推論的代表。
**核心概念**：GGUF 格式、量化等級（Q4_K_M 等）、CUDA backend、offload layer 到 GPU、為何適合本地/混合部署、與 vLLM/TensorRT 的定位差異。
**動手做**：用 llama.cpp 跑量化模型，調 GPU offload 層數。

### 課 9.12 — 推論引擎架構（四）：ONNX Runtime / FlashInfer
**學什麼**：跨框架部署與可插拔 kernel 庫。
**核心概念**：ONNX 圖、ONNX Runtime 的 CUDA/TensorRT execution provider、FlashInfer（把 attention/GEMM/communication/sampling 拆成可插拔高效 kernel，供 vLLM/SGLang 使用，JIT 生成）。
**動手做**：把模型導成 ONNX 並用 ORT CUDA EP 推論。
**延伸**：ONNX Runtime docs；FlashInfer GitHub + 論文。

### 課 9.13 — KV Cache、Batching 與 Serving 系統設計
**學什麼**：推論「系統」層面的設計（Staff 級會被問的東西）。
**核心概念**：KV cache 大小估算與管理、prefill vs decode 階段特性、continuous batching、request scheduling、latency (TTFT/ITL) vs throughput 取捨、記憶體預算規劃、量化對 serving 的影響；**【v1.9】production scheduler 的硬需求**——admission control、**backpressure**（過載時排隊/拒絕而不是崩潰）、**request cancellation**（client 斷線要能中止 decode 省算力）、timeout／deadline-aware scheduling、preemption 與 priority queue、fairness／starvation 防治、overload protection、token budget scheduling（這些比再學一套 serving framework 更決定服務品質；接 13.9）；**【v2.0】reasoning／agentic workload 對假設的顛覆**：reasoning 模型 **decode-heavy**（與傳統 chat 的長 prefill／短 decode 相反）、思考鏈 KV 爆炸（**量級示例**：70B 級 GQA 模型、FP16 KV 下，10K-token CoT 每個 decode step 約讀 ~3 GB KV——實際依層數/KV heads/head dim/dtype 而定，**這筆帳要會自己算**，見 9.24 的估算法）、test-time scaling 的多分支 KV 共享、thinking token 可較激進淘汰；agentic trace 特徵（多輪 tool call、prefix 命中率 60–85%——**特定 trace／服務的觀測值**，但「KV 命中率直接等於錢」的結論通用）、TTFT vs 總延遲的 SLO 重定義、對 P/D 配比的衝擊（接 9.32/9.33）。
**圖解**：LLM serving 系統方塊圖（scheduler / KV manager / kernel）。

### 課 9.14 — 多 GPU 與分散式
**學什麼**：模型放不下一張卡時怎麼辦。
**核心概念**：data / tensor / pipeline / expert(MoE) parallelism、ZeRO/FSDP、collective（NCCL）、NVSHMEM、通訊與計算重疊、消費級多卡（無 NVLink）的限制與 PCIe 拓撲、推論時的 tensor parallel。
**動手做**：用兩張卡做 tensor parallel 推論（⚠️ **「單卡多 process 模擬」只有 NCCL 2.30+ 實驗功能**支援同 GPU 多 rank，需 `NCCL_MULTI_RANK_GPU_ENABLE=1`、可能 hang，且不代表真實拓撲——沒有雙卡就以概念與程式結構為主）。
**延伸**：PMPP 5e 多 GPU + NCCL/NVSHMEM 章。

### 課 9.15 — GPU 記憶體管理與 OOM 排除
**學什麼**：訓練/推論最常見的實務問題。
**核心概念**：VRAM 用量拆解（weights/activations/KV cache/optimizer states/碎片）、`torch.cuda.memory_summary`、caching allocator、gradient checkpointing、activation offload、`PYTORCH_CUDA_ALLOC_CONF`（新名 `PYTORCH_ALLOC_CONF`）、量化省記憶體、抓記憶體洩漏；**【v2.0】memory debugging 工具鏈**：memory snapshot／visualizer（`_record_memory_history`＋官方視覺化網頁）、expandable segments、pluggable allocator 與 `MemPool` API（接 8.15 的自訂 allocator）；**【v1.9】Lazy Module Loading**（官方 CUDA Features 章節）——CUDA 只在真正用到時才載入 module/kernel，對「連結大型函式庫（cuDNN/TensorRT）時的啟動時間與 VRAM 佔用」影響顯著；`CUDA_MODULE_LOADING` 環境變數、版本支援與冷啟動延遲量測。
**動手做**：診斷並修一個 OOM 情境（**目標機 VRAM**——例如把模型塞進 5090 的 32GB；筆電 8GB 則練「塞進 8GB」的等比例縮小版）。

### 課 9.16 — 從 EDA 到 AI：你的經驗如何遷移
**學什麼**：把你 25 年 C/C++/EDA 的資產對應到 AI/GPU 領域（你個人最有價值的一課）。
**核心概念**：EDA 的時序/記憶體/平行直覺如何對應 GPU；TCL/腳本經驗對應 build/部署自動化；LSF job scheduling 對應多 GPU/叢集排程；C++ 效能調校經驗直接可用；EDA × AI 的交集（用 GPU 加速 placement/routing/timing、ML for EDA）；如何包裝履歷與作品集切入 Staff/Principal。
**備註**：這課也談面試策略——把你的 capstone（自訂 attention kernel 或 mini 推論引擎）說成故事。

### 課 9.17 — Paged KV Cache 的 Block Manager（推論引擎的心臟）
**學什麼**：vLLM 之所以快的核心——把 KV cache 當作分頁記憶體來管理。
**核心概念**：KV cache 的記憶體痛點（變長序列、碎片）；把 KV 切成固定大小 block、block table（logical→physical 映射）、動態配置/回收 block、prefix sharing 與 copy-on-write（多 request 共享前綴）、beam search 的 block 共享；**實務澄清**：block manager 通常跑在 **host 端**（CPU scheduler），GPU kernel 只是依 block table 做 gather/scatter——所以關鍵技能是「host 端 block 管理邏輯 + 消費 block table 的 attention kernel」，而非追求 device 端 lock-free 分配（那是更 niche 的進階題）；與課 8.15 的 allocator 概念一脈相承。
**動手做**：實作一個精簡版 paged KV block manager（host 端）+ 一個依 block table 取 KV 的 kernel。
**延伸**：vLLM PagedAttention 論文（附錄 A）；對照課 9.10、9.13。

### 課 9.18 — 數值穩定性：穩定的 softmax / log-sum-exp / LayerNorm / Welford
**前置**：Part 5（reduction）、9.1。
**學什麼**：AI kernel 裡最容易被忽略、卻會炸的數值坑。
**核心概念**：naive softmax 的 overflow（exp 大數）→ 減 max 的穩定 softmax；log-sum-exp trick；LayerNorm/RMSNorm 的穩定計算（兩遍 vs Welford 單遍求 mean/variance）；低精度（FP16/BF16/FP8）下用 FP32 accumulator 累加；catastrophic cancellation；與 9.6 online softmax（FlashAttention）的關係。
**動手做**：寫 naive vs 穩定 softmax，在 FP16 下製造 overflow 觀察差異；用 Welford 寫穩定 LayerNorm。

### 課 9.19 — 把自訂 kernel 接進推論框架：TensorRT Plugin / ONNX Runtime Custom Op
**前置**：9.3、9.9、9.12。
**學什麼**：讓你的 kernel 不只在 PyTorch 能用，也能接進 production 推論框架（這是「會寫強 kernel」→「能接進真實 AI 系統」的關鍵一步）。
**核心概念**：TensorRT plugin（IPluginV3 介面、註冊、serialize、與 engine 整合）；ONNX Runtime custom op / 自訂 CUDA EP kernel；何時需要 plugin（框架沒有的 op、特殊融合、特殊精度）；與 9.3 的 PyTorch extension 對照——同一個 kernel，接不同框架的不同 FFI。
**動手做**：把 Part 8/9 寫的某個 kernel 包成一個 TensorRT plugin（或 ORT custom op）。
**延伸**：TensorRT plugin 文件、ONNX Runtime custom operator 文件。

### 課 9.20 — 多租戶與 GPU 共享：MPS（與 MIG 概念）
**前置**：9.13（10.2 多執行緒/叢集整合為背景，可並讀或後補——不阻擋本課）。
**學什麼**：一張卡服務多個 process/模型時怎麼共享而不互相拖累。
**核心概念**：CUDA MPS（Multi-Process Service）——**【v2.0 更正】Volta 之後每個 MPS client 通常保有自己的 CUDA context 與位址空間，MPS server 協調並共享 GPU 排程資源**（「多 process 共用一個 context」是 pre-Volta 舊模型），減少排程開銷、提升小 workload 利用率，適合多租戶推論；**限制與新能力**：僅 Linux/QNX、**dynamic parallelism 不支援**（接 12.6）、R610 起有 static SM partitioning 與 fault isolation；MPS 的資源限制（active thread percentage、記憶體）；**【v2.0】Green Contexts**（CUDA 12.4+、13.1 起有 runtime API）：SM 粒度切分的第三選項——GPU 共享光譜＝time-slicing／MPS／green contexts／MIG；**【v2.0 定稿】多租戶 KV/prefix cache 安全**：tenant isolation、cache salting、跨租戶 timing side channel（prefix cache 命中時間差可探測他人 prompt——實證見〈Auditing Prompt Caching in Language Model APIs〉arXiv:2502.07776）、敏感 prompt 殘留與 eviction policy（接 10.11 安全、13.9 運維）；**MIG（Multi-Instance GPU）概念**——硬體層切割成隔離實例、QoS；**【v2.0 統一口徑】支援範圍＝官方 MIG Supported GPUs 清單所列的資料中心 GPU＋部分 RTX PRO Blackwell SKU；GeForce（目標卡 5090/4090）不支援**，此處只講概念以備未來伺服器環境；MPS（軟性共享）vs MIG（硬性隔離）的取捨。
**動手做**：開啟 MPS，跑多個推論 process 觀察利用率與延遲變化。
**備註**：MPS 在消費卡一般可用（**以 driver/SKU 實測為準，不作泛化保證**）；GeForce 無 MIG；MIG 支援清單見 1.9（含部分 RTX PRO Blackwell）。

### 課 9.21 — Triton Inference Server：多模型/多後端部署
**前置**：9.9、9.13。
**學什麼**：把模型變成可上線的服務（serving 平台層）。
**核心概念**：**先消歧義——這裡的 Triton 是 NVIDIA Triton Inference Server（serving 平台），跟課 9.7 的 OpenAI Triton（寫 kernel 的語言）完全是兩回事，只是同名**；model repository、多後端（TensorRT / PyTorch / ONNX / Python backend）、**server 層的 dynamic batching**（與 kernel 層 batching 不同）、concurrent model execution、model ensemble、metrics / health endpoint；與 TensorRT-LLM、vLLM 的搭配部署。
**動手做**：用 Triton Inference Server 部署一個模型，設定 dynamic batching，發 inference request。
**延伸**：Triton Inference Server 文件。

### 課 9.22 — PyTorch 編譯器內部：torch.compile / TorchInductor / AOTInductor
**前置**：9.2、9.7。
**學什麼**：`torch.compile` 底下到底發生什麼（Staff 級會被問的框架內部）。
**核心概念**：TorchDynamo（graph capture、guard、graph break）、TorchInductor（後端；**【v2.0 精確化】產出不只 Triton——會按情況選 Triton kernel／ATen／cuBLAS/cuDNN／CUTLASS／external templates，「Inductor＝Triton」是過度簡化**）、AOTAutograd、**AOTInductor**（提前編譯部署；現代產物除 `.so` 外還有 **`.pt2` archive**）、dynamic shape 處理、kernel cache、與課 9.7 Triton 的連結；**【v2.0】FlexAttention**：用 `torch.compile` 生成自訂 attention 變體（score_mod／block mask）——很多情況下是「取代手寫 attention kernel」的第一選擇（與 9.6/9.24 相接）。
**動手做**：對一個模型開 `torch.compile`，用 `TORCH_LOGS` 看它生成的 Triton kernel 與 graph break。

### 課 9.23 — PyTorch 推論優化：CUDA Graphs in PyTorch 與 ONNX 匯出
**前置**：8.9、9.12、9.22。
**學什麼**：把訓練好的模型推到低延遲推論的實務技巧。
**核心概念**：`torch.cuda.graph` / CUDA graph trees（捕捉前向、消 launch overhead，接課 8.9）、`reduce-overhead` 模式、靜態形狀需求；ONNX 匯出常見坑（dynamic axes、不支援的 op、opset 版本、與 9.12 ONNX Runtime / 9.19 plugin 的銜接）。
**動手做**：對推論模型套 CUDA graph 並量測延遲；匯出 ONNX 並用 ORT 驗證數值一致。

### 課 9.24 — LLM 注意力變體與 KV cache 進階：GQA / MQA / MLA、量化與淘汰
**前置**：9.6、9.13、9.17。
**學什麼**：現代 LLM 用來縮小 KV cache 的注意力變體與管理技巧（直接決定能服務多少 context/併發）。
**核心概念**：MHA → **MQA（multi-query）→ GQA（grouped-query）→ MLA（multi-head latent attention，DeepSeek）** 的演進與 KV 大小取捨；**KV cache 量化**（FP8/INT8/INT4 KV）；**KV eviction / compaction**（H2O、StreamingLLM、滑動窗）；**【v2.0 定稿】DeepSeek Sparse Attention（DSA）／token 級 sparse attention**：lightning indexer、top-k token selection、sparse prefill/decode、與 FP8 KV cache 的搭配、與 H2O/StreamingLLM 的本質差異（**選取式** sparse vs **淘汰式** eviction——前者 KV 全保留、只算被選中的；案例實作見 9.36 的 DeepSeek kernel 生態）；對 kernel 與 block manager（9.17）的影響。
**動手做**：估算 MHA vs GQA vs MLA 的 KV 大小；實作 FP8 KV cache 並比較精度/容量。

### 課 9.25 — 進階解碼與取樣 kernel：speculative / sampling / constrained
**前置**：9.13、5.3（reduction）。
**學什麼**：decode 階段的演算法與對應 GPU kernel。
**核心概念**：prefill vs decode 的差異複習；**speculative decoding**（draft model / Medusa / **EAGLE-3【v2.0 更新：現行主流，已進 vLLM/SGLang/TRT-LLM】**、verify kernel）；**取樣 kernel**（greedy、temperature、top-k、top-p/nucleus、repetition penalty 的高效實作）；**constrained / structured decoding**（JSON/grammar、logit mask）、function calling；beam search 的 GPU 實作。
**動手做**：寫一個 top-k/top-p 取樣 kernel；用 logit mask 實作結構化輸出。

### 課 9.26 — LoRA / Multi-LoRA serving、MoE 推論與 disaggregated serving
**前置**：9.13、9.14、9.5。
**學什麼**：把推論引擎推到「多適配器、多專家、prefill/decode 分離」的進階架構。
**核心概念**：**LoRA / Multi-LoRA serving**（同時掛多個 adapter、批次中混合不同 LoRA、SGMV kernel）；**MoE 推論**（routing、expert 分派、expert parallelism、token drop）；**disaggregated prefill/decode**（prefill 與 decode 跑不同節點/池，如 DistServe/Mooncake 思路）、KV 傳輸；多節點推論；**production 級 disaggregation 的完整實作（Dynamo/NIXL）獨立成課 9.33**。
**動手做**：用 vLLM/SGLang 跑 Multi-LoRA 或 MoE 模型，觀察 routing 與批次行為。

### 課 9.27 — AI 編譯器全景：從 PyTorch model 到 GPU kernel
**前置**：9.2、9.7、9.22。
**學什麼**：建立一張完整地圖——一個模型到底怎麼一路變成 GPU 上跑的 kernel（Staff 級的「整體理解」）。
**核心概念**：PyTorch eager → TorchDynamo（capture）→ AOTAutograd → **TorchInductor**（fusion + 產 Triton）→ PTX/SASS；旁支：TorchScript（legacy）、**XLA / JAX**、ONNX export → ONNX Runtime EP、**TensorRT** graph 優化 + plugin、以及 **MLIR / TVM / IREE** 生態；**tensor layout / stride / memory format（channels_last 等）互通**與 DLPack（接 13.6）。重點不是每個都精通，而是看懂「graph capture → fusion → codegen → runtime scheduling」這條鏈。
**圖解**：PyTorch model → graph → kernel → runtime 的全景流程圖。

### 課 9.28 — 分散式訓練進階：Transformer Engine、FP8 訓練、序列/上下文平行（次要）
**前置**：9.14、6.13。
**學什麼**：補齊「完整 AI CUDA 知識」的訓練側（**對純推論目標屬次要，可略過或之後再讀**）。
**核心概念**：在 9.14（data/tensor/pipeline/expert parallel、ZeRO/FSDP）之上補：**NVIDIA Transformer Engine**（FP8 訓練/推論的 Transformer 層庫）、**FP8 訓練**（scaling、穩定性）、**sequence / context parallel**（長序列）、gradient communication overlap、checkpoint save/load 效能、NCCL 深入與 NVSHMEM。
**動手做**：用 Transformer Engine 跑一段 FP8 訓練，觀察 scaling 與精度。
**備註**：⚠️ 你的主線是推論；這課列在這裡是為了「完整地圖」，優先級低於 9.24–9.26。RL 後訓練的 **rollout 側**（推論工程師真正會碰到的訓練交集）獨立成課 9.38。

### 課 9.29 — 從零用 CUDA 實作神經網路（MLP/CNN 前向 + 反向）
**前置**：5.1–5.2（GEMM）、9.1、9.3。
**學什麼**：不靠框架，親手把一個小神經網路的前向與反向傳播寫成 CUDA kernel——這是真正搞懂「模型如何映射到 GPU 運算」的最佳練習。
**核心概念**：把 MLP/CNN 拆成 GEMM + bias + activation + loss；前向各層的 kernel；**反向傳播**（鏈式法則、梯度的 kernel、權重更新）；用 shared memory tiling 的 GEMM 當核心；數值梯度檢查（gradient check）；與 cuBLAS/cuDNN 對照（看自己手寫差多少）。
**動手做**：在 MNIST 上用純 CUDA 實作一個 MLP（前向+反向+訓練迴圈），驗證收斂並對比 cuBLAS 版；**【v2.0】延伸：Transformer block kernel 全套**——手寫 embedding lookup、RMSNorm、RoPE、SwiGLU FFN、logits projection、sampling 的完整 kernel 鏈，串成一個能跑的 mini-Llama 前向（9.0 的架構語彙在此落地；這正是 Capstone 選項 B 的地基）。
**備註**：這課直接餵給 Capstone 選項 G（純 CUDA 訓練 NN）。

### 課 9.30 — Microscaling 量化格式深度：MXFP4/MXFP8 與 NVFP4【v2.0 新增｜Core】
**前置**：9.5、8.11（8.17 可並讀）。
**學什麼**：Blackwell 推論的預設精度是怎麼運作的——「FP4」不是一個數字格式，是一套 **block scaling 體系**；9.5 一筆帶過已不夠。
**核心概念**：OCP **MXFP**（32 元素 block、E8M0 二次冪 scale）vs **NVFP4**（16 元素 block、E4M3 FP8 scale＋per-tensor FP32 的兩層 scaling）的數值分析（動態範圍／精度取捨）；tcgen05 與 sm_120 block-scaled MMA 的 **scale factor layout**（資料排布極複雜，呼應 8.17 的「別裸寫 PTX」警告）；dequant-fused GEMM 的 kernel 寫法；NVFP4 官方 checkpoint 生態（DeepSeek-R1/Llama/Nemotron）與 **ModelOpt** 產製工作流；FP8/FP4 KV cache（接 9.24）；NVFP4 訓練 recipe 概觀。
**動手做**：（需 Blackwell／雲端）用 ModelOpt 量化一個小模型到 NVFP4 並量測精度/速度；sm_75 階段：手算 MXFP4/NVFP4 的量化-還原誤差，理解兩層 scale 各自的作用。
**備註**：⚠️ 硬體加速需 Blackwell；數值分析部分 sm_75 可做。

### 課 9.31 — CUTLASS 4.x CuTe DSL：用 Python 寫效能天花板級 kernel【v2.0 新增｜Core】
**前置**：6.12、9.7。
**學什麼**：CuTe 的 layout 代數搬進 Python——MLIR JIT 到 ptxas；【v2.0 精確化】官方 benchmark（B200 特定 workload）編譯時間改善約 **33–116×**、效能與 C++ 版 **comparable**（benchmark-specific，非普遍定律）；**FlashAttention-4 全部用它寫**。2026 年寫新 attention/GEMM 變體的主流起手式。
**核心概念**：`nvidia-cutlass-dsl` 套件、編譯管線（Python → MLIR → ptxas）、layout/tensor 代數（與 6.12 CuTe C++ 同一套心智模型——C++ 學過的直接遷移）、TMA/tcgen05 等新硬體特性的第一手暴露、`cute.compile_to` 細粒度編譯 API、與 Triton/cuTile 的定位差異（控制力 vs 生產力）、讀 FA-4 原始碼的地圖。
**動手做**：（⚠️ 需 sm_80+；完整特性需 Hopper/Blackwell）用 CuTe DSL 寫 tiled GEMM，對比 6.12 C++ 版與 9.7 Triton 版的程式碼量／編譯時間／效能。
**備註**：對你的職涯目標，重要性等同 6.12。sm_75 可讀原始碼與 layout 代數。

### 課 9.32 — KV Cache 作為儲存系統：分層、Offload 與分散式 Cache Pool【v2.0 新增｜Core】
**前置**：9.17、9.13。
**學什麼**：KV cache 從「單機分頁」升級為「**GPU→CPU DRAM→NVMe→遠端儲存**的分散式系統」——agentic 時代 KV 命中率直接等於成本。這與你 EDA 的儲存階層／資源排程直覺高度對口。
**核心概念**：分層 KV 體系與遷移策略、PCIe 頻寬競爭（offload vs weights/activations）、**LMCache**（vLLM/SGLang 的 cache 中介層：CPU/SSD/Redis/S3/GDS/NIXL 多後端、prefix 重用、跨 engine 共享）、**Mooncake**（分散式 KVCache pool、用叢集閒置 DRAM/SSD 建 cache 層、KVCache-centric scheduling；vLLM×Mooncake 官方實測吞吐 3.8×／P50 TTFT ÷46——【v2.0 精確化】**該數字的條件是 1P1D、12 GPU、特定 agentic trace，不能當一般結論**）、NVIDIA Dynamo 的 KVBM（見 9.33）、prefix cache 命中率經濟學。
**動手做**：估算一個 agentic 服務的 KV 分層容量／頻寬帳；（桌機/雲端）架 LMCache＋vLLM，量測命中率對 TTFT 的影響。
**備註**：sm_75 可做估算與小規模實驗。

### 課 9.33 — NVIDIA Dynamo 與 NIXL：Disaggregated Serving 的 Production 實作【v2.0 新增｜Core】
**前置**：9.26、9.32。
**學什麼**：9.26 講了 disaggregation 概念；這課補 production 落地層——NVIDIA 官方分散式推論框架（已 1.0 GA），v1.9 完全缺席的最大 AI 生產系統缺口。
**核心概念**：Dynamo 架構（**SLO-based Planner、KV-aware Router、KV Block Manager** 跨 HBM/DRAM/SSD 階層）、**NIXL**（推論專用傳輸庫：統一 NVLink/InfiniBand/GDS/物件儲存的非同步 KV 傳輸、memory registration/descriptor 模型、與 forward pass 的 overlap）、engine-agnostic orchestration（vLLM/SGLang/TRT-LLM 皆可掛）、prefill/decode 分離（**【v2.0.5 釐清】純 LLM 只有 prefill/decode；多模態場景才多出 vision **encode** 一段成為三段，見 9.37**）、host 層為何用 Rust（接 10.16）、Dynamo Snapshot 秒級冷啟動（【v2.0 精確化】⚠️ **仍屬 experimental／limited preview**，有 privileged DaemonSet、backend 與 multi-GPU 限制；接 13.8）。
**動手做**：（雲端/雙節點）用 Dynamo 部署一個 P/D 分離的小服務、用 NIXL benchmark KV 傳輸延遲；單機階段：讀 design docs 畫出完整資料流。
**備註**：⚠️ 完整 lab 需多節點；概念與 API sm_75 可讀。

### 課 9.34 — Hybrid／Linear Attention 與 SSM 推論：當 KV Cache 假設被顛覆【v2.0 新增｜Core】
**前置**：9.24、9.17、9.0。
**學什麼**：Qwen3-Next／MiniMax／Nemotron／Jamba／gpt-oss 都已是 hybrid 架構——linear 層維護**固定大小 recurrent state** 而非線性成長的 KV cache，paged allocator／prefix caching／preemption 的設計全部要重想。這是把你和「只會背 PagedAttention 的人」區分開的一課。
**核心概念**：linear attention／Gated DeltaNet／Mamba2 的推論數學（recurrent state vs KV cache）、hybrid 模型的記憶體管理（state cache 與 paged KV 共存）、sliding window attention 交錯層的 kernel 與 cache 配置（每層不同 window）、**為何 Mamba/GDN/linear attention 的 state reuse 語意與 Transformer KV prefix caching 根本不同**（【v2.0 定稿精確化】vLLM 已有 hybrid KV-cache manager 與 Mamba/hybrid 的 prefix caching 支援，但 block size、命中條件與各 operator 的支援限制都不同——不是單純「失效」，要懂各自的語意與邊界）、vLLM 把 hybrid 列為一級公民的支援方式。
**動手做**：估算同參數量下 hybrid vs 全 attention 的記憶體／頻寬帳；讀 vLLM hybrid 模型的 block manager 處理。
**備註**：sm_75 可做（分析與 code reading 為主）。

### 課 9.35 — MoE 推論 Kernel 實戰：Grouped GEMM、Dispatch/Combine 與 DeepEP【v2.0 新增｜Core】
**前置**：9.26、6.20、6.21。
**學什麼**：9.26 的 MoE 是系統視角；這課下到 kernel——MoE 推論的三個熱路徑。
**核心概念**：**grouped GEMM**（變長 expert batch；cuBLAS 13 grouped GEMM／CUTLASS grouped kernel）、**dispatch/combine kernel**（token 排序、permute/unpermute、all-to-all）、**DeepEP**（高效能開源 EP 通訊庫，提供專用 dispatch/combine GPU kernel——【定稿更正】非「第一個」，Tutel／DeepSpeed-MoE 等更早已有開源 EP all-to-all 實作，DeepEP 以效能與 production 驗證著稱：NVLink＋RDMA 混合、prefill 高吞吐 vs decode 低延遲 kernel 分離、FP8 dispatch、hook-based 通訊計算 overlap；**【v2.0 定稿】版本遷移案例：V1 以 NVSHMEM 為底層（接 6.20）→ V2 改用 NCCL GIN／Device API（接 6.21）——比較兩代的 bootstrap、GPU-initiated networking、維護成本與部署條件**）、expert 負載平衡、wide-EP（接 13.13）。
**動手做**：實作一個簡化版 dispatch/combine kernel；讀 DeepEP 原始碼（V1 對照 6.20 NVSHMEM、V2 對照 6.21 NCCL GIN）畫出兩代通訊流的差異。
**備註**：⚠️ **分兩層**——簡化版 dispatch/combine kernel（縮小規模）**sm_75 可跑**；**DeepEP V2 runtime lab 有硬門檻：需 Hopper sm_90+（或支援 sm_90 PTX ISA 的架構）、PyTorch 2.10+、NCCL 2.30.4+、節點內 NVLink＋跨節點 RDMA**。⚠️ 附錄 B.6 的 PyTorch 2.12 wheel 路線是 NCCL 2.29.7（**不符** DeepEP V2），要跑 V2 得走 standalone NCCL 2.30.7 或官方 NCCL override。

### 課 9.36 — Production Kernel 案例研讀：FlashMLA、DeepGEMM 與 Machete【v2.0 新增｜Core】
**前置**：9.24、9.5、12.3（可並讀）。
**學什麼**：現存最好的 production kernel 閱讀教材——精煉、經真實 SLO 壓力驗證，涵蓋推論的三個關鍵路徑。
**核心概念**：**FlashMLA**（Hopper 上的 MLA decode kernel：H800 達 3000 GB/s／~660 TFLOPS——【v2.0 精確化】repo 現行文件數字、隨版本演進，**教材將 pin commit＋硬體＋shape＋CUDA 版本**再引數字；paged KV block size 64——9.24 MLA 概念的實作對照）、**DeepGEMM**（以「極簡核心」聞名的 FP8 GEMM 庫——初版核心約 300 行、**現已重構擴充、該數字僅具歷史意義**；JIT 編譯設計、fine-grained scaling、**在特定硬體/dtype/shape 下可勝過既有手調 kernel**的設計原因分析——與 12.3 NVRTC/JIT 直接呼應）、**Machete**（Hopper mixed-input W4A16 GEMM，接 9.5）、「從讀 code 到改 code」的方法論。
**動手做**：對三個 repo 各寫一頁「架構筆記」（資料流、關鍵 kernel、為什麼快）；挑一個小改動、實作並驗證。
**備註**：sm_75 可讀；實跑需 Hopper／雲端。

### 課 9.37 — VLM 多模態推論（＋生成模型選修）【v2.0 新增｜Core；DiT 部分 Elective】
**前置**：9.13、9.1。
**學什麼**：2026 年推論引擎職缺已把 VLM 列為必備——vision encoder 與 LLM 的混合 serving 有自己的一套規則。
**核心概念**：ViT/vision encoder 的推論特性（**compute-bound、無 KV cache**——與 LLM decode 剛好相反）、image token 的 embedding 插入與 chunked prefill 的交互、encoder/prefill/decode 三段 disaggregation（vLLM-Omni 思路）、多模態輸入的 batching 與前處理瓶頸（接 13.2/13.10）、視訊輸入的 frame 取樣；**選修**：diffusion/DiT 推論優化（step distillation、timestep/block cache）。
**動手做**：用 vLLM serve 一個小型 VLM，profile encoder vs decode 的資源占比；估算 image token 對 KV／TTFT 的影響。
**備註**：小模型 sm_75 勉強可玩；完整 lab 需大 VRAM。

### 課 9.38 — RL 後訓練基礎設施：Rollout 即推論【v2.0 新增｜Core（概觀級）】
**前置**：9.10、9.14。
**學什麼**：RL 後訓練（RLHF/RLVR）的 rollout 生成已是推論引擎的一級用例——vLLM 官方直接下場做 RL 框架。這是「推論工程師需要懂的那部分訓練」。
**核心概念**：RL pipeline 中 rollout 的角色與吞吐需求、colocated vs disaggregated rollout、weight resharding 與 **in-place 權重更新**（訓練引擎→推論引擎）、sleep/wake 模式、**train-inference 數值不一致**問題（importance sampling 修正）、verl／vime（vLLM 官方 RL 框架）／OpenRLHF 的架構對比。
**動手做**：讀 verl 的 rollout worker 與 weight sync 實作；畫出一個 RLVR 系統的資料流與資源時間線。
**備註**：sm_75 讀為主。不深入 RL 演算法本身（那是另一門課）；與 9.28 分工：9.28＝訓練技術、本課＝rollout 系統。

**📚 Part 9 參考**：FlashAttention（FA-1→4）/FlashInfer 論文｜Triton（OpenAI）官方 tutorials｜Triton Inference Server docs｜torch.compile / TorchInductor / AOTInductor / FlexAttention / TorchAO / torch.library 文件｜Transformer Engine docs｜vLLM（V1）/ SGLang（Multi-LoRA、MoE、disaggregation）/ TensorRT-LLM（含 plugin）/ llama.cpp / ONNX Runtime docs｜DeepSeek MLA、Medusa/EAGLE-3（speculative）、StreamingLLM/H2O（KV 淘汰）論文｜NVFP4 / ModelOpt 文件｜NVIDIA Dynamo / NIXL docs｜LMCache / Mooncake｜DeepSeek FlashMLA / DeepGEMM / DeepEP｜CuTe DSL（nvidia-cutlass-dsl）文件｜verl / vime / OpenRLHF｜CUDA MPS docs｜PMPP 5e LLM 章｜GPU MODE 進階講次。

---
---

# Part 10 — 進階主題與 Capstone 專案

> 目標：補上更底層/更系統的進階主題，並用一個整合專案把所學串起來。

### 課 10.1 — PTX 與 Inline Assembly
**學什麼**：必要時直接操作 PTX/SASS（你的 EDA 底子讓這變得有趣）。
**核心概念**：PTX ISA 基礎、`asm volatile` inline PTX、`ldmatrix`/`mma`/`cp.async` 指令、何時手寫 PTX、`nvdisasm` 看 SASS、register 配置觀察。
**動手做**：用 inline PTX 呼叫一個特殊指令。

### 課 10.2 — CUDA 與多執行緒 Host／叢集整合
**學什麼**：把 GPU 程式接進真實的工程系統（呼應你的 LSF/EDA 環境）。
**核心概念**：多 CPU thread 餵多 GPU、`cudaSetDevice`、per-thread default stream、MPI + CUDA、CUDA-aware MPI、與 job scheduler（LSF/Slurm）整合、GPU 資源配置。
**動手做**：多執行緒 host 驅動多 stream/多 GPU 的骨架。

### 課 10.3 — 多 GPU 程式設計細節
**學什麼**：單機多卡的低階控制。
**核心概念**：peer-to-peer access (`cudaDeviceEnablePeerAccess`)、unified virtual addressing、跨卡 memcpy、拓撲查詢 (`nvidia-smi topo`)、NVLink vs PCIe 路徑；**⚠️【v2.0.3】GeForce 的 PCIe P2P 已被驅動封殺**——消費級 GeForce（實測含 **RTX 4090／5090**）不支援 GPU-to-GPU P2P，`cudaDeviceEnablePeerAccess()` 回 `cudaErrorPeerAccessUnsupported`（各世代/組合以實測為準，別一概而論）；P2P 不可用時 **NCCL 通常改走 SHM/host memory 中轉、SHM 關閉時再退 network transport**，頻寬明顯下降。**注意 P2P 本身可走 PCIe 或 NVLink，取決於 GPU/拓撲/driver**——4090/5090 不支援，資料中心卡（DGX/HGX）走 NVLink；**【v2.0.5 更正】專業工作站卡自 Ada 世代起已移除 NVLink 橋接（NVLink 橋止於 Ampere 的 RTX A6000），Ada/Blackwell 專業卡只剩 PCIe P2P**——別對未來採購有錯誤期待，務必逐 GPU pair 用 `cudaDeviceCanAccessPeer()`／`nvidia-smi topo -p2p p/n` 實測，別假設「P2P＝一定要 NVLink」。
**動手做**：先試 `cudaDeviceEnablePeerAccess()`——在目標 RTX 4090/5090 上**觀察它回 `cudaErrorPeerAccessUnsupported`**、量測跨卡拷貝/NCCL 退化到 CPU shared-memory 路由時的頻寬懲罰；若手上有 NVLink 資料中心卡（雲端）再測真 P2P 頻寬，兩者對照。這才是消費級工作站真實的系統工程經驗。

### 課 10.4 — 主機端系統拓撲：NUMA、CPU 親和性與 PCIe Lane
**學什麼**：Host 端的擺放如何影響 GPU 的實際頻寬（雙 5090 無 NVLink 時尤其關鍵）。
**核心概念**：NUMA node 概念、`numactl`/`lscpu`/`nvidia-smi topo -m` 看拓撲、把 host thread 與 pinned memory 綁到「與該 GPU 同一 NUMA node」的 CPU（affinity）、CPU PCIe lane 分配（雙卡是否各 x16 還是被切成 x8/x8）、PCIe 代數與 H2D/D2H 頻寬上限；單 socket 工作站 NUMA 效應較小、但 PCIe 拓撲與 affinity 仍有感。
**動手做**：量測「綁對 vs 綁錯 NUMA node」對 H2D 頻寬的差異。
**備註**：因為消費卡走 PCIe（無 NVLink），這層優化在你的工作站上比在伺服器更值得做。

### 課 10.5 — GPUDirect Storage (GDS) 與 DMA
**學什麼**：讓資料從 NVMe 直接進 GPU，繞過 CPU bounce buffer。
**核心概念**：傳統路徑（NVMe→CPU RAM→GPU）的多餘拷貝、GPUDirect Storage 的 `cuFile` API（含 stream-ordered async I/O）、DMA 直送、何時有用（大資料集/大權重載入、I/O bound）、何時沒差（運算 bound）；GDS 能否走 full direct path 受 **GPU / driver / kernel / 檔案系統 / NVMe / PCIe 拓撲 / ACS / P2PDMA / `nvidia-fs.ko` / `cufile.json`** 影響。
**⚠️ 在 RTX 5090 workstation 上此為 optional lab**：GeForce 不一定支援完整 direct path（NVIDIA P2PDMA 典型支援清單是 A100/L40S/H100/GB100 等資料中心卡，GeForce 常只走 **compat mode**）。**實作前先跑 `gdscheck.py -p` 驗證平台**；若僅 compat mode，就改成「概念理解 + `cuFile` API walkthrough + direct-path vs CPU bounce-buffer 的 benchmark 設計」。
**動手做**：跑 `gdscheck.py -p`；視結果做 cuFile 直送對比，或做 benchmark 設計。

### 課 10.6 — 現代 C++ 與 CUDA 融合（核心）
**學什麼**：用 modern C++ 寫安全、可維護的 GPU 程式（展示資深架構設計能力）。
**核心概念**：RAII 包裝 device 記憶體（自製 `device_buffer<T>` 或用 thrust，杜絕 `cudaFree` 洩漏並顧及例外安全）、template kernel、`constexpr`/`if constexpr`、**C++20 Concepts 約束 template kernel 參數**（型別安全）、`__host__ __device__` 通用程式碼；**誠實界線**：device 端不支援例外，所以 `std::expected` 這類錯誤處理主要用於 **host 端**，device 端仍以 error-code/回傳值為主；最新 C++ 標準庫在 device 端的支援度依 nvcc 版本而異，使用前要確認。**⚠️【v2.0.2】排雷**：可用 C++20 的 Concepts／`constexpr`，但 **避開 module 化**：C++20 language modules 與 C++23 的 `import std;`（標準庫模組，C++23 才加入）在 nvcc＋CMake（Ninja module mapper）下相容性目前仍不成熟，一律維持傳統 `#include`，否則 build system 易崩。**【v2.0】擴充**：move-only resource 與 **stream-ordered lifetime**（資源生命週期綁 stream 而非 scope——GPU 工程特有的 RAII 變形）、compile-time architecture dispatch（`__CUDA_ARCH__`／`if constexpr`／policy template）、`std::expected`（C++23）實作 host 端 device-API 錯誤處理、**stdexec／senders-receivers 一瞥**（C++26 方向、NVIDIA 主導，但**尚無主流推論引擎生產採用——面試談資層級，不獨立成課**）；**C++ 基準（v2.0 修正）**：可攜性基準採 C++20，CUDA 13.3 起選定 device-side C++23 功能可用，但 CCCL 正式測試 dialect 仍以 C++17/20 為主——逐項驗證後再用，別概括成「C++23 只能 host」。
**動手做**：寫一組 RAII + Concepts 約束的 CUDA C++ 工具類。

### 課 10.7 — mdspan 多維張量抽象與型別安全
**學什麼**：用 `mdspan` 取代易錯的手動 1D 索引計算。
**核心概念**：`cuda::std::mdspan`（由 libcu++/CCCL 提供，**可在 device code 使用**）、layout（row/col-major、strided）、用 mdspan 表達 tensor/矩陣讓索引不再手算 `row*W+col`、與 kernel template 結合、extents 與邊界；對比手動索引在可讀性與 bug 率上的差異。
**動手做**：把 Part 5 某個 2D kernel 改用 `cuda::std::mdspan` 重寫。
**延伸**：libcu++ (CCCL) 文件。

### 課 10.8 — 效能可攜性與多架構部署
**學什麼**：讓同一份程式在 4090 與 5090（及未來架構）都跑得好。
**核心概念**：fatbin 打包多 `-gencode`、PTX 前向相容 JIT、架構特化的 tile 參數、runtime 查 CC 選 kernel、CI 中測多架構；**baseline vs `a`（架構專屬）vs `f`（family 專屬）編譯 target 的取捨**（複習課 1.4）——例如要在 RTX 50 + GB10 通用就用 `compute_120f`、要在 B200 + B300 通用就用 `compute_100f`，但 10.x 與 12.x family **不能**共用同一 binary，跨 family 部署得各編一份打進 fatbin。
**動手做**：建一個依 CC 自動選 kernel 變體的範例；用 `compute_120f` 編一份能同時跑 sm_120/sm_121 的 binary。

### 課 10.9 — 能耗、時脈與穩定性
**學什麼**：長時間跑 workload 的硬體層調校與監控。
**核心概念**：`nvidia-smi` 監控（功耗/溫度/時脈/利用率）、power limit、clock locking（profiling 用）、thermal throttling、**ECC 要分清楚**：GeForce RTX 50 的 **GDDR7 為 on-die ECC / SEC、永遠開啟、無軟體開關**（且只保護 die 內部、不保護匯流排傳輸），這**不等於**資料中心 GPU 那種 CUDA/NVML/DCGM 可見可管理的 ECC；**【v2.0】世代差異**：RTX 4090/3090 Ti 曾有的驅動層 VRAM soft-ECC 開關，**在 RTX 5090 上已移除**（counters、retired pages、Xid 事件、health policy 另在 Part 13 / 12.8 談）；量測可重現性。
**動手做**：鎖時脈做穩定 benchmark，監控長時間負載。

### 課 10.10 — CUDA 生態系廣度：圖形互通、其他語言與跨廠商可攜性
**前置**：Part 3、10.8。
**學什麼**：擴展視野——CUDA 不只 C++/AI，還有圖形互通、其他語言綁定、跨廠商可攜性。（**v1.8 起：本課是「跨後端可攜性實作軌」的入口**——全景與決策框架在這裡，動手移植與效能對齊在 10.14/10.15。）
**核心概念**：**圖形/計算互通**（CUDA 與 OpenGL / Vulkan / DirectX 共享 buffer,計算+渲染管線,用於即時視覺化/模擬）；**其他語言與模型**（CUDA Fortran、OpenACC 指令式平行、`nvc++ -stdpar=gpu` 的 **`std::execution::par / par_unseq`**【v2.0 更正名稱】——含 unified/managed memory mode、fallback 與演算法限制，對照課 6.14）；**跨廠商可攜性（⚠️ v1.9 更正定位）**：**Triton**＝可涵蓋 NVIDIA 與 AMD 後端的 kernel DSL，但各平台需**重新 autotune**（**可攜 ≠ 效能可攜**）；**HIP**＝CUDA-like 的 **AMD GPU** 程式模型，用於 CUDA→ROCm 移植（實作見 10.14）；**SYCL / oneAPI**＝另一種跨硬體 C++ 抽象（概念選修）；**cuTile Python / CUDA Tile C++＝僅屬 NVIDIA CUDA 生態**——它改善的是**跨 NVIDIA 架構**可攜性，**不是** AMD/ROCm 後端（前版誤把 cuTile 與 Triton 並列為跨廠商可攜層，此處更正）；何時該在意可攜性、移植成本。
**動手做**：擇一小試——CUDA-OpenGL interop 顯示一個 GPU 運算結果；或對一個 kernel 做 hipify **靜態轉換 + 共用 header／backend abstraction 設計**（本機只做原始碼層轉換與 review，**不把本機 NVIDIA 卡當作 HIP 移植成功的驗證**——編譯／執行／效能驗證見課 10.14 的雲端 MI300X lab）。

### 課 10.11 — 機密運算、虛擬化與安全部署
**前置**：1.9（13.8 叢集為背景，可後補——本課概念為主，不需先讀完 Part 13）。
**學什麼**：企業/雲端把 GPU 安全地切分與保護的機制（多為資料中心情境，概念為主）。
**核心概念**：**Confidential Computing / GPU TEE**（Hopper/Blackwell 的機密運算、TEE-I/O、加密的 CPU-GPU 資料路徑、保護模型權重與輸入）；**虛擬化**（vGPU 切分、與 MIG（9.20）的差異）；安全多租戶部署、attestation；**⚠️ 多需資料中心卡/平台，目標卡 5090/4090 無法實作，屬面試/系統設計知識**。
**動手做**：畫出一個機密推論服務的信任邊界（threat model）與資料保護點。

### 課 10.12 — Capstone 專案（擇一深做）
**學什麼**：把整套能力做成一個有份量的作品。
**選項 A**：**自訂 FlashAttention 風格 kernel**——從 naive attention 一路優化到 tiled + online softmax + Tensor Core，包成 PyTorch extension，對比官方 FA。
**選項 B**：**Mini LLM 推論引擎**——自己實作 KV cache + paged attention + continuous batching 的精簡版，服務一個小模型。
**選項 C**：**EDA × GPU 加速**——用 GPU 加速一個 EDA 相關的運算（如某種圖/矩陣/時序分析），發揮你的領域優勢。
**選項 D**：**cuTile vs 傳統 CUDA 對比研究**——用 cuTile（Python 或 C++，課 8.18/8.19）與手刻 CUDA C++ 各實作一個 fused attention 或 GEMM，比較程式碼量、效能、可維護性，封裝成 PyTorch extension。這在面試（尤其 NVIDIA 生態/推論引擎）很有說服力——同時展示「會用新抽象」與「懂底層」。
**選項 E**：**N-body / 粒子模擬**——物理模擬 + 即時視覺化（可結合課 10.10 圖形互通），經典且容易展示效能優化歷程。
**選項 F**：**影像處理管線**——濾鏡/卷積/直方圖串成 pipeline，用 Nsight 做完整優化報告（適合練 Part 5 + Part 8 + Part 13 資料管線）。
**選項 G**：**純 CUDA 訓練神經網路**——延伸課 9.29，在 MNIST 上手寫 MLP/CNN 的前向+反向+訓練迴圈，不靠框架。
**選項 H【v2.0.1｜sm_75 可交付】**：**分散式推論系統資料流研讀報告**——讀 DeepEP／NVIDIA Dynamo／NIXL 原始碼與 design docs，畫出 dispatch/combine 或 P/D 分離的完整資料流與通訊帳（不需多卡，純分析＋圖）。
**選項 I【v2.0.1｜sm_75 可交付】**：**mini-Llama 前向**——用課 9.0 的架構語彙＋課 9.29 的 kernel，手寫 embedding／RMSNorm／RoPE／SwiGLU／logits／sampling 串成一個能跑的小模型前向（筆電可跑縮小版）。
**選項 J【v2.0.1｜sm_75 可交付】**：**推論引擎 host 端瓶頸報告**——延伸課 10.16，對一個 serving 服務同時跑 Nsight Systems＋perf，定位並改善一個 host 側瓶頸（大部分在 CPU、筆電可做）。
**產出**：完整程式碼 + 優化日誌 + benchmark 報告 + README——可直接放進 GitHub 作為求職作品。

### 課 10.13 — 把 Capstone 寫成技術報告/部落格
**學什麼**：用工程寫作放大作品的影響力。
**核心概念**：roofline 與 before/after 圖表、清楚的 benchmark 方法、可重現性、技術敘事。對 Staff/Principal 面試，「能把複雜優化講清楚」本身就是核心能力。

### 課 10.14 — CUDA→HIP/ROCm 移植實戰（跨廠商可攜性 I）｜含雲端 lab【v1.8 新增】
**前置**：10.10、Part 5（手上要有自己寫的 kernel 當移植素材）、9.7（Triton 對照）。
**學什麼**：把你的 CUDA kernel 真正移到 AMD 世界——HIP 語言/工具鏈、自動轉換、以及兩家硬體心智模型的差異。
**核心概念**：**HIP＝面向 AMD GPU 的 C++ runtime API 與 kernel 程式模型**，API 刻意接近 CUDA，主要用途是把 CUDA 程式移植到 ROCm／AMD GPU。**⚠️ v1.9 重大更正**：前版寫「雙後端，可先在本機 NVIDIA 卡開發與驗證」是錯的——NVIDIA 後端雖存在（`HIP_PLATFORM=nvidia`，經 `hipother/hipnv` 轉呼叫 CUDA；HIP 7.0–7.2 版本化文件仍列此路徑），但 **NVIDIA 後端屬低優先維護（【v2.0 修正措辭】HIP 7.2 安裝文件仍列出 NVIDIA 平台、未移除；v1.9「已收斂為 AMD-only」說法過強）**、套件安裝有已知摩擦，**且最關鍵：它編出來的是 CUDA、跑的是 NVIDIA 卡，驗證不了 wavefront-64／LDS／MFMA／效能等任何 AMD 行為**。故**本機不作為 HIP runtime 正確性驗證平台**；`hipify-perl` / `hipify-clang` 自動轉換（cudaMalloc→hipMalloc 等 API 改名）與其極限（**非 drop-in**：架構專屬 intrinsics、warp-level primitives、Tensor Core 路徑需手工，依 HIP Porting Guide）；**心智模型差異**（⚠️ **本 lab 以 CDNA MI300X 為準**；AMD **RDNA** 遊戲卡 wavefront 多為 32，別一概而論成 64）：warp 32 ↔ **wavefront 64（CDNA）** 對 divergence/occupancy/shuffle 寬度的影響、shared memory ↔ **LDS**、SM ↔ **CU**、Tensor Core ↔ **Matrix Core (MFMA)**、**無 PTX 對應層**（AMD 直接編到 GCN/CDNA ISA code object，可攜策略與課 1.4 的 NVIDIA 模型不同）、`-arch=sm_120` ↔ `--offload-arch=gfx942`（MI300X）；函式庫對照（cuBLAS↔hipBLAS/rocBLAS、cuDNN↔MIOpen、NCCL↔RCCL、CUTLASS↔Composable Kernel、TensorRT↔MIGraphX）；工具對照（nvidia-smi↔amd-smi、Nsight Compute↔ROCm Compute Profiler〔原 Omniperf〕、cuda-gdb↔rocgdb）。
**動手做**：**① 本機（你的 NVIDIA 機器）——只做靜態層**：對課 5.2 的 kernel 跑 hipify，產出「自動轉換／無法轉換／需手工重寫」三欄清單；review 轉換後原始碼與 build system；設計 CUDA/HIP 共用 header、backend abstraction 與條件編譯；檢查 compile configuration 與 API mapping。**② 雲端 MI300X lab——編譯與執行驗證**：用 `hipcc` / `amdclang++ --offload-arch=gfx942` 編譯、執行、驗證正確性，再做 ROCm profiling 與第一版效能記錄。
**備註**：**所有 HIP 編譯／執行／profiling／正確性與效能驗證一律在雲端 MI300X 完成**（本機無 AMD 卡——與 MIG/NVSwitch 的處理一致）；本機只做靜態轉換與抽象層設計。術語橋接見課程 repo 的 `resources/NVIDIA_AMD_術語與概念對照表.md`（**【v2.0.5 更正】該檔已存在**；內容需按新知識同步：Triton sm_80+、DeepEP V2）。
**延伸**：HIP 文件與 HIP Porting Guide；HIPIFY；ROCm 文件。

### 課 10.15 — 跨後端效能對齊實戰（跨廠商可攜性 II）｜部分雲端 lab【v1.8 新增】
**前置**：10.14、9.7、Part 8（profiling 能力）。
**學什麼**：「能跑」之後的真功夫——同一個 kernel 在不同後端把效能**對齊**到各自硬體該有的水準，並建立可重複的對齊方法論。這正是市場上「跨 CUDA / ROCm / Triton 做 kernel 移植與效能對齊」職能的核心。
**核心概念**：**效能對齊方法論**：各平台先算 roofline 天花板（**課 8.1** 的 roofline／machine balance 方法，換上 MI300X 的規格——**【v2.0.5 更正】v2.0.4 誤指課 1.5，那是 5090 規格專題**）→ 量現況（Nsight Compute ↔ ROCm Compute Profiler 的對應 metrics）→ 差距歸因（wavefront 64 的 occupancy 差異、LDS bank、MFMA vs mma 路徑、coalescing 行為差異）→ 調參（block 大小取 64 的倍數、autotune 重跑）→ 迭代；**Triton 當可攜層的實測**：同一份 Triton kernel 在 NVIDIA（PTX）與 AMD（AMDGCN）後端的行為與重新 autotune；**三方對照總表**：同一 GEMM / softmax 的 CUDA C++ / HIP / Triton 在 5090、4090、MI300X（雲端）上的「達成峰值百分比」矩陣——這張表直接是作品集素材；**XLA / Pallas 概觀**（⚠️ 現況：JAX Pallas 的**主要 GPU 後端已轉向 Mosaic GPU、僅 Hopper+**；舊的 Triton backend **已 deprecated（JAX 官方 changelog 標示將移除）**、歷史上最低 Ampere，且 **`pallas.gpu` 已更名 `pallas.triton`**【v2.0.1】——**sm_75 兩條路都不能本機做，概念理解即可**；TPU 後端另為 Mosaic）。
**動手做**（**此課全程需 sm_80+ 卡，非 sm_75 筆電**）：① **有桌機後**：同一 kernel 的 Triton 版在 4090 vs 5090 做 autotune 對齊（延續 9.7）；② **雲端 lab**：把 10.14 的 HIP kernel 與 Triton 版在 MI300X 上 profile 並調到合理的峰值百分比，產出三方對照矩陣與「對齊報告」。**沒有雙卡/雲端前，本課只做方法論閱讀**。
**備註**：本課產出（對照矩陣 + 對齊報告）直接可放作品集 / 面試（接 11.1 / 11.2）。
**延伸**：ROCm Compute Profiler 文件；AMD Instinct MI300X workload tuning guide；JAX Pallas 文件。

### 課 10.16 — 推論引擎的 Host 端效能工程【v2.0 新增｜Core】
**前置**：9.2、9.13、10.2。
**學什麼**：2026 年單 GPU kernel 已高度優化，瓶頸大量移回 host——**特定實證研究（WukLab）顯示在其測試情境下** vLLM 的 CPU scheduling overhead 可佔總推論時間一半以上（非所有 workload 的固定比例，但方向性成立）。這是你 25 年 C++ 系統功力最大的差異化戰場。
**核心概念**：推論引擎的 host 解剖（process/thread 模型、asyncio event loop、tokenizer/detokenizer 執行緒、**overlap scheduler 設計**——vLLM V1／SGLang 的 near-zero CPU overhead 賣點）、**nanobind**（官方 microbenchmark 顯示呼叫 overhead 比 pybind11 低約一個數量級——**特定測試情境數字**；free-threading 一級支援）、**Python GIL 與 free-threading（3.13t/3.14t）**對 serving 的意義（vLLM 已在 3.14t 驗證可跑）、host 側 std::thread／lock-free 與 CUDA 的協作（以「讀懂 vLLM/SGLang scheduler」為目標，不教通用 lock-free 課）、診斷工具三件套：**Nsight Systems 找 launch gap ＋ perf/flamegraph 找 CPU 熱點 ＋ eBPF 概念**；**「Dynamo 的 host 層為何用 Rust」**——Staff 級系統設計論述素材（接 9.33）。
**動手做**：用 nanobind 綁一個 CUDA 元件並 benchmark 對比 pybind11；對一個 vLLM 服務同時跑 Nsight Systems 與 perf，定位一個 host 側瓶頸並提出改法。
**備註**：sm_75 完全可跑——這課大部分在 CPU 上。

**📚 Part 10 參考**：PTX ISA docs｜CUDA C++ Programming Guide（multi-device、UVA、graphics interop）｜CUDA-aware MPI 文件｜GPUDirect Storage / cuFile 文件｜libcu++ (CCCL) mdspan 文件｜ROCm/HIP 移植指南｜NVIDIA Confidential Computing / vGPU docs｜nanobind docs / perf / eBPF（v2.0）｜你的 capstone 對應領域論文。

---

# Part 11 — 職涯、面試與持續學習

> 目標：把學習轉成職涯成果，並接上能長期成長的社群與資源。
> **授課順序：最後上**——雖然編號是 Part 11，但依課程地圖與〈附錄 E〉，建議在 **Part 12（系統／JIT）、Part 13（生產／管線）之後**才上這一部分（Part 12/13 的技術正是 Staff/Principal 職涯的主要籌碼）。

### 課 11.1 — GPU/CUDA 面試準備地圖
**核心概念**：常見題型（記憶體階層、coalescing、occupancy、reduction 優化、attention、量化、多 GPU）、白板手寫 kernel、效能估算題（給頻寬/算力估 runtime）、系統設計題（設計一個推論服務）；對應你目標公司（NVIDIA/Anthropic/Synopsys/MediaTek）的側重。

### 課 11.2 — 作品集與開源貢獻
**核心概念**：把 capstone 做成可展示專案、貢獻 GPU MODE / Triton / vLLM / CUTLASS 等開源、參加 kernel 競賽（GPU MODE 有 kernel leaderboard/競賽）、寫技術文章建立 signal。

### 課 11.3 — 持續學習與社群
**核心概念**：GPU MODE（Discord + YouTube，最活躍的現代 GPU 社群）、NVIDIA GTC 演講、追 PMPP 更新、追 CUDA release notes、Tri Dao / NVIDIA 部落格、arXiv 的 MLSys/kernel 論文。建立每週固定的「讀 + 寫 kernel」節奏。

### 課 11.4 — 為下一步鋪路（你的 18 個月 roadmap 對接）
**核心概念**：把這份 CUDA 課程嵌進你既有的 AI/ML 18 個月 roadmap；CUDA 與 ML 理論（李宏毅/林軒田）、PyTorch、推論引擎四者的時間配置；何時該停止「學」、開始「做專案 + 投履歷」。

---
---

# Part 12 — CUDA 系統程式設計與 Driver / JIT Runtime

> 目標：拿到 GPU 的**底層系統控制力**——這是「會寫 kernel」進化到「能打造推論引擎/框架/動態 kernel 系統」的關鍵。TensorRT plugin、Triton、PyTorch extension、動態 codegen 全都建立在這層。
> **學習順序**：建議在 Part 11 之前學（見課程地圖）。你的 25 年系統程式經驗在這裡非常吃香。

### 課 12.1 — Runtime API vs Driver API
**前置**：Part 3。
**學什麼**：兩套 API 的分界，以及何時非用 Driver API 不可。
**核心概念**：CUDA Runtime API（`cudaXxx`，高階、隱含 context）vs Driver API（`cuXxx`，低階、顯式控制）；`CUcontext`、primary context、初始化模型、兩者混用；**【v2.0】`cuGetProcAddress` 與版本化 entry point**（ABI-safe 的 driver API 動態載入——寫 loader/框架的必備）；CUDA Python 1.0（green contexts、process checkpointing）作為 Python 端對應。
**動手做**：用 Driver API 初始化、取得 device、建立 context 跑一個 kernel。

### 課 12.2 — Context、Module 管理與動態 kernel 啟動
**前置**：12.1、3.9。
**學什麼**：手動載入與啟動 kernel（不靠 `<<<>>>`）。
**核心概念**：module loading（載入 PTX/cubin/fatbin）、`cuModuleLoad`/`cuModuleGetFunction`、`cuLaunchKernel`（手動傳參與 grid/block 配置）、為何 plugin/外掛系統需要這種動態啟動。
**動手做**：把一個 kernel 編成 PTX，runtime 載入並用 `cuLaunchKernel` 啟動。

### 課 12.3 — NVRTC：執行期編譯 CUDA C++
**前置**：12.2。
**學什麼**：在程式執行時把 CUDA C++ 字串編成可執行 kernel（動態 codegen 的核心）。
**核心概念**：NVRTC API、runtime compilation、把生成的 PTX 交給 Driver API 載入、應用場景（依輸入動態產生 kernel、JIT 特化、框架 codegen）；與 Triton/Inductor 的 JIT 概念對照。
**動手做**：用 NVRTC 在 runtime 編譯一段 kernel 字串並執行。

### 課 12.4 — JIT 工具鏈：nvJitLink / nvPTXCompiler / nvFatbin
**前置**：12.3、3.9。
**學什麼**：JIT 連結與 PTX 編譯的底層元件。
**核心概念**：`nvJitLink`（runtime device linking，連結多個 PTX/LTO-IR）、`nvPTXCompiler`（PTX→cubin）、`nvFatbin`（組 fatbin）；**libNVVM / NVVM IR**（基於 LLVM IR 的 device 編譯入口，給寫自訂語言/編譯器後端的人）；**呼應 9.4 的 PTX compiler 缺件雷**（Linux 是靜態庫 `libnvptxcompiler_static.a`，非 `.so`）——這層出問題就會壞掉 JIT extension。
**動手做**：用 nvJitLink 在 runtime 連結兩段 device 程式碼。

### 課 12.5 — CUDA IPC：跨 process 共享 device 記憶體
**前置**：8.10、12.1。
**學什麼**：讓多個 process 共用同一塊 GPU 記憶體（多 process 推論、MPS 場景的基礎）。
**核心概念**：`cudaIpcGetMemHandle`/`cudaIpcOpenMemHandle`、IPC event、跨 process 零拷貝、安全/生命週期注意；與課 9.20 MPS、Part 13 多租戶的關係。
**動手做**：兩個 process 透過 IPC handle 共享一塊 device buffer。

### 課 12.6 — Dynamic Parallelism：kernel 內啟動 kernel
**前置**：Part 4。
**學什麼**：device 端直接啟動子 kernel（適合遞迴、不規則、自適應工作量）。
**核心概念**：**【v2.0】一律以 CDP2 為準**——CUDA 12+ 的 Dynamic Parallelism 是第二代（CDP1 已 deprecated/移除；CC 9.0+ 只支援 CDP2）：`cudaStreamFireAndForget`／`cudaStreamTailLaunch` 的 tail-launch 語意、**device 端已不能同步等待子 kernel 完成**（舊教材的 in-kernel `cudaDeviceSynchronize` 已移除——照舊教材寫會編不過）；device-side kernel launch、巢狀 grid、資源限制、overhead 取捨、適用/不適用場景（與 persistent kernel 8.16 比較）；⚠️ **MPS 下不支援 dynamic parallelism**（接 9.20）。
**動手做**：用 dynamic parallelism 實作一個自適應細分的運算。

### 課 12.7 — CUPTI：自製 Profiling / Tracing 工具
**前置**：Part 8。
**學什麼**：用 CUPTI 程式化收集效能資料（Nsight 底層用的就是它）。
**核心概念**：CUPTI Activity/Callback API、追蹤 kernel/memcpy/API、自建 profiler 或把 GPU 事件接進你的監控、NVTX 標記的程式化使用；與 Nsight 的關係。
**動手做**：用 CUPTI 攔截並記錄一段程式的 kernel 執行時間。

### 課 12.8 — NVML / DCGM：程式化監控與管理 GPU
**前置**：10.9。
**學什麼**：用 API（而非只 `nvidia-smi`）監控/管理 GPU，給生產環境用。
**核心概念**：NVML（query 利用率/溫度/功耗/時脈/ECC、設 power limit）、DCGM（資料中心級健康監控、診斷、telemetry）、把 GPU 指標接進 Prometheus/Grafana；與 Part 13 observability 串接。
**動手做**：用 NVML 寫一個小工具輪詢並輸出 GPU 健康指標。

### 課 12.9 — CUDA Python 1.0：cuda.core / cuda.bindings / device primitives
**前置**：12.1、9.2。
**學什麼**：CUDA 13.3 釋出的 **CUDA Python 1.0**（semantic versioning 穩定 API）——用 Python 直接掌控 CUDA，不必落到 C++。
**核心概念**：`cuda.core`（Pythonic 的 device/stream/event/module 物件、執行 kernel）、`cuda.bindings`（低階 driver/runtime/NVRTC/NVVM 綁定）、**`cuda.compute`（host-callable 的 CCCL 平行演算法：sort/scan/reduce/transform）**、**`cuda.coop`（供 Numba CUDA kernel 使用的 device-callable CCCL block/warp primitives**（【v2.0.5】注意 `cuda-python` 本體是 host-side 綁定，這類 device-callable 介面在 Python 端是給 JIT 前端（Numba／`cuda.lang`）解析用的 stub、不能單獨在 host 執行）**，目前 experimental——【v2.0】import 路徑已移至 `cuda.coop._experimental`，示範程式碼要留意）**、`cuda.pathfinder`（函式庫探索）、**【v2.0】`cuda.lang`**（Python SIMT kernel DSL——CUDA 13.3 官方 Programming Guide 已明列，與 Numba 整合過渡中；⚠️ Experimental、快速演進。v1.9 曾因查無實據移除，v2.0 依官方文件重新加入，這個生態不到一年翻一輪、以當期文件為準）、green contexts、process checkpointing；**新開發主線**：numba-cuda-mlir（獨立套件；**傳統 numba-cuda 已進入 maintenance mode**，見 6.19）；與 PyTorch、Numba、cuTile Python 的關係；何時用 CUDA Python vs C++ Driver API。
**動手做**：用 cuda.core 在 Python 端載入並啟動一個 kernel（對照 12.2 的 C++ 版）。
**備註**：CUDA Python 1.0 是 CUDA 13.x 週期的重要里程碑，但**實際可用性與 wheel 相容性以 `cuda-python` 官方 release notes 與 PyPI wheel 標註為準**（不同平台/CUDA 版本的 wheel 可能不同）——和 9.4 的 PyTorch 一樣，安裝後實測再用。
**延伸**：CUDA Python 1.0 docs。

**📚 Part 12 參考**：CUDA Driver API docs｜NVRTC docs｜nvJitLink / nvPTXCompiler / nvFatbin / libNVVM docs｜CUDA IPC（Programming Guide）｜CUPTI docs｜NVML / DCGM docs｜CUDA Python 1.0 docs（cuda.core / cuda.bindings）。

---
---

# Part 13 — AI CUDA Production 系統與資料管線

> 目標：把推論引擎從「能跑」推到「能上線營運」——資料管線、部署、多租戶、可觀測性。AI CUDA 不只是 kernel；資料從磁碟/網路一路到 GPU 的整條路徑、以及上線後的運維，都是 Staff/Principal 的核心職責。
> **學習順序**：建議在 Part 11 之前學（見課程地圖）。其中 NVLink/NVSwitch/MIG/RDMA 等資料中心特性你本機跑不了，會明確標為「概念為主」。

## 13-A 資料管線與 I/O

### 課 13.1 — 資料管線總覽與瓶頸定位
**前置**：8.2、8.10。
**學什麼**：看清「資料到 GPU」整條路徑，哪裡會卡。
**核心概念**：磁碟/網路 → CPU decode/preprocess → H2D → GPU 的端到端路徑；常見瓶頸（I/O bound、CPU preprocess bound、PCIe bound）、GPU 餓死（starvation）、用 Nsight Systems 看 GPU idle 是否因餵不夠。
**圖解**：端到端資料管線與各段瓶頸。

### 課 13.2 — DALI：GPU 資料載入與前處理管線
**前置**：13.1。
**學什麼**：用 DALI 把資料載入/增強搬到 GPU，餵飽訓練/推論。
**核心概念**：NVIDIA DALI pipeline、把 decode/resize/normalize/augment 放 GPU、與 PyTorch/TF DataLoader 整合、prefetch、消除 CPU 前處理瓶頸。
**動手做**：用 DALI 建一個影像前處理 pipeline 餵給模型。

### 課 13.3 — GPU 影像編解碼：nvImageCodec / nvJPEG2000 / nvTIFF
**前置**：13.1（nvJPEG 已在 6.11）。
**學什麼**：在 GPU 上做影像編解碼（CV / 醫療影像 / 大圖管線）。
**核心概念**：nvImageCodec（統一介面）、nvJPEG2000、nvTIFF、硬體解碼器、與 DALI / 資料管線整合、大影像（醫療/衛星）場景。
**動手做**：用 nvImageCodec 批次解碼一批影像到 GPU。

### 課 13.4 — nvCOMP：GPU 壓縮 / 解壓
**前置**：13.1。
**學什麼**：在 GPU 上壓縮/解壓資料（省頻寬、KV cache/資料集壓縮）。
**核心概念**：nvCOMP 演算法（LZ4/Snappy/Deflate/Cascaded 等）、GPU 直接解壓、應用（壓縮資料集減 I/O、壓縮 KV cache/activation 省 VRAM）、壓縮率 vs 速度取捨。
**動手做**：用 nvCOMP 壓縮/解壓一塊資料，量測吞吐與壓縮率。

### 課 13.5 — Pinned / Async Staging / Zero-copy 管線（pipeline 視角）
**前置**：8.8、8.10。
**學什麼**：把記憶體傳輸做成不擋路的 pipeline（延續 8.10，從管線角度看）。
**核心概念**：pinned memory pool、double-buffered staging、`cudaMemcpyAsync` + stream 重疊、zero-copy 的適用與陷阱、與 GDS（10.5）的取捨、對 tail latency 的影響。
**動手做**：把資料載入做成 double-buffered async staging，量測 GPU 利用率提升。

### 課 13.6 — DLPack 與 `__cuda_array_interface__`：框架間零拷貝張量交換
**前置**：9.2、10.7。
**學什麼**：在 PyTorch / CuPy / TensorRT / 你的 kernel 之間**不複製**地共享 GPU 張量。
**核心概念**：DLPack 標準（`from_dlpack`/`to_dlpack`）、`__cuda_array_interface__`（CuPy/Numba）、零拷貝互通、CCCL 3.3 強化的 DLPack/mdspan 互通（接課 10.7）、避免不必要的 device-to-device 拷貝。
**動手做**：用 DLPack 把 PyTorch tensor 零拷貝交給 CuPy 處理再交回。

## 13-B 部署、多租戶與運維

### 課 13.7 — 生產級 GPU 拓撲：NVLink / NVSwitch / Fabric Manager / GPUDirect RDMA
**前置**：10.3、1.9。
**學什麼**：資料中心多卡/多節點的互連與直送（概念為主）。
**核心概念**：NVLink（卡間高速）、NVSwitch（全互連 fabric）、Fabric Manager（管理 NVSwitch 拓撲）、GPUDirect RDMA（網卡 → GPU 直送，跨節點分散式）、**【v2.0】EGM（Extended GPU Memory）**——**【v2.0.5 更正】非 CUDA 13.3 新增**：概念與早期軟體支援始於 Grace Hopper（GH200）／CUDA 12.x（讓 GPU 經 C2C 存取 Grace 的 LPDDR），13.x 續有擴展——跨 superchip 的記憶體池化概念；**rack-scale（NVL72）拓撲深化獨立成課 13.13**；**⚠️ 這些多為資料中心卡/DGX 才有，目標卡 5090/4090 工作站跑不了，但面試與系統設計要懂**（呼應課 1.9）。
**圖解**：單機 PCIe vs NVLink vs NVSwitch fabric 拓撲對比。

### 課 13.8 — 叢集與排程：Kubernetes GPU Operator、Slurm / LSF
**前置**：10.2、9.20。
**學什麼**：在叢集上配置與排程 GPU 工作（接你的 LSF 背景）。
**核心概念**：NVIDIA GPU Operator（K8s 上自動裝驅動/runtime/監控、device plugin、time-slicing/MPS/MIG 配置）、Slurm GPU 排程（gres）、**LSF GPU 排程（你熟的環境）**、資源配額與 QoS；**【v2.0】GPU checkpoint/restore**：`cuda-checkpoint`＋CRIU（device state dump → 異節點還原）、**Dynamo Snapshot**（K8s 推論 worker 冷啟動從分鐘級壓到秒級；⚠️ **experimental／limited preview**，有 privileged DaemonSet 等部署限制）——elastic serving／搶佔式排程的關鍵新能力。
**動手做**：寫一個 K8s GPU workload 的 manifest（或 Slurm/LSF GPU job 腳本）。
**備註**：你 EDA 的 LSF 經驗在這裡直接遷移成 AI 叢集排程能力。

### 課 13.9 — Serving 可觀測性（Observability）、可靠性與 SLO
**前置**：9.13、12.7、12.8。
**學什麼**：推論服務上線後怎麼「看得見、守得住」。
**核心概念**：metrics（吞吐/延遲/GPU 利用率/KV cache 佔用/佇列長度）、tracing、SLO/SLA（TTFT、ITL、p99 tail latency）、用 DCGM/NVML（12.8）+ Triton metrics（9.21）+ Prometheus/Grafana、告警、容量規劃；把效能回歸（7.12）延伸到線上監控；**可靠性面**：Xid error log 解讀、persistence mode、driver reset/recovery、thermal/power throttling 監控、ECC 事件、多租戶下的 rate limiting / quota、CUDA Error Log Management（`CUDA_LOG_FILE`，接 7.5）；**【v2.0】fleet 級效能調查**：用 telemetry/SQL 量化整個機隊的實際效能與 roofline 差距、tail latency 歸因、autoscaling/routing 效率——單機 profiling 之上的 Staff 級視角（Anthropic Performance Engineer 職缺的核心能力）。
**動手做**：為一個推論服務設計一組監控指標與 SLO dashboard。

### 課 13.10 — 視訊 AI 管線：NVENC / NVDEC / Video Codec SDK / PyNvVideoCodec【v2.0 正名】
**前置**：13.1、13.3。
**學什麼**：在 GPU 上解碼/編碼視訊，餵給視覺模型（video AI / 串流分析必備）。
**核心概念**：硬體編解碼器 NVDEC（解）/ NVENC（編）、Video Codec SDK 與 **PyNvVideoCodec**（Python SDK 的正式產品名稱）、解碼後直接留在 GPU（零拷貝接前處理）、與 DALI 整合、batch、video → frame → 模型的 pipeline、即時串流場景。
**動手做**：用 NVDEC 解碼一段影片，frame 直送 GPU 前處理 + 推論。

### 課 13.11 — LLM 權重載入與 tokenization 管線
**前置**：13.1、13.5。
**學什麼**：把「載入模型 + 餵 token」這條常被忽略、卻很影響冷啟動與延遲的路徑做好。
**核心概念**：**權重載入**（safetensors、mmap 記憶體映射、lazy/分片載入、與 GDS 10.5 搭配直送 GPU、量化權重載入）、**model warmup**（先跑幾次暖機、CUDA graph 捕捉）；**tokenization 管線**（CPU tokenizer 瓶頸、批次化、與 prefill 重疊）、batch collation、padding/packing；**【v2.0】streaming 正確性**：incremental detokenization（UTF-8 邊界、stop string 判定）、長 prompt（50K+ token 的 agent prompt）下 tokenization 佔 TTFT 的比例與平行化。
**動手做**：用 safetensors + mmap 載入權重並量測冷啟動；把 tokenization 與推論重疊。

### 課 13.12 — Serving 基準測試與模型生命週期
**前置**：9.21、7.12、13.9。
**學什麼**：用對的工具量 serving 效能,並管理模型上線的生命週期。
**核心概念**：**【v2.0】AIPerf（NVIDIA 新一代 client-side benchmark 工具，本課主力）＋ Triton Perf Analyzer / Model Analyzer / GenAI-Perf（GenAI-Perf 已凍結新功能、列為輔助）**（吞吐-延遲曲線、找最佳 batch/併發、LLM 專用指標如 TTFT/ITL/TPS）、MLPerf Inference 概念；**【v2.0 定稿】benchmark 方法學**：open-loop vs closed-loop、Poisson/burst arrival、trace replay、warm-up、client 端瓶頸排除、**goodput 與 SLO attainment**（比 raw throughput 更誠實的指標）、p50/p95/p99、可重現的 benchmark report 格式；**模型生命週期**：版本管理、canary / A-B rollout、藍綠部署、回滾、shadow traffic；與 7.12 效能回歸、13.9 監控串成完整 ops 流程。
**動手做**：用 AIPerf（或 GenAI-Perf/Perf Analyzer）對一個 LLM 服務跑吞吐-延遲基準，畫出曲線。

### 課 13.13 — Rack-scale 推論：GB200 NVL72、MNNVL 與 Wide-EP【v2.0 新增｜Reference（概念）】
**前置**：13.7、9.14、9.35。
**學什麼**：「72 GPU 是一個 NVLink domain」如何徹底改變 parallelism 設計——面試與容量規劃必懂，本機跑不了。
**核心概念**：NVL72 拓撲（36 Grace＋72 Blackwell；NVLink 5：單 GPU 1.8 TB/s、全機架 **130 TB/s aggregate NVLink 頻寬**【v2.0 精確化：NVIDIA 官方用語，非 bisection；拓撲特性另述】）、Multi-Node NVLink（MNNVL）與 IMEX channel／驅動設定、NVLink SHARP in-network reduction、**wide expert parallelism**（EP 直接橫跨 72 GPU；GB200＋Dynamo 對 MoE 的加速）、KV 傳輸走 NVLink 而非 IB 的設計差異、與「8-GPU node」世界觀的對比（TP/EP/PP 配比全變）、Grace C2C 與兩-NUMA-node 記憶體模型（接 3.8/10.4）、GB300／Vera Rubin 前瞻。
**動手做**：給定一個 MoE 模型規格，分別在「8×H100 node」與「NVL72」設計 parallelism mapping，比較通訊帳與 KV 傳輸路徑。
**備註**：⚠️ 純概念課（雲端也難摸到整機架）；與 1.9、13.7 呼應。

**📚 Part 13 參考**：DALI docs｜nvImageCodec / nvJPEG2000 / nvTIFF｜nvCOMP docs｜NVENC/NVDEC（Video Codec SDK）/ PyNvVideoCodec｜safetensors｜DLPack + `__cuda_array_interface__`｜GPUDirect RDMA / NVSwitch / Fabric Manager / NVL72（MNNVL/IMEX）docs｜NVIDIA GPU Operator（K8s）｜cuda-checkpoint / CRIU / Dynamo Snapshot（v2.0）｜Slurm / LSF GPU 排程｜AIPerf / Triton Perf Analyzer / Model Analyzer / GenAI-Perf｜DCGM。

---
---

# 附錄 A — 主參考資料總表

> 凡可公開取得者皆附連結。書籍以正版購買為準（我只列出處，不提供盜版）。

## A.1 經典教材（書）
- **Programming Massively Parallel Processors (PMPP), 5th Edition** — Hwu, Kirk, El Hajj（**【v2.0.2 更正】2026 年出版（Elsevier 頁面同時列「5th Edition – 2026-02-27」與「Published: 2026-06-03」兩個日期，以官方頁面為準）**——原單一「2026-02」不夠準確；新增 LLM、filtering、wavefront、進階 GEMM、warp-level、cooperative groups、NCCL/NVSHMEM 章）。**本課程主教材**。
  - 出版頁：`https://shop.elsevier.com/books/programming-massively-parallel-processors/hwu/978-0-443-43900-1`
- **Professional CUDA C Programming** — Cheng, Grossman, McKercher（偏實務、優化，適合你）。
- **The CUDA Handbook, 2nd Ed.** — Nicholas Wilt（API 與底層細節參考書）。
- **CUDA by Example** — Sanders & Kandrot（入門經典，較舊但觀念清楚）。

## A.2 NVIDIA 官方文件（最權威、會更新）
- CUDA C++ Programming Guide：`https://docs.nvidia.com/cuda/cuda-programming-guide/`（**【v2.0.5】官方已改版路徑**；舊的 `cuda-c-programming-guide/` 仍可轉址）
- CUDA C++ Best Practices Guide（優化聖經）：`https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/`
- CUDA Toolkit 下載：`https://developer.nvidia.com/cuda-downloads`（目前 13.3）
- CUDA Toolkit Archive（歷史版本）：`https://developer.nvidia.com/cuda-toolkit-archive`
- CUDA Installation Guide for Linux：`https://docs.nvidia.com/cuda/cuda-installation-guide-linux/`
- Nsight Compute（含 Profiling Guide）：`https://docs.nvidia.com/nsight-compute/`
- Nsight Systems：`https://developer.nvidia.com/nsight-systems`
- Compute Sanitizer：`https://docs.nvidia.com/compute-sanitizer/`
- CUDA Samples：`https://github.com/NVIDIA/cuda-samples`
- CUDA Library Samples：`https://github.com/NVIDIA/CUDALibrarySamples`
- CUDA 13.x Release Notes（版本/元件表）：`https://docs.nvidia.com/cuda/cuda-toolkit-release-notes/`
- CUDA GPU Compute Capability（sm_xx 對照，5090=12.0）：`https://developer.nvidia.com/cuda/gpus`
- CUDA Driver API：`https://docs.nvidia.com/cuda/cuda-driver-api/`
- NVRTC（runtime compilation）：`https://docs.nvidia.com/cuda/nvrtc/`
- CUPTI（profiling API）：`https://docs.nvidia.com/cupti/`
- NVML：`https://docs.nvidia.com/deploy/nvml-api/`｜DCGM：`https://docs.nvidia.com/datacenter/dcgm/`
- CUDA Tile（C++ / IR / Python）：`https://developer.nvidia.com/cuda/tile`｜IR：`https://github.com/NVIDIA/cuda-tile`｜cuTile Python：`https://github.com/NVIDIA/cutile-python`、docs `https://docs.nvidia.com/cuda/cutile-python/`｜教學：`https://github.com/NVIDIA/TileGym`、accelerated-computing-hub 的 cuda-tile tutorial
- CCCL（Thrust/CUB/libcu++，含 mdspan/DLPack）：`https://github.com/NVIDIA/cccl`

## A.3 硬體白皮書 / 規格
- **NVIDIA RTX Blackwell GPU Architecture 白皮書**（5090 內部架構）：
  `https://images.nvidia.com/aem-dam/Solutions/geforce/blackwell/nvidia-rtx-blackwell-gpu-architecture.pdf`
- sm_120 / Blackwell 編譯支援討論：NVIDIA Developer Forums「CUDA Toolkit 12.8 what GPU is sm_120」。
- RTX 5090 LLM 規格與 VRAM 對應參考（Spheron / RunPod / Vast 的 5090 指南）。

## A.4 線上課程 / 講座
- **NVIDIA DLI — Fundamentals of Accelerated Computing with CUDA C/C++**（官方、profile-driven、附證書）：
  `https://learn.nvidia.com/`（搜課名）
- **NVIDIA DLI — CUDA Python（Numba）** 版本（若你想先用 Python 起步）。
- **GPU MODE（前 CUDA MODE）** — 最推薦的現代社群課程：
  - YouTube：`https://www.youtube.com/@GPUMODE`
  - 資源整理：`https://github.com/gpu-mode/resource-stream`
  - 講義/程式：`https://github.com/gpu-mode/lectures`
  - Christian Mills 的詳盡筆記：`https://christianjmills.com/series/notes/cuda-mode-notes.html`
- **Oxford CUDA 課程（Mike Giles）**：`https://people.maths.ox.ac.uk/gilesm/cuda/`
- **Caltech CS 179: GPU Programming**：`https://courses.cms.caltech.edu/cs179/`
- **freeCodeCamp CUDA Course（Elliot / Infatoshi）**：`https://github.com/Infatoshi/cuda-course`（YouTube 有完整影片）
- **NVIDIA Accelerated Computing Hub**：`https://github.com/NVIDIA/accelerated-computing-hub`
- **Mojo GPU Puzzles**（觀念通用、互動式）：`https://puzzles.modular.com/`

## A.5 AI Kernel / 推論引擎（GitHub + 論文）
- CUTLASS / CuTe：`https://github.com/NVIDIA/cutlass`
- FlashAttention：`https://github.com/Dao-AILab/flash-attention`；FA-2 用 CUTLASS 論文 `arXiv:2312.11918`
- FlashInfer：`https://github.com/flashinfer-ai/flashinfer`（論文 `arXiv:2501.01005`）
- Triton（OpenAI）：`https://github.com/triton-lang/triton` + 官方 tutorials
- HIP（AMD ROCm 的 CUDA-like C++ 程式模型，**面向 AMD GPU**；`HIP_PLATFORM=nvidia` 的 NVIDIA 後端存在但屬**低優先維護**／轉呼叫 CUDA，**不能當作 AMD 行為的驗證平台**——見課 10.14）：`https://rocm.docs.amd.com/projects/HIP/en/latest/`（含 HIP Porting Guide）【v1.8；定位 v1.9/v2.0 兩度修正】
- HIPIFY（CUDA→HIP 自動轉換工具）：`https://github.com/ROCm/HIPIFY`【v1.8】
- ROCm 文件入口（rocBLAS/MIOpen/RCCL/ROCm Compute Profiler 等）：`https://rocm.docs.amd.com/`【v1.8】
- 《NVIDIA↔AMD 術語與概念對照表》：本課程 `resources/NVIDIA_AMD_術語與概念對照表.md`【v1.8 新增；**v2.0.5 更正：已存在**】
- TensorRT-LLM：`https://github.com/NVIDIA/TensorRT-LLM`
- vLLM：`https://github.com/vllm-project/vllm`
- SGLang：`https://github.com/sgl-project/sglang`
- llama.cpp：`https://github.com/ggml-org/llama.cpp`
- ONNX Runtime：`https://github.com/microsoft/onnxruntime`
- Triton Inference Server（serving 平台，≠ 上面的 OpenAI Triton）：`https://github.com/triton-inference-server/server`
- ThunderKittens：`https://github.com/HazyResearch/ThunderKittens`

## A.6 中文資源（輔助）
- 台大計中「NVIDIA CUDA GPU 高速平行運算程式開發」課程介紹。
- 「譚升的博客 / CUDA 编程」、知乎「CUDA 编程入门极简教程」（簡中、概念清楚）。
- 灣區筆記 Bay Area Notes「CUDA 教學：6 步驟入門」、都會阿嬤「CUDA 教學」系列（繁中）。
> 中文資源適合補概念，但**最前沿、最完整的仍是英文官方文件與論文**；建議以英文為主、中文為輔。

## A.7 進階函式庫 / 資料管線 / 生產工具（v1.3 新增）
- 數學：cuTENSOR `https://docs.nvidia.com/cuda/cutensor/`｜MathDx（cuBLASDx/cuFFTDx/cuSOLVERDx/cuRANDDx）`https://docs.nvidia.com/cuda/mathdx/`
- 稀疏：cuSPARSELt（2:4 結構化稀疏）`https://docs.nvidia.com/cuda/cusparselt/`｜cuDSS（直接稀疏求解）`https://docs.nvidia.com/cuda/cudss/`
- 資料管線：DALI `https://docs.nvidia.com/deeplearning/dali/`｜nvCOMP `https://docs.nvidia.com/cuda/nvcomp/`｜nvImageCodec `https://docs.nvidia.com/cuda/nvimagecodec/`
- 互通：DLPack `https://github.com/dmlc/dlpack`（文件 `https://dmlc.github.io/dlpack/latest/`）
- 框架編譯器：torch.compile / TorchInductor / AOTInductor（PyTorch docs）｜Transformer Engine（FP8）`https://github.com/NVIDIA/TransformerEngine`
- 視訊：NVENC/NVDEC（Video Codec SDK）｜PyNvVideoCodec
- 資料科學：RAPIDS cuDF/cuML/cuGraph/cuVS `https://rapids.ai/`
- 權重格式：safetensors `https://github.com/huggingface/safetensors`
- Serving 基準：**AIPerf（主線）**／Triton Perf Analyzer / Model Analyzer / GenAI-Perf（已凍結新功能，輔助）
- 部署：NVIDIA GPU Operator（K8s）`https://docs.nvidia.com/datacenter/cloud-native/`
- PyTorch 安裝矩陣（依官方 install matrix 選擇與 driver/GPU 相容的 wheel；**本課程基準環境 cu130**，cu128 於 2.12 標記 deprecated 且移出標準 release matrix、2.13 明列移除——見附錄 B.6）：`https://pytorch.org/get-started/locally/`
- 其他（補完整性，多為 legacy/embedded）：cuDLA（Jetson/DLA 部署）、NVBLAS（BLAS drop-in，較 legacy）

## A.8 v2.0 新增資源
- FlashAttention-4（CuTe DSL 實作）：`https://github.com/Dao-AILab/flash-attention` + arXiv:2603.05451
- CuTe DSL（CUTLASS 4.x Python）：`https://docs.nvidia.com/cutlass/latest/media/docs/pythonDSL/overview.html`｜PyPI `nvidia-cutlass-dsl`
- NVIDIA Dynamo：`https://docs.nvidia.com/dynamo/`｜NIXL：`https://github.com/ai-dynamo/nixl`
- LMCache：`https://github.com/lmcache/lmcache`｜Mooncake：`https://github.com/kvcache-ai/Mooncake`
- DeepSeek 開源 kernel：FlashMLA / DeepGEMM / DeepEP：`https://github.com/deepseek-ai`
- NVSHMEM：`https://docs.nvidia.com/nvshmem/`｜NCCL Device API：NCCL User Guide（2.28+）
- NVFP4 介紹：NVIDIA 部落格「Introducing NVFP4」｜TensorRT Model Optimizer：`https://github.com/NVIDIA/Model-Optimizer`
- TorchAO / FlexAttention / torch.library / memory visualizer：PyTorch 官方 docs
- Helion：`https://github.com/pytorch/helion`｜Gluon：`https://triton-lang.org/main/gluon/`｜TileLang：`https://github.com/tile-ai/tilelang`
- KernelBench：`https://github.com/ScalingIntelligence/KernelBench`
- NVBench：`https://github.com/NVIDIA/nvbench`
- AIPerf（GenAI-Perf 後繼的 LLM serving benchmark）
- verl / vime / OpenRLHF（RL rollout 基礎設施）
- PyTorch Release Compatibility Matrix：`https://github.com/pytorch/pytorch/blob/main/RELEASE.md`
- cuda-checkpoint / CRIU / Dynamo Snapshot（GPU checkpoint/restore）

---

# 附錄 B — 速查表（B.5/B.6 已填；B.1–B.4 隨教材逐課補）
- B.1 CUDA Runtime API 常用清單（記憶體/stream/event/error）
- B.2 nvcc 編譯旗標速查（`-arch`/`-gencode`/`-G`/`-lineinfo`/`-Xptxas`/`--use_fast_math`）
- B.3 Nsight Systems / Compute 常用指令與關鍵指標
- B.4 記憶體階層與延遲/頻寬數量級對照
- B.5 compute capability ↔ 架構 ↔ 代表卡 對照（已填）
- B.6 版本鎖定矩陣（v2.0 新增，已填）

**B.6 — 版本鎖定矩陣【v2.0 新增；全課程唯一的版本真相來源，正文各處版本敘述以本表為準】**

| 元件 | 最低可用 | **課程驗證版本** | 最新可選（2026-07） | 備註 |
|---|---|---|---|---|
| CUDA Toolkit | 12.8（Blackwell 門檻） | **13.3 Update 1** | 13.3 U1 | CUDA Tile C++ 需 13.3 |
| NVIDIA Driver | R580（13.x 最低相容） | **R610（610.43.02）** | R610+ | **課程驗證**用 R610；13.x minor compat 與 cuTile 執行最低 **R580**、Tile 詳細 profiling 需 **580.126.09** |
| PyTorch | 2.7（sm_120 起點） | **2.12（cu130）** | 2.13.0 | cu130 為 PyPI 預設；cu128 於 2.12 deprecated＋移出標準 matrix、2.13 明列 CUDA 12.8/12.9 builds removed。**2.13 差異註**：Triton pin 升 3.7.1、cu13 binary 不再內含 `ptxas`（**僅影響只靠 wheel、未裝完整 Toolkit 的環境**；本課裝完整 CUDA Toolkit，system `nvcc/ptxas` 路線不受影響——見 9.3/9.4）、Inductor 新增 CuTeDSL backend（接 9.31）、3.15/3.15t 為初期支援且 `torch.compile` 尚不支援 3.15、移除 3.13t Linux binary——**課程維持 2.12 為驗證基準** |
| Triton | 3.3（CC 8.0 門檻起點） | 3.7.x | 3.7.x | sm_75 僅 interpreter |
| NCCL | 2.28.7（教 GIN 的最低版本；symmetric memory 歷史起點為 2.27） | **PyTorch wheel 路線＝2.29.7（torch 2.12.1 釘 `nvidia-nccl-cu13==2.29.7`）；standalone C++ 路線＝2.30.7** | 2.30.7 | Device API 在 2.30 前後 host setup 有差異、官方明示 API 仍在演進——**教材範例必須 pin 版本、換版後重新編譯**（課 6.21／DeepEP V2 的關鍵相依） |
| cuTile Python | 1.0（CUDA 13.1） | 1.4.0 | 1.5.0（2026-07） | Hopper 需 1.4.0+ |
| CCCL | 3.x | 3.3.3 | 3.3.x | 隨 13.3 U1 |
| Ubuntu | 22.04 | **26.04 LTS** | 26.04 | 24.04 亦相容 |
| Host GCC（給 nvcc） | 6.x（官方支援下限） | **15（26.04 預設）** | 15 | CUDA 13.3 支援 gcc ≤ 15（`__GNUC__ > 15` 報錯）；26.04 預設 gcc-15 免 `-ccbin`；>15 才需 `-ccbin`／`NVCC_CCBIN`，見 2.3 |
| Python | 3.10 | 3.12 | 3.14（含 3.14t） | free-threading 見 10.16 |

> 查證日期：2026-07-16。開課或換機時重跑〈課 2.4〉驗證 checklist 並更新本表；正文提到版本號之處若與本表衝突，以本表為準。

**B.5 — Compute Capability 速查（「我的卡能不能跑」用）**

| CC / sm | 架構 | 代表卡 | 你的定位 | 關鍵能力門檻 |
|---|---|---|---|---|
| **7.5** | Turing | **Quadro RTX 4000（你的筆電）**、T4、RTX 20 | **現況主力** | FP16 WMMA Tensor Core ✅；**無** cp.async、**Triton 非官方支援**（3.3 起，只讀語法）、**無** cuTile |
| 8.0 | Ampere（DC） | A100 | 雲端 | cp.async、cuSPARSELt 2:4、Triton、cuTile 起跑線 |
| 8.6 | Ampere（消費） | RTX 30 | — | 同 8.0 家族 |
| **8.9** | Ada | **RTX 4090（未來桌機）**、L40S | 未來桌機 | **FP8** Tensor Core |
| 9.0 | Hopper | H100/H200 | 雲端 | WGMMA、TMA、thread block cluster/DSM、PDL、MemSyncDomain |
| 10.0 / 10.3 | Blackwell（DC） | B200/GB200、B300/GB300 | 雲端 | tcgen05/TMEM、FP4；`compute_100f` family（**與 12.x 不互通**） |
| **12.0** | Blackwell（消費） | **RTX 5090（未來桌機）** | 未來桌機 | 第 5 代 Tensor Core FP4、擴充 mma.sync（**無** TMEM/tcgen05）；`compute_120f` family |
| 12.1 | Blackwell | GB10 / DGX Spark | 雲端 | 同 12.x family |

> 一句話：**你現在能實跑的是 sm_75 那一列**；**Triton／cuTile → sm_80+（4090 或任何 Ampere+ 雲端即可，不必等 5090）**；**FP8 → 4090（sm_89）**；**FP4 → 5090 或資料中心 Blackwell**；**tcgen05／TMEM → 僅資料中心 Blackwell sm_100/103（5090 沒有）**。跨 family（10.x↔12.x）binary 不通用，fatbin 要各編一份。

# 附錄 C — 中英詞彙對照表（之後逐課填）
warp、occupancy、coalescing、bank conflict、divergence、tiling、reduction、scan、stream、kernel fusion、roofline、Tensor Core、quantization… 等。

# 附錄 D — 依目標的跳讀地圖
- **D.1 純打底（理解 GPU）**：Part 1 → 4。
- **D.2 盡快寫出有用 kernel**：Part 3 → 4 → 5.1~5.3 → 6.2 → 8.1~8.6。
- **D.3 推論引擎工程師路線（你的主線）**：Part 3 → 4 → **Part 5 子集（5.1–5.5、5.9–5.10：GEMM/reduction/scan/histogram/atomics/cooperative groups）**——【v2.0.6 修正理由】GEMM／reduction／atomics 確實是 FA 與 paged KV 的直接地基；**scan／histogram／cooperative groups 則是為建立完整平行演算法能力而列入 Core，並非兩者每一項都直接依賴**（v2.0.5 曾一律寫成「FA 與 paged KV 都建在這上面」，過度宣稱） → 8 → 9（全部，**含 v2.0 新增 9.0、9.30–9.38**）→ 6.20/6.21（通訊）→ 12（系統/JIT）→ 13（生產/管線，含 13.13）→ 10.12 選項 A/B。
- **D.4 優化專精路線**：Part 4 → 8（全部，含 8.20）→ 7.13 → 9.6 → 8.17 → 9.31 → 10.12。
- **D.5 系統 / 基礎設施路線（Staff/Principal）**：Part 4 → 8 → 12（Driver/NVRTC/IPC/NVML）→ 10.16（host 端效能）→ 13（資料管線/部署/observability）→ 9.32/9.33（分散式 KV 與 Dynamo）。

---

## 下一步

這份大綱是「目錄＋每課的學習藍圖」。**實際開課時**，每一堂課我會展開成完整教材：可執行的程式碼、逐行註解、架構圖/流程圖、Nsight 截圖式說明、對應書章頁碼。

---

# 附錄 E — 7 小時／週的時間軸學習計畫【v1.9 新增】

> **前提**（依學員實際條件）：每週約 **7 小時**（平日忙就週末補足）；目標為 Staff/Principal 等級的 LLM 推論引擎開發；硬體為 **Dell Precision 7550 筆電（Quadro RTX 4000，Turing / sm_75，8GB）**，未來可能加入 5090／4090 桌機。
> **分工**：課程地圖回答「順序」、**附錄 D** 回答「依目標怎麼跳讀」、**本附錄回答「要多久」**——把 D.3（推論引擎主線）套上真實時間預算與里程碑。

## 先講誠實的算術

| 走法 | 課數（實列） | 有效學習時數 | 7h／週 → 日曆時間（含複習/緩衝） |
|---|---|---|---|
| **完整 185 課（v2.0）** | 185 | 約 380–560 h | **約 16–26 個月** |
| **Core Path（＝下方三階段實列）** | **136 必修＋2 可選＝138** | **約 340–430 h** | **約 13–19 個月** |
| 精簡核心（只求會寫 kernel） | 約 35 | 約 70–90 h | 約 3–5 個月 |

> ⚠️ **數字對齊（v2.0.5 逐課實列重算）**：下方三階段相加 = **階段 1（58 課／~116h，含 7.1–7.9 與補齊的 5.4/5.5/5.9/5.10）＋階段 2（22 課／~63h）＋階段 3（58 課／≥160h：8.15–8.20 6＋Part 9 其餘 28＋6.20/6.21 2＋7.10–7.13 4＋Part 12 9＋10.16 1＋13.13 1＋Capstone 1＋10.14/10.15 可選 2＋Part 11 4）＝ 138 課（**含 10.14/10.15 兩堂可選；純必修 136**）/ ≥339h**——**v2.0.4 曾寫「≈148 課」，逐課點算後為 138，已更正**（階段 3 的「Part 9 其餘」已含 9.30–9.38，勿重複計數）。完整 185 課必然 ≥ Core Path，完整版下限 380h。7h／週：Core ≈ 340h÷7 ≈ 49 週純學習 ≈ 11–12 個月，加複習/作業/卡關緩衝 → **13–19 個月**；完整版再多約 **47** 堂較輕量課（若不做 10.14／10.15 兩堂跨後端選修，Core 為 **136 必修**、差額則是 **49** 堂）→ **16–26 個月**。真想壓到 3–5 個月，走「精簡核心」或〈附錄 D.2〉最短路徑。**若 16 堂 v2.0 新課全跳過，時程即回到 v1.9 的估計——它們是「補到 2026 年職缺水位」的投資，不是灌水。**
> 估時假設：讀＋小動手做約 1.5 h／課；真正要寫 kernel、debug、跑 profiler、做 Capstone 的課要 **2–3×**。「有效學習時數」不含複習/作業/卡關緩衝，「日曆時間」已含。**別用「讀完」當標準，用「跑得出結果」當標準。**

## Core Path：三階段

### 階段 1（0–3+ 個月，約 116 h，含 7.1–7.9）— 能寫出「有優化、且驗證過正確」的 kernel
- **Part 1**（1.1–1.9）＋ **Part 2**（2.1–2.7；2.8／2.9 需要時再看）
- **Part 3 全部**（3.0–3.9）← **3.0 對資深 C++ 工程師是全課程 CP 值最高的一課**
- **Part 4 全部**（4.1–4.10）← **不可跳，這是分水嶺**
- **課 5.1–5.5、5.9–5.10**（GEMM naive／tiled GEMM／reduction／**scan**／histogram／atomics／cooperative groups）← **與〈附錄 D.3〉對齊**（v2.0.4 階段 1 只放 5.1–5.3、與 D.3 衝突，v2.0.5 補齊）。**【v2.0.6】理由精確化**：GEMM／reduction／atomics 是 FA 與 paged KV 的直接地基；scan／histogram／cooperative groups 屬「完整平行演算法能力」訓練，非兩者的硬前置
- **課 8.1–8.6**（方法論＋Roofline＋Nsight Systems/Compute＋頻寬／shared／occupancy＋自製 autotuner）
- **課 7.1–7.9**（除錯思維／cuda-gdb／**compute-sanitizer**／printf／錯誤處理／race／記憶體錯誤／**數值正確性驗證**）← **邊寫 kernel 邊學、別拖到最後**（CI 與效能回歸 7.10–7.12 才留到階段 3）
- **里程碑**：能對自己的 kernel 做 profile、指出瓶頸、用數據證明優化有效，**且能用 sanitizer 抓出 race／越界並以 CPU reference 驗數值**。

### 階段 2（3–6 個月，約 63 h）— 讀得懂、改得動推論引擎
- **課 6.1–6.2（cuBLAS）**＋ **6.12（CUTLASS／CuTe 概念）**；其餘 Part 6 按需查（**別當教科書讀**）
- **課 8.7–8.14**（指令層／streams／CUDA Graphs＋PDL／非同步＋pipeline／Tensor Core／warp primitives／fusion／完整案例）
- **課 9.0–9.7**（**【v2.0】9.0 LLM 架構解剖起手** → DL 運算特性 → GEMM → attention → FlashAttention → **Triton**）
- **課 9.13**（serving 系統＋backpressure）、**9.17**（Paged KV cache）、**9.18**（數值穩定）
- **里程碑**：能講清楚一次 forward 的算力／頻寬帳；能改出一個 fused kernel（**筆電階段：CUDA C++**；有 sm_80+ 卡再做 Triton 版）。

### 階段 3（6–13 個月，**≥160 h**，視 Capstone 深度而定）— 系統級與作品集
> ⚠️ **這階段最重**：幾乎整個 Part 9 剩餘＋**7.10–7.13**（7.1–7.9 已在階段 1）＋Part 12＋v2.0 新增課＋Capstone；光 Capstone A/B（自訂 FA／mini 推論引擎）單項就可能 40–80h。把它當「6–13 個月、以 Capstone 為主軸」而非固定時數。
- **課 8.15–8.20**（allocator／cluster＋CLC／warp specialization／CUDA Tile／cuTile／**LLM-in-the-loop**）
- **Part 9 其餘（含【v2.0 新增】9.30–9.38）**（量化、投機解碼、serving 框架、多 GPU…）（其中 **9.31 CuTe DSL、9.32/9.33 分散式 KV 與 Dynamo 為 Core 優先**；9.37/9.38 視目標選讀）
- **課 6.20/6.21**（GPU-initiated 通訊——搭配 9.35 一起讀）＋ **課 7.10–7.13**（測試／CI／效能回歸／**浮點語意**）← **Staff 分水嶺，別跳**
- **Part 12**（Driver API／NVRTC／CUPTI）＋ **課 10.16**（host 端效能）＋ **Part 13 選讀**（13.13 概念）
- **Part 10 Capstone**＋（可選）**10.14／10.15 跨後端軌**
- **Part 11**（職涯）＋ 課 9.16（經驗遷移，此時再讀）
- **里程碑**：一個能公開展示的 Capstone ＋ 一份效能報告。

## 哪些可以先跳（回頭再補）
- **Part 6 大部分**（21 課的函式庫生態；**但 6.20/6.21 通訊兩課屬主線、別跳**）→ 其餘用到再查。
- **Part 13**（生產／資料管線）→ 真的要上線再讀。
- **10.11（機密運算）、10.13、2.9（WSL2／雲端）** → 需要時再看。
- **10.14／10.15（跨後端）** → 有雲端 MI300X 預算時再做；它是**差異化**，不是基礎。

## 筆電（sm_75）跑不了／要降級、要等桌機或雲端的課
> **先講底線**：CUDA 13.x 的最低架構就是 **sm_75**——筆電剛好踩線，**能跑 13.3**，但新的 Tile／部分 Tensor Core／cluster／硬體非同步路徑用不到。
- **需 sm_80+（Ampere；sm_75 硬性不可跑，非降級）**：**8.18／8.19**（CUDA Tile C++／cuTile Python 官方最低 **CC 8.0**——**筆電完全不能跑，非 Blackwell 專屬**）、**6.16** cuSPARSELt（2:4 結構化稀疏需 Ampere Sparse Tensor Core，CC 8.0+）、**9.7** 的 **Triton**（Triton 3.3 起**已移除 Turing 支援、最低 CC 8.0**——sm_75 只能用 interpreter 學語法，**GPU 執行/profiling/autotune 要等 sm_80+ 卡**）
- **需 sm_80+ 才有硬體加速（sm_75 可編、退回較慢路徑）**：**8.10** 的 `cp.async`／`cuda::pipeline`／`cuda::barrier` 非同步 global→shared 拷貝（sm_75 退回同步版，觀念可讀）
- **需 sm_89+（Ada；FP8）**：**8.11** 的 **FP8** Tensor Core（Ada/Hopper 起，4090 可、筆電不可；**FP8 不是 Blackwell 才有**）
- **需 sm_90+（Hopper）**：**8.9** 的 Programmatic Dependent Launch（PDL，官方明列 CC 9.0+）、**8.15** 的 **Memory Synchronization Domains 子節**（⚠️ **8.15 的 allocator/pool/arena/VMM 主體 sm_75 完全可做**，只有 MemSyncDomain 這一小節需 sm_90+）、**8.16** 的 thread block cluster／distributed shared memory／CLC（筆電與 4090 皆不支援；CLC 另需 Blackwell）
- **需 Blackwell（sm_100/120）**：**8.11** 的 **FP4／block-scaled** Tensor Core、**8.17** 的 tcgen05（sm_100/103）與 block-scaled mma.sync（sm_120）路線
- **9.6** 的 FA-3 Hopper 路線；**10.14／10.15** 的 AMD 實機部分（雲端 MI300X）
- **【v2.0】新增課的門檻**：**9.30**（硬體加速需 Blackwell；數值分析 sm_75 可做）、**9.31**（需 sm_80+，完整特性 Hopper/Blackwell）、**6.20/6.21/9.35**（通訊需多卡/雲端；**DeepEP V2 runtime 另需 sm_90+、PyTorch 2.10+、NCCL 2.30.4+，見 9.35**）、**9.33**（完整 lab 需多節點）、**13.13**（純概念）；**sm_75 完全可做**：7.13、8.20、9.0、9.32（估算）、9.34、9.36（讀）、9.38（讀）、10.16
- **⚠️ 8GB VRAM 天花板**：**Part 9 的 vLLM／TensorRT-LLM 大模型 serving（9.9–9.14）在 8GB 上只能玩具級**（小量化模型／短 context）；KV cache、paged attention、continuous batching 的**觀念與精簡實作可在筆電做**，真實吞吐／併發要等桌機大 VRAM 或雲端。
> 這些課的**觀念仍可先讀**；只有動手做要等硬體。**基礎 Tensor Core（FP16 WMMA）在 Turing／sm_75 可跑**（8.11 的入門部分不受影響）。

## 節奏建議
- **每週固定完成 1 個「動手做」** 比讀 3 課有用。Part 4 之後，**沒跑過的課等於沒學**。
- 自建一個 `PROGRESS.md`（學員自己維護的進度檔）記錄，每完成一課寫一行「我證明了什麼」。
- 卡關時**先跳過、標記、回頭補**——別讓一課卡死整條路徑。

---


---

# 附錄 F — 修訂紀錄

> **完整逐版修訂紀錄（v1.1–v1.9.1）規劃移入教材 repo 的 `CHANGELOG.md`**（Codex 建議：79 行歷史不該留在正文）——**⚠️【v2.0.1】該檔尚未建立、屬規劃中，引用前請確認**。本節保留 v2.0／v2.0.1 摘要。

## v2.0 更新（2026-07，第八輪外部互審：Claude（Fable 5）× Codex 獨立雙審合併）

> 課程數 169 → **185**（新增 16 課：6.20／6.21、7.13、8.20、9.0、9.30–9.38、10.16、13.13）；維持 13 Parts、不重編既有課號。雙審報告（`review_v2.0_claude.md` 與 Codex 審查紀錄）為製作過程的內部文件、**未隨本大綱發布**。

**🔴 P0 事實修正**：
① **4.10**：**【v2.0.1 撤回並更正】** public `cuda::thread_scope` 只有**四**個列舉值（thread/block/device/system）——v2.0 曾據誤判改成「五值含 _cluster」，本機 CUDA 13.3 實測 `cuda::atomic<T, cuda::thread_scope_cluster>` 編譯失敗，故改回四值；cluster atomic 走 `__nv_atomic_*`＋`__NV_THREAD_SCOPE_CLUSTER` builtin（非 `cuda::atomic`）。
② **4.6**：surface 是**可讀寫**的 CUDA array 介面、非唯讀；texture reference 已棄用；`__ldg` 降為歷史技巧（Codex）。
③ **9.20**：MPS 在 Volta 後為 per-client context，非「共用一個 context」；補 Linux/QNX 限定、不支援 dynamic parallelism、R610 static SM partitioning（Codex）。
④ **1.9**：GB200＝2 顆 B200 GPU＋1 顆 Grace（每顆 GPU 自身雙 die）；MIG 支援清單已含部分 RTX PRO Blackwell（Claude＋Codex）。
⑤ **9.4／2.4**：`cpp_extension` 的「JIT」＝Ninja＋nvcc 建置流程，與 driver PTX JIT／NVRTC／nvPTXCompiler 分四層——v1.9 混為一談（Codex）。
⑥ **7.5**：錯誤分級（非所有 error 都 sticky）＋ CUDA 13.x Error Log Management（Codex）。
⑦ **12.6**：以 CDP2 為準（CDP1 已棄用；in-kernel 同步等待已移除）（Claude＋Codex）。
⑧ **1.4／8.17**：dense block-scaled MMA 自 PTX ISA 8.8 可用 `sm_120f`，非全綁 `sm_120a`；WGMMA 在 Blackwell 標 deprecated（Claude＋Codex）。
⑨ **8.18**：官方旗標為 `--enable-tile`（雙破折號）（Claude）。
⑩ **10.10**：`std::execution::par / par_unseq`，非 `std::par`（Codex）。

**🕐 版本時效更新**：PyTorch cu130 為 PyPI 預設、cu128 已棄用（header／9.4 改寫；新增**附錄 B.6 版本鎖定矩陣**為唯一版本真相來源）；FlashAttention-4（2026-03，CuTe DSL、Blackwell）；vLLM V1（V0 已移除）；EAGLE-3；AIPerf 接替 GenAI-Perf（13.12）；cuTile 1.5.0；Triton 3.7.x；`pallas.triton` 更名；**12.9 重新加入 `cuda.lang`**（13.3 官方 Programming Guide 已明列——v1.9 移除是當時正確、現已翻案，標 Experimental）；1.3 Rubin 量產＋Rubin CPX 路線圖 case study；10.14 HIP NVIDIA 後端改「低優先維護」；10.9 補 4090 soft-ECC 開關與 5090 移除。

**➕ 結構性補洞（兩條軸線）**：**通訊下沉到 kernel 內**——6.20 NVSHMEM／6.21 NCCL Device API＋symmetric memory／9.35 MoE kernel＋DeepEP；**推論系統上升到機架尺度**——9.32 分層/分散式 KV（LMCache/Mooncake）／9.33 Dynamo＋NIXL／13.13 NVL72＋wide-EP。另補：9.0 現代 LLM 架構解剖、9.30 NVFP4/MXFP microscaling、9.31 CuTe DSL（Python）、9.34 hybrid/linear attention 推論、9.36 production kernel 案例研讀（FlashMLA/DeepGEMM/Machete）、9.37 VLM 多模態、9.38 RL rollout、7.13 浮點語意與確定性、8.20 LLM-in-the-loop kernel 工程、10.16 host 端效能工程；9.8 改版為 2026 DSL 光譜（CuTe DSL/Gluon/Helion/TileLang/cuda.lang）；3.8 補 HMM/ATS、3.9 補 device LTO、6.4 補 cuDNN Frontend/SDPA、6.2 補 cuBLAS 13、6.16 精度表更新、6.18 補 nvCOMPDx、7.12 補 NVBench、9.3 補 torch.library 現代流程、9.15 補 memory 視覺化工具鏈、9.22 修 Inductor 多後端＋.pt2＋FlexAttention、9.29 補 Transformer kernel 全套、12.1 補 cuGetProcAddress、13.7 補 EGM、13.8 補 checkpoint/restore、13.9 補 fleet 級調查、13.11 補 incremental detokenization。

**🧹 文件工程**：cuBLASDx 統一於 6.18（6.3 去重）；13.10 正名 PyNvVideoCodec；半形標點清理；不存在的本地資源（`tools/build.sh`／NVIDIA-AMD 對照表）標「規劃中」；附錄 E 重算（Core Path ≈148 課／13–19 個月）；v2.0 新課標 Core/Elective/Reference 與 sm_75 可跑性；歷史 changelog 移出正文。

**誠實駁回（雙審皆同意不加）**：C++26／std::execution 專課（無生產採用，併入 10.6 一節）；embedding/cuVS 專課（encoder-only serving 是已學知識的簡單子集）；tokenizer 演算法專課（系統面補 13.11 即可）；訓練深潛（9.38 已補推論交集，職缺證據支持推論為主的取捨）；課程改名（Codex 提議「CUDA、現代 C++ 與 AI/LLM 推論系統完整課程」——v2.0 以 VLM/hybrid/RL 補廣度＋「如何使用」加定位聲明代替改名，維持檔名連續性）；nvmath-python／Groq LPU／TLX／特定 LLM-kernel 產品專課。

### v2.0 定稿修訂（2026-07，第九輪互審：Codex 對 v2.0 全檔複審，Claude 逐項採納）

- **修錯 12 項**：13.3 GA/U1 日期分開標（GA 2026-05／U1 2026-06）；「關於時效」段落版本字樣 v1.9.1→v2.0；CompileIQ 明確標「不支援 NVRTC」（release notes 明文）；nvmath-python 已 1.0 GA（非 Beta）；Numba：傳統 numba-cuda 標 maintenance mode、numba-cuda-mlir 為新開發主線（6.19/12.9）；**DeepEP V1=NVSHMEM → V2=NCCL GIN 的版本遷移案例**（6.20/9.35）；9.34 改「state reuse 語意不同」（vLLM 已有 hybrid KV-cache manager，非單純「失效」）；9.20 MIG 口徑與 1.9 統一；8.19 巢狀粗體修復；附錄 E Part 6 課數 19→21；附錄 A.7 PyTorch 條目 cu128→cu130；附錄 A.7 AIPerf 定位與正文同步。
- **去絕對化 9 處**（benchmark 數字全部加註條件，不當普遍定律）：Rubin「R100」標媒體慣稱、CPX 改「官方未再列出、定位待確認」；3.0 虛擬函式「效能災難」→「可能昂貴、要量測」＋ libcu++ 無一般用途 device 容器；8.20 LLM kernel 能力限定題型/預算/verifier；9.13 的 3GB KV 標為量級示例（70B/GQA/FP16）、60–85% 命中率標觀測值；9.31 CuTe DSL 33–116×（B200 特定 workload）；9.32 Mooncake 3.8×標 1P1D/12 GPU/P50；Dynamo Snapshot 標 experimental/limited preview（9.33/13.8）；9.36 FlashMLA ~660 TFLOPS（隨版本演進、教材 pin commit）、DeepGEMM「300 行」標歷史意義；13.13 改「130 TB/s aggregate NVLink 頻寬」。
- **補知識 5 項（併入既有課、不加課）**：9.24 補 DeepSeek Sparse Attention／token 級 sparse（選取式 vs 淘汰式）；13.12 補 benchmark 方法學（open/closed-loop、goodput、SLO attainment）；9.20 補多租戶 KV/prefix cache 安全（cache salting、timing side channel）；7.10 補 host 端 ASan/UBSan/TSan/libFuzzer；7.13 補數值重現性政策（deterministic algorithms、bitwise vs numerically acceptable）。

**第十輪（Codex 定稿放行前 4＋7 項；審計數字經第十一輪更正）**：Rubin 改「2026-05-31 宣布量產爬坡、H2 2026 上市」；header driver 敘述精確化（Windows 自 13.1 起 Toolkit 不隨附 display driver、R610 為驗證基準而非「隨附」）；DeepEP 去「第一個」（Tutel/DeepSpeed-MoE 更早）；6.21 NCCL 功能逐版鎖定（sym mem 2.27／Device API 2.28／GIN 2.28.7／文件 2.30.7）＋ B.6 增 NCCL 列；9.7 殘留 cu128 改寫；B.6 補 PyTorch 2.13 差異註（Triton 3.7.1、cu13 無 ptxas、Inductor CuTeDSL backend、3.15 初期支援）；10.16 兩個數字標特定研究/microbenchmark；9.36 DeepGEMM 去絕對化；1.5 的 4N/4NP 降為旁註；9.20 side channel 附論文（arXiv:2502.07776）；7.10 明確 TSan（host）/racecheck（GPU）分工。

**第十一輪（放行確認，Codex 2＋2 項）**：2.3 修掉殘留的「隨附 packaged driver」措辭（Windows 自 13.1 起需另裝、Linux 依安裝方式而定，與 header 對齊）；B.6 NCCL 列改精確版本（教 GIN 最低 2.28.7；課程驗證分兩路：PyTorch 2.12.1 wheel 釘 `nvidia-nccl-cu13==2.29.7`、standalone C++ 用 2.30.7；Device API 換版需重編、範例 pin 版本）；B.6 的 PyTorch 2.13 ptxas 註限定「僅 wheel-only 環境」；本節審計數字 4＋6 → 4＋7 更正。**至此 v2.0 定稿。**

### v2.0.1 修訂（2026-07，第十二輪：Claude × Codex × Gemini × Grok 四家互審，Claude 逐項查證合併）

> 四家審查合流。Claude 對每項親自查證（**本機 CUDA 13.3 編譯**／arXiv 論文／出版商與官方文件），只採納查證通過者。**課數維持 185、13 Parts、不重編課號**——本輪為事實勘誤＋硬體門檻精確化＋文件工程，未增刪課。

**🔴 事實修正（含撤回 v2.0 自身錯誤）**：
① **4.10／附錄 F①【撤回 v2.0 的 P0 修正】**：public `cuda::thread_scope` 只有**四**個值（thread/block/device/system）。v2.0 曾把 v1.9 正確的「四值」改成「五值含 _cluster」——**本機 CUDA 13.3 實測 `cuda::atomic<T, cuda::thread_scope_cluster>` 編譯失敗（`namespace "cuda" has no member`）**；cluster atomic 走 builtin `__nv_atomic_*`＋`__NV_THREAD_SCOPE_CLUSTER`（非 `cuda::atomic`）。（Codex 抓出＋Claude 本機編譯確認）
② **9.6 FA-4**：刪查無出處的「比 FA-3 快 1.5–2×」，改論文實測（B200/BF16：vs cuDNN 9.13 ~1.3×、vs Triton ~2.7×、1613 TFLOPs/71%）；FA-4 code-reading 排在 9.31 之後（階段 3）。**支援範圍見 v2.0.2 修正②——套件支援 Hopper+Blackwell，非 Blackwell-only。**
③ **附錄 A.1 PMPP 5e**：出版日由 2026-02 更新——見 v2.0.2 修正③（不寫死單一日期，Elsevier 頁面 2/27 與 6/3 並列）。
④ **9.35／附錄 E DeepEP V2 門檻**：分層標示——簡化 kernel sm_75 可跑；**V2 runtime 需 sm_90+、PyTorch 2.10+、NCCL 2.30.4+、NVLink＋RDMA**；點出 B.6 的 wheel NCCL 2.29.7 不符、須走 standalone 2.30.7。（Codex）
⑤ **10.3 GeForce P2P**：補「消費級 GeForce（含 4090/5090）不支援 PCIe P2P、`cudaDeviceEnablePeerAccess` 回 `cudaErrorPeerAccessUnsupported`」；動手做改為「觀察 P2P 失敗＋量測 fallback 頻寬懲罰」。（Gemini）**（卡型概括／NCCL fallback／「P2P≠一定 NVLink」措辭已由 v2.0.2 ⑤、v2.0.3 ① 收斂。）**
⑥ **B.5 速查**：「FP4／tcgen05 → 5090」拆成「FP4 → 5090 或 DC Blackwell；tcgen05/TMEM → 僅 DC sm_100/103」，消除「5090 有 tcgen05」誤讀。（Codex＋Grok）
⑦ **10.15 Pallas**：Triton backend 由「best-effort」更新為「已 deprecated（官方 changelog 標將移除）」。（Codex）

**➕ 內容精確化（併入既有課、不加課）**：
⑧ **3.8**：UM 在 x86+PCIe（page-fault、慢）vs GB200 NVLink-C2C（~900 GB/s coherent）效能落差大，明確警告。（Gemini）**（「毒藥／零拷貝標配」絕對化措辭已由 v2.0.2 ④ 收斂。）**
⑨ **9.7**：補 Hopper/Blackwell Triton 進階定址——見 v2.0.2 修正①（block pointer＝結構化定址、顯式 TMA 走 `tl.make_tensor_descriptor`）；sm_75 無 TMA、概念先懂。
⑩ **10.6**：補「避開 C++20 Modules（`import std;`），nvcc/CMake 相容性未成熟，維持 `#include`」排雷。（Gemini）
⑪ **8.18**：CompileIQ「亦支援 Triton/Helion」加「以當版 release notes 為準」。（Grok）
⑫ **1.4**：「你的 5090 在這」→「目標卡／未來桌機在這；現況筆電 sm_75」。（Grok）
⑬ **10.12 Capstone**：新增 3 個 sm_75 可交付選項 H/I/J（DeepEP/Dynamo 資料流報告、mini-Llama 前向、host 端瓶頸報告）。（Grok）

**🧹 文件工程**：
⑭ **line 4／附錄 F 抬頭**：`CHANGELOG.md` 由「已移出」改「規劃中、尚未建立」（消除斷鏈，同 tools/build.sh 慣例）。（Codex＋Grok）
⑮ **line 19／23**：「每堂課都標前置」「16 課均標分級」→ 誠實化為「部分已標、教材期補齊」；補「Core＝知識地圖核心、非現在就要實跑」定義，與附錄 E 硬體分層對齊。（Codex＋Grok）

**誠實駁回／未採納（附理由）**：Grok 的 D.3a/D.3b 拆分與「Core 全面重標表」＝可觀的編輯口味調整，本輪只做「定義釐清＋與附錄 E 對齊」最小修正，未整批改分級（尊重課程設計者的 Core 意圖）；Grok「新增成本會計（$/1M tokens）課」＝與 v2.0 已駁回加課原則一致，不加；附錄 E 完整版下限 380h vs Core 上限 420h 區間重疊＝估算模糊、可接受，不動；Gemini G2/G3 採「精確補一句」而非誇大（block pointer 非「不用就龜速」的唯一路徑、UM 差異文件本已提及只是加強）。**repo 同步（PROGRESS.md 仍 v1.8/167、把 v2.0.1 併入 `~/AI/AI_Course/.../cuda-ai-course/`）＝獨立動作，待使用者指示。**

> ⚠️ 仍待使用者本機二次驗證的時效宣稱（B.6 為準）：PyTorch 2.12 cu130 預設、NCCL wheel 2.29.7／standalone 2.30.7、AIPerf 接替 GenAI-Perf、Rubin 量產時程、DeepEP V2 的 PyTorch/NCCL 版本下限。開課前對 live 官方 matrix 覆核。

### v2.0.2 修訂（2026-07，第十三輪：Codex 對 v2.0.1 複審，Claude 逐項查證合併）

> Codex 提 7 點，Claude 全數查證後採納（含反轉 v2.0.1 自身兩處過度修正）；Gemini/Grok 本輪無新意見。**課數維持 185、13 Parts、不重編課號。**

**🔴 修正（含撤回 v2.0.1 過度修正）**：
① **9.7 Triton／TMA【訂正 v2.0.1 引入的錯誤】**：block pointer（`tl.make_block_ptr`＋`tl.advance`）＝結構化區塊定址；**Hopper/Blackwell 的顯式 TMA 路徑是 `tl.make_tensor_descriptor` 及其 load/store**，不是 block pointer。v2.0.1 把 TMA 綁在 block pointer 上有誤。（Codex 查官方 Triton API）
② **9.6 FA-4【撤回 v2.0.1 過度修正】**：官方套件 README 明列 FA-4「optimized for Hopper and Blackwell GPUs (e.g. H100, B200)」——**套件支援 Hopper＋Blackwell**；v2.0.1「非官方支援 Hopper」是過度修正。正確切分＝套件支援 Hopper+Blackwell、論文硬體設計/數據聚焦 B200。benchmark 數字（1.3×/2.7×/1613 TFLOPs）維持。（Codex 抓出＋Claude 查 repo README）
③ **附錄 A.1 PMPP 5e**：不寫死 2026-06-19（無官方依據）——Elsevier 頁面同時列「2026-02-27」與「Published 2026-06-03」，改「2026 出版、兩日期並列」。（Codex 查 Elsevier）

**➕ 去絕對化／收斂**：
④ **3.8 UM**：PCIe UM「效能毒藥」、GB200 UM「零拷貝標配」改為有條件敘述——PCIe fault/migration 熱路徑上「可能」很貴、需 prefetch/advise；NVLink-C2C coherent 好用但仍受 TLB/資料放置/局部性影響、一樣要 profiling＋memory advice。（Codex 引 CUDA UM 指南）
⑤ **10.3 GeForce P2P**：核心事實（4090/5090 無 PCIe P2P）保留；不再概括「所有 RTX 30/40/50」，改「消費級 GeForce（實測含 4090/5090）、各組合以實測為準」；NCCL fallback 由「CPU zero-copy」改「SHM/host memory 中轉、SHM 關閉再退 network transport」。（Codex 引 NCCL 文件）
⑥ **10.6 `import std;`**：正名——C++20 是 language modules、`import std;` 是 **C++23** 標準庫模組；避開 module 化的排雷建議不變。（Codex 引 WG21 P3208）

**🧹 文件工程**：
⑦ **line 19／附錄 F 抬頭**：「16 課均標 API 穩定度」改「適用處標、逐課補齊」（避免過度宣稱）；`review_v2.0_claude.md` 標為「製作過程內部文件、未隨大綱發布」（消除斷鏈）。（Codex）

**未採納**：無——Codex 本輪 7 點全採。
**repo 同步仍待使用者指示**：現 repo README 為 v1.7.5/165 課、PROGRESS 為 v1.8/167 課、下一課分別寫 1.2 與 2.4，兩份彼此不一致；Codex 建議 v2.0.2 定稿後再「一次性」同步，勿只更新其中一份。

> **【v2.0.2 已定稿】** 兩個 High（FA-4 支援範圍、Triton TMA API）已修，可作為目前的黃金版；正式逐課產教材前先做 repo 同步。

### v2.0.3 修訂（2026-07，第十四輪：Codex 對 v2.0.2 複審，Claude 逐項查證合併）

> Codex 本輪 0 High、2 Medium＋2 Low，全數採納。重點是修掉 **Markdown 實際渲染破損**（本機 pandoc 逐行確認）與 P2P 語意的最後收斂。**課數維持 185、13 Parts、不重編課號。**

**🖥️ Markdown 渲染修復（pandoc 實測 5 行殘留裸 `**`，全修）**：line 31／120／791／792／1049——根因是 `**` 貼在全形標點與 CJK 之間（pandoc flanking 規則無法開/閉粗體），以及 1049 module 句因前一版殘留開頭 `**` 造成 **奇數 `**`（真破損）**。改法：把粗體邊界移離全形標點、拆掉不必要的巢狀粗體。修後 pandoc 全文 0 裸 `**`。（Codex 本機 pandoc 抓出＋Claude 本機 pandoc 覆核）

**🔧 技術收斂**：
① **10.3 P2P 末句**：「真 P2P 要資料中心卡／NVLink」不正確——**P2P 可走 PCIe 或 NVLink，取決於 GPU/拓撲/driver**；4090/5090 不支援，但專業 RTX/Quadro 與資料中心 GPU 可能支援（PCIe 或 NVLink），要逐 pair 以 `cudaDeviceCanAccessPeer()`／`nvidia-smi topo -p2p` 實測。1033「在 GeForce 上」收斂為「在目標 RTX 4090/5090 上」。（Codex 引 CUDA multi-GPU 指南）
② **9.7 Triton TMA 適用範圍**：釐清 `tl.make_block_ptr` 是**一般結構化定址、非 sm_90+ 專屬**；只有 `tl.make_tensor_descriptor` 的 TMA backing 才是 sm_90+ TMA-capable 路徑（v2.0.2 誤把「sm_90+ 專屬」套到兩者）；同時拆掉該句不必要的巢狀粗體。（Codex 引 Triton 官方 API）

**🏷️ 版本標籤一致化**：FA-4（802）等本輪改對的段落 inline 標籤 v2.0.1→v2.0.2/v2.0.3；附錄 F 的 v2.0.1 ④（UM）、⑤（P2P）加註「措辭已由 v2.0.2 收斂」，保留歷史但不誤導。（Codex #4）

> **【v2.0.3】** 技術內容與渲染皆已收斂；建議此版定稿後，一次性把 outline＋README＋PROGRESS 同步到 185 課（三者現為 v2.0.3／v1.7.5-165／v1.8-167，彼此不一致，勿只更新其一）。

### v2.0.4 修訂（2026-07，第十五輪：App 版 Claude 對 v2.0.3 複審，Claude 本機查證後選擇性採納）

> App-Claude 提 1 P0＋3 minor。Claude 在**本機 26.04 + CUDA 13.3** 三重實測後判定：**P0 的技術內容錯、嚴重度誇大，但戳到一個真的小缺口**（Part 2 從未提 host compiler 相容性）；只採納「補正確版註記」這一項。**課數維持 185、13 Parts、不重編課號。**

**➕ 唯一新增（補正確版 host compiler 註記）**：
- **2.3／2.6／B.6**：補「CUDA 13.3 nvcc 支援 **gcc ≤ 15**（`host_config.h` 門檻 `__GNUC__ > 15`）；**26.04 預設 gcc-15 剛好可用、不需 `-ccbin`**（本機 `nvcc -arch=sm_75` 實測編過）；若 host gcc >15（26.04 庫內有 gcc-16）才需 `-ccbin g++-15`／`NVCC_CCBIN`／`-allow-unsupported-compiler`」；B.6 增「Host GCC」列。

**🔍 判定為錯／過時、未採納（附本機證據）**：
- **P0「必須 -ccbin g++-14、nvcc 會炸」＝錯**：本機實測預設 gcc-15 `nvcc -arch=sm_75` 直接編過、13.3 `host_config.h` 明列支援 gcc≤15；App-Claude **誤引**使用者自己的 `dev_packages_guide`（該檔結論正相反：「13.3 支援 gcc-15、**不需** -ccbin，`tools/build.sh` 也沒用它」），並把 CUDA 13.1（gcc-15 或不在支援表）與 13.3（支援 gcc-15）混為一談。故大綱維持「不需 -ccbin」正確，只補相容性註記、**不改編譯政策**。
- **minor #3「9.7 動手做仍寫雙卡 autotune、與 interpreter 不一致」＝過時**：App-Claude 在看 v1.9；v2.0.3 的 9.7 動手做早已標「GPU 執行需 sm_80+／筆電只用 interpreter／autotune 待 sm_80+ 卡」，已一致、未改。
- **minor #1「cu128『棄用』→『移除』」**：屬用詞精修、須核對 PyTorch 2.12 官方 release notes 原文（撰稿環境無法查證），暫不改，記此待製作 9.4 時定案。

**🗂️ 待 repo 同步時處理（minor #2，App-Claude 正確）**：`tools/build.sh` 與《NVIDIA↔AMD 對照表》其實**已存在**於 `cuda-ai-course/`（本機確認）；大綱現標「規劃中／尚未建立」是相對 repo 的過時說法——併入 repo 時改「已存在」、對照表按新知識（Triton sm_80+、DeepEP V2）小幅同步、順手建 `CHANGELOG.md`。

> **【v2.0.4】** 至此技術內容、渲染、host compiler 相容性皆收斂。下一步：一次性 repo 同步（outline＋README＋PROGRESS → 185 課／26.04／B.6 為版本真相）。

### v2.0.5 修訂（2026-07，第十六輪：Codex／Gemini／Grok 三家對 v2.0.4 複審，Claude 逐項查證合併）

> 三家共提 ~21 項，Claude **採納 20、駁回 1**（附本機／官方查證）。**課數維持 185、13 Parts、不重編課號**；但 **Core Path 課數與階段 1 內容有實質更正**。

**🔴 P0（數字錯／假斷言／技術錯句）**：
① **Core Path「約 148 課」算錯 → 實為 138**（Grok 抓出，Claude 逐課點算證實）。修法**不是改數字了事**，而是連同 ② 一起解：階段 3 逐項實列＝58（8.15–8.20 6＋Part 9 其餘 28＋6.20/6.21 2＋7.10–7.13 4＋Part 12 9＋10.16 1＋13.13 1＋Capstone 1＋10.14/10.15 可選 2＋Part 11 4）；連帶更正「185−Core ≈ 37 堂」→ **47 堂**、階段 1 時數 108h→116h。並註明「Part 9 其餘」已含 9.30–9.38、勿重複計數。
② **〈附錄 D.3〉與〈附錄 E〉互相打架**（Grok）：D.3 明列 Part 5 子集含 **5.4 scan／5.5／5.9／5.10** 且「FA 與 paged KV 都建在這上面，不可跳」，E 階段 1 卻只放 5.1–5.3、還註明「scan 是 5.4、不在此三課」。**以 D.3 的技術判斷為準**，階段 1 補齊 5.4/5.5/5.9/5.10（階段 1：54→58 課）。
③ **本地資源假「不存在」**（Grok＋Codex＋App-Claude 三家共識，本機確認）：`tools/build.sh` 與《NVIDIA↔AMD 術語對照表》**都已存在**於課程 repo，正文卻仍寫「尚不存在／規劃中」→ 2.6／10.14／附錄 A.5 全改「已存在」，並標出 build.sh 的 `-std=c++17` 需與大綱 C++20 基準對齊。
④ **WGMMA「被 tcgen05 取代」對消費級 sm_120 是錯句**（Grok）：同課後文自己就寫「sm_120 無 TMEM/tcgen05」。改為分述——DC Blackwell 由 tcgen05 接手；**sm_120 是「WGMMA 不可用 → 改走擴充 `mma.sync`／CUTLASS／Tile」**（1.4、8.17 兩處）。
⑤ **8.16 cluster「僅 5090 可跑」漏掉 Hopper**（Grok）：cluster 是 **sm_90+**，H100/H200 雲端才是正宮實作場；同段後文「需桌機或雲端」也因此自相矛盾。已改。

**➕ 技術精確化（Gemini 六點，全採）**：
⑥ **13.7 EGM 非 CUDA 13.3 新增**——概念與早期支援始於 Grace Hopper／CUDA 12.x，13.x 為擴展。
⑦ **9.7 Triton 編譯管線越權**——Triton 的 codegen **到 PTX 為止**，`PTX→SASS` 是 `ptxas`／nvPTXCompiler 的事，原句把 SASS 畫進 Triton 管線會讓人以為 Triton 自己做 SASS 產生器。
⑧ **10.3 專業工作站卡 NVLink 已成過去式**——NVLink 橋止於 Ampere 的 RTX A6000，**Ada 世代起專業卡已移除**，只剩 PCIe P2P；v2.0.3 寫「專業 RTX/Quadro…PCIe 或 NVLink 皆可」對未來採購有誤導。
⑨ **8.15 VMM 早於 cudaMallocAsync**——VMM 是 CUDA **10.2** 的 Driver API，比 11.2 的 `cudaMallocAsync` 更早、是彈性記憶體池的基石（既然要強調版本史就該給全貌）。
⑩ **9.33 「encode」術語不明**——純 LLM 只有 prefill/decode；三段是多模態才有的 vision encode（指向 9.37）。
⑪ **12.9 `cuda.coop` 定位**——`cuda-python` 本體是 host-side 綁定，device-callable 介面在 Python 端是給 JIT 前端解析的 stub、不能單獨在 host 執行。

**🔧 Codex Medium/Low（採 4）**：
⑫ **B.6 Host GCC「最低 12」不實**——CUDA 13.3 官方支援 **GCC 6.x–15.x**，欄位改「6.x（官方支援下限）」。
⑬ **B.6 R610 太絕對**——改「**課程驗證**用 R610；13.x minor compat 與 cuTile 執行最低 **R580**、Tile 詳細 profiling 需 **580.126.09**」。
⑭ **cu128 用詞**——「標記棄用」→「**自 binary matrix 移除**」（安裝實務上「wheel 沒了」比「deprecated」重要；Grok 亦提）。
⑮ **附錄 A.2 legacy URL**——改用現行 `cuda-programming-guide/`。
⑯ **10.15 交叉引用指錯課**——roofline／machine balance 是 **8.1**，不是 1.5（1.5 是 5090 規格專題）。
⑰ **「你的 5090／4090」殘句**（Grok）——正文多處仍寫成本機已有，一律改「目標卡」。

**❌ 駁回 1 項（附查證）**：
- **Codex #3「Rubin CPX 狀態過時、應改『預計 2026 年底供應』」＝不採納**。Codex 引的是 **2025-09 AI Infra Summit 的新聞稿存檔頁**（舊稿不會下架），但查證顯示 **GTC 2026（3 月）的主要 Rubin 平台清單未再列 CPX、另新增 Groq 3 LPX；NVIDIA 並未發布「CPX 已取消」的官方聲明**。v2.0.4 現行寫法「GTC 2026 未再列出、後續定位待官方確認（『已移除』為媒體推論、非官方聲明）」**比建議的改法更準確**，維持原文。

**📌 未在本輪處理（列為 repo 同步／教材製作待辦）**：Grok P2 的前置依賴系統化、Core/Elective 全課表、最短路徑補 9.0、附錄 B.1–B.4/C 空白、長段落排版；以及 **repo 同步**（見下）。

> **【v2.0.5】** Core Path 數字、D.3/E 衝突、假斷言、WGMMA/cluster 錯句均已修，可作為封版候選。

### v2.0.6 修訂（2026-07，第十七輪：Codex／Grok 對 v2.0.5 複審，Claude 採納 6 項）

> 兩家皆判定 v2.0.5 的上輪 P0 已清、可當授課黃金版；本輪只剩小瑕疵。**課數維持 185、Core Path 138（136 必修＋2 可選）不變。**

① **Part 5 的 Core 理由過度宣稱**（Codex，**Claude 同意這是自己上輪的錯**）：v2.0.5 為對齊 D.3 而把 scan／histogram／cooperative groups 一併寫成「FA 與 paged KV 都建在這上面」。事實是 **GEMM／reduction／atomics 才是直接地基**；另三課列入 Core 的正當理由是「建立完整平行演算法能力」，不是硬前置。D.3 與 E 兩處理由已改寫（**課程內容不變、只修理由**）。
② **Core 138 含 2 堂可選**（Codex／Grok）：10.14／10.15 是跨後端選修 → 標「**136 必修＋2 可選＝138**」，差額標「47（含選修）／49（純必修）」。
③ **cu128 用詞未統一**（Codex／Grok）：A.7／B.6 仍寫「棄用」→ 統一為「**2.12 標記 deprecated 且移出標準 release matrix；2.13 明列 CUDA 12.8/12.9 builds removed**」。
④ **9.4 綁版本點**（Grok）：改「**以 2.12 官方 install matrix 為準**（2.11 起轉換、2.12 定案）」。
⑤ **階段 3 敘事易讀成雙計**（Grok）：改「**Part 9 其餘（含 9.30–9.38）**」。
⑥ **changelog 對 Rubin CPX 過度斷言**（Codex）：改「**GTC 2026 主要平台清單未再列 CPX、另新增 Groq 3 LPX；NVIDIA 未發布『已取消』官方聲明**」。（正文 1.3 的保守寫法兩家皆認可、不動。）

> **【v2.0.6 封版】** 大綱側已收斂。**課程 repo 同步為開課前置**：outline→v2.0.6、CLAUDE/README/PROGRESS/NOTE 範本對齊、`build.sh` C++17→C++20、**`resources/NVIDIA_AMD_術語與概念對照表.md` 需修 3 處**（wavefront 64 未分 CDNA/RDNA、roofline 誤指課 1.5 應為 8.1、**第 101 行仍停在 v1.9 前的錯誤基線「HIP 可在 5090 開發驗證 AMD 行為」**——大綱 10.14 早已更正）。**開課前務必完成 repo 同步**——三家一致指出：舊 repo 的 `CLAUDE.md`（v1.8/167 課/Ubuntu 24.04/誤稱本機有 5090）、`README`（v1.7.5/165 課）、`PROGRESS`（v1.8/167）、`build.sh`（C++17）、`lesson_2_3` 的 `ldconfig | grep nvptxcompiler` 假驗證、`lesson04` 把 FP4 綁 `sm_120a` 等，都會讓授課從錯誤基線開始。

---

你可以先看過這份大綱，然後告訴我：
1. 大綱結構/順序要不要調整、有沒有想增刪的主題；
2. 想從哪一課（或哪個 Part）開始正式上課；
3. 程式範例的偏好（例如：是否每課都要附 CMake 工程、是否要同時給 CUDA C++ 與 Triton 兩版、註解用中文或中英混合）。

等你決定，我們就從你指定的那一課開始，一課一課把它變成完整、圖文並茂的教材。
