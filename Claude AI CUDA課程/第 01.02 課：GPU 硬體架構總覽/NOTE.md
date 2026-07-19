# 課 1.2 — GPU 硬體架構總覽

> **Part 1 — GPU 與 CUDA 硬體基礎**　｜　難度：入門　｜　本機（sm_75）：✅ 全部可實跑
> **前置**：課 1.1（為什麼需要 GPU）
> **大綱對應**：`CUDA_AI_完整課程大綱_v2.0.6.md` 課 1.2

---

## 1. 學什麼

一顆現代 NVIDIA GPU **由什麼組成**，以及這個組成如何**直接約束你寫的 kernel**。

課 1.1 回答「為什麼要用 GPU」，這課回答「GPU 裡面長什麼樣」。但重點不是背規格表——
而是建立一條可以隨時自己驗證的路徑：**從硬體階層 → 資源預算 → 我的 kernel 能開多少 thread**。

你的 EDA 背景在這裡是直接可轉移的：SM 之於 GPU，很像 core 之於多核 CPU，
但**資源分配是靜態的、編譯期就決定的**——這點和 CPU 的動態亂序執行差很多，也是本課的核心。

---

## 2. 核心概念

### 2.1 階層：GPU → GPC → TPC → SM → CUDA Core

由外往內四層，但**只有 SM 那層是你寫程式時真正碰得到的**：

| 層級 | 是什麼 | 你在 CUDA 裡碰得到嗎 |
|---|---|---|
| **GPU** | 整顆晶片 | ✅ `cudaSetDevice` |
| **GPC**（Graphics Processing Cluster） | 一組 TPC + 光柵化資源 | ❌ 完全查不到 |
| **TPC**（Texture Processing Cluster） | 通常 = 2 個 SM | ❌ 完全查不到 |
| **SM**（Streaming Multiprocessor） | **執行單位**：block 被派到這裡 | ✅ `multiProcessorCount`、`%smid` |
| **CUDA Core** | SM 內的 FP32 ALU | ⚠️ 數量只能查表推導 |

> **⚠️ 踩雷（本課最重要的一句）**：**GPC/TPC 對 CUDA 程式設計幾乎不存在。**
> 它們是繪圖管線與晶片切割（良率、產品分級）的單位。談運算時只要記住 **SM 是排程單位**。
> 網路文章常把 GPC 講得好像會影響 kernel 效能——不會。真正影響你的是 SM 內的資源預算。

### 2.2 SM 裡面有什麼

以 Turing（本機）為例，一個 SM 被切成 **4 個 processing block**，每個各有自己的
warp scheduler 與暫存器分區：

- **warp scheduler**（每 SM 4 個）：每個 cycle 挑一個 **ready 的 warp** 發指令
- **register file**：本機每 SM **65536 個 32-bit 暫存器（256 KB）**——比 L1 還大，這是 GPU 的特色
- **CUDA core（FP32 ALU）**：Turing 每 SM 64 個；**Ada/Blackwell 是 128**
- **INT32 datapath**：Turing 起與 FP32 分離 → 整數位址計算可與浮點運算**同時**進行
- **Tensor Core**：Turing 每 SM 8 個（第 2 代）
- **RT Core**：每 SM 1 個（做光線追蹤，AI 用不到）
- **L1 / Shared memory**：**同一塊 SRAM 切出來的**，比例可調

> **⚠️ 踩雷**：「每個 SM 有 64 個 CUDA core」這句話**只對 Turing/Volta 成立**。
> Ampere GA10x、Ada、Blackwell 都是 128。任何硬編這個數字的程式，換卡就算錯。
> 本課程式碼一律用 `cudaGetDeviceProperties` 動態查、查不到的就誠實標明是查表推導。

### 2.3 記憶體階層：越近越小越快

```
暫存器      每 SM 256 KB    ~1 cycle      每個 thread 私有
Shared/L1   每 SM 96 KB     ~30 cycles    block 內共享（你手動管理）
L2          全晶片 4 MB     ~200 cycles   所有 SM 共享
Global(GDDR6) 8 GB          ~400+ cycles  384 GB/s
```

跟 CPU 最大的差別有兩個：

1. **暫存器檔比 L1 還大**（256 KB vs 96 KB）。CPU 是幾百 bytes 的暫存器 + 幾十 KB L1。
   GPU 把電晶體砸在暫存器上，是為了**讓幾十個 warp 的狀態同時活著**——這樣才能靠切換藏延遲。
2. **Shared memory 是「你自己管的 cache」**。CPU 的 L1 你只能影響、不能指定；
   GPU 的 shared memory 是明確的 scratchpad，要放什麼、什麼時候放，全由你的程式決定。

### 2.4 latency hiding：為什麼要「常駐很多 warp」

SM 不會等一個 warp 的記憶體讀取完成才做事。它的做法是：

```
warp 0 發出 load → 等記憶體（400 cycles）
   ↓ 排程器立刻換人
warp 1 算 FMA → warp 2 算 FMA → warp 3 發 load → …
   ↓ 400 cycles 後
warp 0 的資料回來了 → 重新變成 ready，排回去
```

**warp 切換是零成本的**——因為每個 warp 的暫存器**一直留在暫存器檔裡**，不用像 CPU
那樣存/回存 context。代價就是暫存器檔必須夠大。

於是關鍵指標叫 **occupancy**＝「實際常駐 warp 數 ÷ SM 上限」。
本機 SM 上限是 **32 個 warp（1024 threads）**。常駐 warp 越多，能藏的延遲越多。

### 2.5 資源預算：三條會先撞到的牆

一個 block 要進駐 SM，必須**同時**滿足三個條件。**任一項先用完，occupancy 就卡在那裡**：

| 資源 | 本機每 SM 的量 | 撞牆的樣子 |
|---|---|---|
| **暫存器** | 65536 個 | 每 thread 用超過 64 個 → 塞不滿 1024 threads |
| **Shared memory** | 64 KB（可配置上限） | tile 開太大 → 常駐 block 數變少 |
| **block 數 / warp 數** | 16 blocks / 32 warps | block 開太小（如 32 threads）→ 被 block 數上限卡死 |

那個「**64 個暫存器**」是這樣來的：`65536 ÷ 1024 threads = 64`。
這是本課從規格表跨到實務的第一個真正有用的數字——下面的實驗 B 會把它撞給你看。

> **⚠️ occupancy 不是越高越好**。100% occupancy 不等於最快：有時每 thread 多用暫存器、
> 降到 50% occupancy，反而因為減少了記憶體往返而更快（經典案例是 Volkov 的
> "Better Performance at Lower Occupancy"）。本課先建立「怎麼算」，Part 5 才談「怎麼取捨」。

---

## 3. 動手做

### 3.1 程式一：把硬體階層印出來

完整版：[`src/gpu_topology.cu`](src/gpu_topology.cu)

這支的設計重點是**把資訊分成三種可信度**——這個習慣會讓你日後不被規格文章誤導：

```cuda
// [查詢] CUDA runtime 直接給的 → 一定正確
// [推導] 由查詢值換算的       → 正確，但依賴換算公式
// [白皮書] CUDA 問不到的      → 程式無法驗證，只能查架構白皮書

static int fp32CoresPerSM(int major, int minor)
{
    switch (major) {
        case 7:  return 64;                       // Volta / Turing
        case 8:  return (minor == 0) ? 64 : 128;  // A100 是 64；8.6/8.9 是 128
        case 9:  return 128;                      // Hopper
        case 10:
        case 12: return 128;                      // Blackwell
        default: return -1;                       // 未知世代 → 誠實回報不知道
    }
}
```

⚠️ 承課 1.1 的踩雷：`p.clockRate` 與 `p.memoryClockRate` 在 **CUDA 13.x 已從
`cudaDeviceProp` 移除**，必須改走 attribute：

```cuda
int smClockKHz = 0, memClockKHz = 0;
cudaDeviceGetAttribute(&smClockKHz,  cudaDevAttrClockRate,       dev);
cudaDeviceGetAttribute(&memClockKHz, cudaDevAttrMemoryClockRate, dev);
```

**編譯與執行**：

```bash
cd "Claude AI CUDA課程/第 01.02 課：GPU 硬體架構總覽"
nvcc -std=c++20 -O2 -arch=native src/gpu_topology.cu -o /tmp/gpu_topology && /tmp/gpu_topology

# 或用課程工具（會一次驗 sm_75 + sm_89 + sm_120）
../../tools/check_build.sh --lesson "第 01.02 課：GPU 硬體架構總覽"
```

**實際輸出**（本機 Quadro RTX 4000 with Max-Q Design / sm_75 / CUDA 13.3）：

```
══════════════════════════════════════════════════════════
 GPU 0：Quadro RTX 4000 with Max-Q Design
 Compute Capability 7.5（sm_75）
══════════════════════════════════════════════════════════

① 晶片層級
   [查詢] SM 數量                : 40
   [查詢] SM 時脈                : 1380 MHz
   [查詢] L2 cache               : 4.0 MB（全晶片共用）
   [推導] FP32 CUDA core 總數    : 2560（40 SM × 64/SM）
   [白皮書] GPC / TPC 數量        : CUDA 查不到，需查架構白皮書

② 記憶體子系統
   [查詢] 全域記憶體             : 7.58 GB
   [查詢] 匯流排寬度             : 256 bit
   [查詢] 記憶體時脈             : 6001 MHz
   [推導] 理論頻寬               : 384.1 GB/s
   [推導] 記憶體控制器           : 8 組 ×32-bit
   [查詢] L2 cache               : 4.0 MB

③ 單一 SM 內部資源
   [查詢] warp 大小              : 32 threads
   [查詢] 暫存器檔（每 SM）      : 65536 個 32-bit（= 256 KB）
   [查詢] 暫存器（每 block 上限）: 65536 個
   [查詢] Shared memory（每 SM） : 64 KB
   [查詢] Shared memory（每 block 預設上限）: 48 KB
   [查詢] 常駐 thread 上限       : 1024（= 32 warps）
   [查詢] 常駐 block 上限        : 16
   [查詢] 單一 block thread 上限 : 1024
   [白皮書] warp scheduler / SM   : CUDA 查不到（Turing 及之後為 4）
   [白皮書] Tensor Core 世代      : 第 2 代（Turing，加 INT8/INT4）

④ 資源預算（決定 occupancy 的三個上限）
   要讓 32 個 warp 全部同時常駐在一個 SM，平均每個 thread 只能用：
     暫存器 ≤ 64 個 / thread
   若每個 block 開 256 threads（= 8 warps），要塞滿 SM 需 4 個 block，
     此時每個 block 平均只能用 shared memory ≤ 16.0 KB
   → 暫存器、shared memory、block 數，任一項先用完，occupancy 就卡在那裡。

⑤ 容易誤解的能力旗標
   [查詢] L1 與 shared memory 同一塊 SRAM : 是（可切分比例）
   [查詢] 統一定址 (UVA)          : 支援
   [查詢] 並行 kernel             : 支援
   [查詢] 非同步引擎（複製引擎）  : 3 條
   [查詢] ECC                     : 關閉
   [查詢] 是否 integrated（非獨顯）: 否
```

**解讀重點**：

- **40 SM × 64 core = 2560**，配 1380 MHz → 課 1.1 算出的 7.07 TFLOPS 就是從這裡來的
  （2560 × 2 FLOP/FMA × 1.38 GHz）。兩堂課的數字**互相對得上**，不是各講各的。
- **48 KB vs 64 KB**：每 block 預設只給 48 KB shared memory，但每 SM 有 64 KB。
  要用滿 64 KB 必須明確 opt-in（`cudaFuncSetAttribute` 設
  `cudaFuncAttributeMaxDynamicSharedMemorySize`）——這是刻意的相容性設計，
  免得舊程式一升級就佔光資源。
- **ECC 關閉**：消費級／行動工作站卡多半如此。訓練長時間跑大模型時這是要知道的風險項。
- **非同步引擎 3 條**：可以同時做「H2D 複製 + kernel 執行 + D2H 複製」→ Part 6 的
  stream overlap 就靠這個。

### 3.2 程式二：讓 SM 與資源上限「看得見」

完整版：[`src/sm_residency.cu`](src/sm_residency.cu)

規格印出來只是文字。這支做三個實驗，把規格變成**你眼睛看得到的行為**。

**實驗 A — 問硬體「我在哪個 SM 上？」**

`%smid` 是 PTX 的特殊暫存器，沒有 C++ API，只能用 inline PTX 讀：

```cuda
__global__ void reportSM(unsigned int* smIdOut)
{
    if (threadIdx.x == 0) {
        unsigned int smid;
        asm volatile("mov.u32 %0, %%smid;" : "=r"(smid));
        smIdOut[blockIdx.x] = smid;
    }
}
```

> ⚠️ `%smid` 是**觀測工具，不是可依賴的介面**。block 一旦排上去就不會遷移，
> 但這個值不保證跨 launch 穩定，**絕不可拿來寫演算法邏輯**。

**實驗 B — 暫存器壓力**：N 個互不相依的累加器全展開，逼編譯器配 N 份暫存器。
`cudaFuncGetAttributes` 可以問出 kernel **實際用了幾個暫存器**、有沒有 spill。

**實驗 C — shared memory**：對同一個 kernel 掃不同的 dynamic shared memory 用量，
用 `cudaOccupancyMaxActiveBlocksPerMultiprocessor` 算每 SM 能常駐幾個 block。

**實際輸出**（本機 sm_75）：

```
裝置：Quadro RTX 4000 with Max-Q Design（sm_75）｜SM 數 40｜每 SM 上限 1024 threads（32 warps）

═══ 實驗 A：160 個 block 落在哪些 SM ═══
   實際出現的 SM 編號範圍：0 – 39
   有拿到 block 的 SM 數  ：40 / 40
   每個 SM 拿到的 block 數：min=4  max=4
   前 8 個 SM 的分佈      ：SM0=4 SM1=4 SM2=4 SM3=4 SM4=4 SM5=4 SM6=4 SM7=4
   → block 不是排隊等一個 SM，而是被撒開；這就是 GPU 的 scale-out 單位。

═══ 實驗 B：暫存器用量如何壓低 occupancy（block=256 threads）═══
   每個 SM 共有 65536 個 32-bit 暫存器；每個 thread 用得越多，能常駐的 warp 越少。
   regPressure<4>         暫存器/thread=16   local=0     B  → block/SM=4   warp/SM=32   occupancy=100%
   regPressure<32>        暫存器/thread=63   local=0     B  → block/SM=4   warp/SM=32   occupancy=100%
   regPressure<96>        暫存器/thread=128  local=0     B  → block/SM=2   warp/SM=16   occupancy=50%
   regPressure<128>       暫存器/thread=168  local=0     B  → block/SM=1   warp/SM=8    occupancy=25%
   regPressure<256>       暫存器/thread=255  local=608   B  → block/SM=1   warp/SM=8    occupancy=25%
   （五個 kernel 皆已實際執行成功）

═══ 實驗 C：shared memory 是另一條獨立上限（block=256 threads）═══
   每個 SM 的 shared memory：64 KB
   dynamic shared =  4 KB/block  → block/SM=4   shared 實際用掉  16 KB/SM
   dynamic shared =  8 KB/block  → block/SM=4   shared 實際用掉  32 KB/SM
   dynamic shared = 16 KB/block  → block/SM=4   shared 實際用掉  64 KB/SM
   dynamic shared = 32 KB/block  → block/SM=2   shared 實際用掉  64 KB/SM
   dynamic shared = 48 KB/block  → block/SM=1   shared 實際用掉  48 KB/SM

全部實驗完成。
```

### 3.3 觀察與解讀（這才是本課的重點）

**實驗 A：完美均分 4/4/4…**
160 個 block 撒到 40 個 SM，每個剛好 4 個。這不是巧合——block 少、資源夠、
排程器就 round-robin 撒開。**推論**：想吃滿 GPU，grid 的 block 數至少要 ≥ SM 數，
最好是好幾倍（讓排程器有東西可以填空檔）。開 8 個 block 的 grid，
就是讓 40 個 SM 裡的 32 個閒著。

**實驗 B：兩個轉折點，一個比一個嚴重**

| 暫存器/thread | occupancy | 發生了什麼 |
|---|---|---|
| 16 → 63 | 100% | 還在預算（≤64）內，沒事 |
| **128** | **50%** | 超過 64 → 每 SM 只塞得下 2 個 block |
| **168** | **25%** | 再降一級 |
| **255 + local=608 B** | 25% | 🔴 **register spilling**：暫存器撞到硬上限 255 |

第二個轉折是致命的：`local=608 B` 代表放不下的變數被搬到 **local memory**——
這個名字騙很大，**它其實住在 global memory 裡**。原本 ~1 cycle 的暫存器存取
變成 ~400 cycles 的記憶體往返。看到 `local≠0` 就要警覺。

`regs=255` 也不是巧合：**Turing 每 thread 最多 255 個暫存器**，撞到天花板後
編譯器只能把剩下的往記憶體丟。

**實驗 C：shared memory 是完全獨立的另一條路**
4/8/16 KB 都是 4 個 block（被 **1024 threads 上限**卡住，不是被 shared memory 卡住），
一到 32 KB 就掉成 2 個。看「shared 實際用掉」那欄很清楚：
16 KB×4 = 64 KB 剛好用滿，再多就只能減 block。

這正是**寫 tiling kernel 時的核心取捨**：tile 開大 → 少跑幾趟 global memory，
但常駐 warp 變少 → 能藏的延遲變少。Part 5 會把這個取捨量化成公式。

---

## 4. 圖解

### 4.1 階層樹（本機 Quadro RTX 4000 / TU104）

```
GPU：TU104 晶片
│  L2 cache 4 MB（全晶片共用）＋ 8 組 32-bit 記憶體控制器 → 256-bit → 384 GB/s
│
├─ GPC ×6 ─────────────────── ❌ CUDA 查不到（繪圖/切割單位，與 kernel 無關）
│   └─ TPC ×4 ─────────────── ❌ CUDA 查不到（通常 = 2 SM）
│       └─ SM ×2 ─────────── ✅ 排程單位！block 被派到這裡
│
│  （TU104 完整版 6×4×2 = 48 SM；本卡啟用 40 SM，其餘關閉 → 產品分級）
│
└─ 單一 SM 內部（放大看）
   ┌──────────────────────────────────────────────────┐
   │ Register File 65536 × 32-bit ＝ 256 KB            │ ← 比 L1 還大！
   ├──────────────────────────────────────────────────┤
   │ processing block ×4，每個含：                     │
   │   • warp scheduler ×1  ← 每 cycle 挑一個 ready warp│
   │   • FP32 core ×16      ← 4×16 = 64/SM（Turing）   │
   │   • INT32 單元 ×16     ← 與 FP32 可並行            │
   │   • Tensor Core ×2     ← 4×2 = 8/SM（第 2 代）     │
   ├──────────────────────────────────────────────────┤
   │ L1 / Shared memory  96 KB（同一塊 SRAM，比例可調） │
   │   └─ shared 最多 64 KB（每 block 預設上限 48 KB）  │
   ├──────────────────────────────────────────────────┤
   │ RT Core ×1（光線追蹤，AI 用不到）                  │
   └──────────────────────────────────────────────────┘
      常駐上限：1024 threads = 32 warps = 最多 16 blocks
```

### 4.2 一個 block 進駐 SM 要通過的三道檢查

```
     你的 kernel launch：<<< grid, 256 threads, shmem >>>
                    │
                    ▼
        ┌───── SM 的資源櫃檯 ─────┐
        │                          │
   ① 暫存器夠嗎？   65536 − 已用 ≥ 256 × (每 thread 暫存器數)
   ② shared 夠嗎？  64 KB  − 已用 ≥ 本 block 需要的量
   ③ 名額夠嗎？     warp ≤ 32 且 block ≤ 16
        │                          │
        └──── 三項全過 → 進駐 ─────┘
                    │
              任一項不過 → 在 queue 裡等，
              等到 SM 上有 block 退出才輪到你
```

**這三道檢查是編譯期＋launch 時就決定的靜態分配**，跑起來之後不會再變動——
這和 CPU 的動態亂序執行、硬體自動 cache 管理是完全不同的心智模型。

---

## 5. 本機 vs 目標平台

| 項目 | 本機 Quadro RTX 4000（sm_75） | RTX 5090（sm_120） | 說明 |
|---|---|---|---|
| SM 數 | **40**（實測） | 170（官方規格，待實測） | 4.25× |
| FP32 core / SM | **64**（Turing） | 128（Blackwell） | **這就是不能硬編的原因** |
| FP32 core 總數 | **2560**（實測推導） | 21760（官方規格，待實測） | 8.5× |
| 暫存器 / SM | **65536**（實測） | 65536（各代皆同，待實測） | 這個數字從 Volta 起沒變過 |
| Shared+L1 / SM | **96 KB**（白皮書） | 128 KB（待實測確認） | Ada 起放大 |
| L2 | **4 MB**（實測） | 待實測 | Ada/Blackwell 大幅加大 |
| 記憶體 | 8 GB GDDR6 / 256-bit / **384 GB/s**（實測） | 32 GB GDDR7 / 512-bit / ~1792 GB/s（官方規格，待實測） | ~4.7× 頻寬 |
| Tensor Core | 第 2 代（FP16/INT8/INT4） | 第 5 代（加 FP4） | 課 8.x 會用到 |
| thread block cluster | ❌ 不支援 | ❌ 不支援（需 sm_90+） | **消費級卡沒有**，別誤信 |

> **⚠️ 誠實邊界**：5090 那一欄除了「編得過」（本課兩支程式已用
> `check_build.sh` 對 sm_75/sm_89/sm_120 三架構全部編譯通過）之外，
> **所有數字都是官方規格或推測，本機無實體卡、未經執行驗證**。
> 拿到卡的第一件事就是跑 `gpu_topology`，把這張表的「待實測」全部換成實測值。

---

## 6. 自我檢核

完成這課後，你應該能回答：

- [ ] 為什麼 GPC/TPC 對 CUDA 程式設計幾乎不重要，而 SM 很重要？
- [ ] 為什麼 GPU 的暫存器檔（256 KB）比 L1（96 KB）還大？這個設計服務什麼目的？
- [ ] 本機每 thread 只能用 64 個暫存器才能達到 100% occupancy——這個 64 怎麼算出來的？
- [ ] `local memory` 為什麼是效能陷阱？看到 `cudaFuncGetAttributes` 回報 `localSizeBytes > 0` 要怎麼辦？
- [ ] 為什麼「每 SM 64 個 CUDA core」這句話是錯的（至少是不完整的）？
- [ ] grid 只開 8 個 block，在這台 40 SM 的機器上會發生什麼事？
- [ ] shared memory 每 block 預設上限 48 KB，但每 SM 有 64 KB——為什麼要這樣設計？

---

## 7. 踩雷紀錄（本課實測遇到的）

**① 「暫存器壓力」實驗第一版根本沒壓出效果**

我原本用 N = 4 / 16 / 64 三組，結果**三組全是 100% occupancy**——因為本機
每 thread 的預算剛好是 64 個暫存器，`regPressure<64>` 只用到 64，**剛好卡在邊界上沒超過**。
教學上等於什麼都沒證明。實測掃過 N = 4/32/64/96/128/192/256 才找到真正的轉折點
（96 → 掉到 50%、256 → 開始 spill），最後選了能同時呈現**兩個轉折**的組合。

**教訓**：設計「示範某現象」的實驗時，一定要先實跑掃描確認現象真的會出現，
不能憑推理就把數字寫進教材。

**② `cudaDeviceProp::clockRate` 在 CUDA 13.x 已移除**（承課 1.1）

舊教材幾乎都寫 `p.clockRate`，13.x 會編不過。必須改用
`cudaDeviceGetAttribute(&v, cudaDevAttrClockRate, dev)`。本課兩支程式都已用新寫法。

**③ 規格數字要標清楚「從哪來」**

寫這課時我一度想直接把「6 GPC / 4 TPC」寫成程式輸出——但 **CUDA 根本查不到這兩個值**。
硬寫就變成把白皮書的數字偽裝成實測。最後改成程式明確印出
「`[白皮書] GPC / TPC 數量：CUDA 查不到`」，並在輸出中用 `[查詢]/[推導]/[白皮書]`
三種標籤區分可信度。這個習慣後面每一課都會延用。

---

## 8. 延伸參考

- **PMPP 5e Ch.4**（Compute Architecture and Scheduling）——本課的教科書對應章節
- [CUDA C++ Programming Guide §5.2 Hardware Multithreading](https://docs.nvidia.com/cuda/cuda-c-programming-guide/)
- [CUDA C++ Programming Guide §K Compute Capabilities](https://docs.nvidia.com/cuda/cuda-c-programming-guide/)——各代每 SM 的資源上限對照表
- [NVIDIA Turing 架構白皮書](https://www.nvidia.com/content/dam/en-zz/Solutions/design-visualization/technologies/turing-architecture/NVIDIA-Turing-Architecture-Whitepaper.pdf)——本機這顆 TU104 的 GPC/TPC/SM 配置圖
- [NVIDIA Blackwell 架構白皮書](https://www.nvidia.com/en-us/data-center/technologies/blackwell-architecture/)——未來桌機
- Volkov, *Better Performance at Lower Occupancy*（GTC 2010）——理解「occupancy 不是越高越好」的經典

**下一課**：課 1.3 — NVIDIA GPU 架構演進史（Tesla → Blackwell，每代解決了什麼問題）

---

**我證明了什麼**：用 `%smid` 實測 160 個 block 均勻落在全部 40 個 SM 上；用暫存器壓力掃描
實測出本機 occupancy 的兩個轉折點（128 regs → 50%、255 regs → spill 608 B），
驗證「65536 ÷ 1024 = 64 暫存器/thread」這條資源預算公式；並確立
`[查詢]/[推導]/[白皮書]` 三級可信度標註習慣。
