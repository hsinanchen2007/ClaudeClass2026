# 第 01.02 課：GPU 硬體架構總覽

> **授課軌**：Codex AI CUDA 課程<br>
> **所屬章節**：Part 1 — GPU 與 CUDA 硬體基礎<br>
> **大綱版本**：v2.0.6<br>
> **難度**：入門；重點是建立後續 kernel、memory、occupancy 課程共用的硬體地圖<br>
> **本機驗證**：Dell Precision 7550、Quadro RTX 4000 Mobile（Turing，sm_75，8 GB）、CUDA 13.3、GCC 15<br>
> **前置知識**：完成課 1.1；知道 latency、throughput、SIMT 與 warp 的基本意思

---

## 0. 本課目標

「GPU 有很多核心」不足以指導 CUDA 程式。你還必須知道工作由誰排程、資料放在哪裡，以及哪些數量是程式可查的資源，哪些只是特定產品白皮書中的實體配置。

本課結束後，你應該能：

1. 分開畫出 **CUDA 程式模型**與 **GPU 實體實作**兩張圖。
2. 解釋 GPU、GPC、TPC、SM、warp 與 thread 的關係，不把它們混成同一種「核心」。
3. 說明 CUDA Core、Tensor Core、RT Core、warp scheduler 各自做什麼。
4. 畫出 register、shared/L1、L2、global DRAM 與 host memory 的階層。
5. 解釋 register/shared memory 為何會限制 resident blocks、warps 與 occupancy。
6. 用 CUDA Runtime API 查出本機可觀測資源，並知道 API **沒有**告訴你的內容。
7. 正確解讀本課 runtime probe，而不把一次排程觀察誤寫成永久硬體保證。

---

## 1. 先分開兩張地圖

### 1.1 程式設計者看到的 CUDA 階層

```text
kernel launch
└── grid
    ├── thread block 0
    │   ├── warp 0 = threads 0..31
    │   ├── warp 1 = threads 32..63
    │   └── ...
    ├── thread block 1
    └── ...
```

- **Grid** 是一次 kernel launch 的全部 blocks。
- **Thread block** 是可合作、可用 shared memory、可做 block-level synchronization 的 threads 群組。
- NVIDIA GPU 會把 block 內連續的 threads 分成每組 32 threads 的 **warp**。
- **Warp** 是 SM 的主要指令排程單位；thread 是 CUDA 程式模型中的邏輯工作單位。

一個 block 的 threads 在同一時間由一個 SM 執行，因此能共用該 SM 上分配給 block 的 shared memory。不同 blocks 的排程順序沒有一般性保證；可攜 kernel 不應假設 block 0 一定先於 block 1 完成。

### 1.2 晶片白皮書常畫的實體階層

下面是**概念圖**，不是所有世代都具備相同數量或完全相同連線：

```text
GPU
├── 前端 / work distributor
├── GPC 0 (Graphics Processing Cluster)
│   ├── raster / graphics fixed-function units
│   ├── TPC 0 (Texture Processing Cluster)
│   │   ├── geometry / texture-related units
│   │   ├── SM 0 (Streaming Multiprocessor)
│   │   └── SM 1                         [數量依架構]
│   ├── TPC 1
│   └── ...
├── GPC 1
├── ...
├── GPU-wide L2 cache / interconnect
├── memory controllers
└── device DRAM (GDDR/HBM)

SM
├── warp schedulers / dispatch units
├── FP/INT execution pipelines（常被產品規格統稱 CUDA Cores）
├── Tensor Cores
├── architecture-specific fixed/special function units（例如 RT/SFU）
├── register file
└── unified L1 data cache / shared-memory resources
```

兩個重要邊界：

1. **GPC/TPC 是實體產品的組織方式，不是一般 CUDA launch syntax。** 常規 kernel 指定 grid/block，不指定「請跑在 TPC 2」。
2. **每 GPC 有幾個 TPC、每 TPC 有幾個 SM、每 SM 有幾條 pipeline，都可能跨架構改變。** 例如 Turing TU102 與 RTX Blackwell GB202 白皮書雖都畫出每 TPC 兩個 SM，但 GPC 內的 TPC 數和 SM 內部資源不同，不能把某一張圖當成所有 NVIDIA GPU 的固定規格。

Compute Capability 9.0+ 的 thread block cluster 是進階例外：同 cluster 的 blocks 會被安排在同一 GPC，讓它們使用 cluster synchronization 與 distributed shared memory。本機 sm_75 不支援；正式內容留到課 8.16。

---

## 2. 每個硬體名詞到底代表什麼

| 元件 | 角色 | 最常見誤解 |
|---|---|---|
| **SM** | 接收 blocks、建立 warps、保存 thread state、排程指令並提供 register/shared/L1 與執行單元 | 把一個 SM 當成一個 CUDA Core |
| **Warp scheduler** | 從 ready warps 中選擇下一個可發射指令的 warp | 以為一個 CUDA thread 對應一個獨立硬體 scheduler |
| **CUDA Core** | NVIDIA 產品規格對主要 scalar arithmetic lanes 的簡稱；精確 pipeline 與吞吐量依世代/資料型別而異 | 當成能獨立跑 OS thread、具完整 cache/control 的 CPU core |
| **Tensor Core** | 執行矩陣乘加（MMA）與相關低精度/混合精度資料路徑 | 一般 `float a+b` 會自動使用 Tensor Core |
| **RT Core** | 加速 ray traversal、bounding-volume hierarchy 與 intersection 類工作 | 可直接拿來增加一般 CUDA 或 AI scalar throughput |
| **Register file** | SM 上保存大量 thread-local 32-bit registers | registers 多就一定快；忽略 occupancy 與 spill |
| **Shared memory** | programmer-managed、on-chip、主要由同 block threads 合作使用 | 以為所有 blocks 或所有 SM 都能直接共用 |
| **L1 cache** | 每 SM 的 data cache；現代架構常與 shared memory 共用實體資源/可調 carveout | 把 Runtime 回報的 max shared bytes 當成整個 L1/shared 實體容量 |
| **L2 cache** | GPU-wide cache，所有 SM 與多種引擎共享 | 以為 L2 屬於某一個 block |
| **Memory controller** | 把 L2/interconnect 的請求送往 GDDR/HBM channels | 只看 VRAM 容量，不看 bus width、clock、存取模式 |

### 2.1 「CUDA Core」不是 CUDA 的排程單位

CUDA Runtime 排程的是 grids、blocks、warps 與 threads；它不提供「把 thread 綁到第 37 個 CUDA Core」的介面。產品標示的 CUDA Core 數量可用來理解峰值硬體規模，卻不能直接當成有效平行度 $P$：

- warp 是否 ready 受資料依賴與 memory latency 影響。
- 一條 instruction 會走哪種 pipeline，取決於資料型別與指令。
- 不同世代每 SM 的 lanes、每 cycle issue 能力與 FP/INT 共用方式不同。
- branch divergence、低 occupancy、memory bottleneck 都可能讓 lanes 閒置。

因此比較 GPU 時，不能只比較「CUDA Core 數」。後續會搭配 clock、instruction throughput、memory bandwidth、occupancy 與實測 profiler 指標。

### 2.2 Tensor Core 與 RT Core 不是免費加速

Tensor Core 要透過支援的 library、compiler lowering、WMMA/MMA/PTX 或高階框架路徑才能真正使用。資料形狀、對齊、dtype 與累加型別也會影響能否走 Tensor Core。

RT Core 是 ray tracing 專用固定功能資源。它對 OptiX、DXR、Vulkan ray tracing 等工作有價值，但不會因為模型是「AI」就自動參與 GEMM。這兩種專用單元正好呼應課 1.1 的結論：專用硬體只在工作模式吻合時提供高效率。

---

## 3. 一個 block 如何在 SM 上執行

```text
CPU launch grid
       │
       v
GPU 將可執行 block 分配給有容量的 SM
       │
       v
SM 把 block 線性 thread IDs 切成 32-thread warps
       │
       v
warp scheduler 從 ready warps 選擇並發射指令
       │
       ├── arithmetic / load-store / special pipelines
       ├── register operands
       └── shared/L1/L2/DRAM memory path
```

一個 SM 通常同時保存多個 blocks 和多個 warps。當某 warp 等待資料或 dependency，scheduler 可以發射另一個 ready warp，以 **thread-level parallelism** 隱藏 latency。這不是 CPU 式「把 thread context 從記憶體換入換出」；一般 resident warp 的 program counter 與 register state 已在晶片上。

### 3.1 Residency 與 occupancy 的資源帳

一個 kernel 能在每個 SM 同時放多少 blocks，至少受以下上限共同約束：

```text
active blocks per SM = min(
    architecture block-count limit,
    thread/warp limit,
    register file / registers needed per block,
    shared memory / shared bytes needed per block,
    other architecture/kernel constraints
)
```

例如每 thread 使用 64 個 32-bit registers、每 block 256 threads，單一 block 的理想化 register 需求就是 $64\times256=16,384$ 個 registers；實際 allocation 還會依架構粒度向上取整。增加 registers/thread 可能降低 resident blocks；強行壓低 registers 又可能造成 spill。

**Occupancy** 常指 active warps 相對架構最大 resident warps 的比例。它是 latency-hiding 的條件之一，不等於：

- CUDA Core utilization。
- memory bandwidth utilization。
- 指令吞吐量。
- 最終效能分數。

100% occupancy 的低效率 kernel 仍可很慢；某些高 register、高資料重用 kernel 在較低 occupancy 也可能更快。

---

## 4. 記憶體階層與可見範圍

| 層級 | 主要範圍 | 管理方式 | 本課應保留的直覺 |
|---|---|---|---|
| Register | thread | compiler | 最靠近 execution pipeline；數量有限，會限制 residency |
| Shared memory | block（新架構可擴至 cluster） | programmer | on-chip scratchpad；快但需正確同步與避免 bank conflicts |
| L1 data cache | SM | hardware + carveout policy | 每 SM；常與 shared memory 共用 unified data cache 資源 |
| L2 cache | 整顆 GPU | hardware / 部分 policy API | 所有 SM 共享，是前往 DRAM 前的重要層級 |
| Global memory | device | programmer/runtime | 大容量 GDDR/HBM；高頻寬但高 latency，存取合併與重用很重要 |
| Host memory | CPU/system | OS/runtime | 離散 GPU 通常經 PCIe；傳輸不能從 kernel time 中消失 |

### 4.1 三個常見名稱陷阱

1. **Local memory 不代表 on-chip 或「靠近 thread」。** CUDA 的 local address space 是每 thread 私有的位址語意，實體通常由 device memory 支撐並經 cache；register spill 常落到 local memory。
2. **Shared memory 的 shared 是 block scope。** 一般 blocks 不能靠同一塊 shared memory 互傳；cluster distributed shared memory 是較新且明確 opt-in 的功能。
3. **Unified Memory 不等於一顆物理記憶體。** 它統一程式可見位址與 migration/access 機制；資料放置和搬移成本仍存在。

### 4.2 理論 DRAM 頻寬

本課探針使用 Runtime 回報的 peak memory clock 與 bus width，對本機 GDDR6 做下列估算：

\[
BW_{peak}=2\times f_{memory}\times\frac{bus\ width}{8}
\]

本機為：

\[
2\times 6001\ \text{MHz}\times\frac{256}{8}=384.064\ \text{GB/s}
\]

這是理論峰值，不是 application 可持續頻寬。協定 overhead、存取合併、ECC/partition 使用、clock state、讀寫混合與 kernel 行為都會讓實測不同；課 8.5 會正式量測。

---

## 5. 黃金筆電：哪些是 API 觀察，哪些是白皮書推導

### 5.1 CUDA Runtime 實測

以下為 2026-07-19 在本機 CUDA 13.3 的結果：

| 項目 | 實測值 | 意義 |
|---|---:|---|
| Device | Quadro RTX 4000 with Max-Q Design | driver/runtime 回報名稱 |
| Compute capability | 7.5 (`sm_75`) | Turing CUDA target |
| Active SM count | 40 | `cudaDevAttrMultiProcessorCount` |
| Warp size | 32 | NVIDIA CUDA warp lanes |
| Max resident threads / SM | 1,024 | 架構上限，不是目前 active threads |
| Max resident blocks / SM | 16 | 架構上限，仍受其他資源限制 |
| 32-bit registers / SM | 65,536（256 KiB） | 全部 resident warps/blocks 分配 |
| Max shared memory / SM | 65,536 bytes | 可分配 shared 上限，不是完整 unified L1/shared 實體容量 |
| Max shared / block | 49,152 bytes default；65,536 opt-in | 較大 dynamic shared 要正確 opt-in |
| L2 cache | 4,194,304 bytes（4 MiB） | GPU-wide |
| Memory bus | 256-bit | GDDR interface width |
| 理論 peak bandwidth | 384.064 GB/s | 由 6001 MHz 與 256-bit 估算 |
| Async engine count | 3 | Runtime 回報的 async copy engine 能力數；不要直接推論所有 copy 都能三路滿速 |
| Concurrent kernels | 1 | 布林能力：支援 concurrent kernels；不是「只能跑 1 個」 |

Runtime 顯示 `device.global_memory_mib=7761.375`，而 `nvidia-smi` 顯示實體 framebuffer 8,192 MiB、其中 431 MiB reserved。兩者不矛盾：CUDA 程式看到的是目前可供該 device/runtime 使用的總量，不應用產品包裝容量替代 allocation check。

### 5.2 由 Turing 白皮書推導的 execution-unit 數

CUDA Runtime 不直接回報 CUDA/Tensor/RT Core 數。NVIDIA Turing 白皮書則說每個 Turing SM 有：

- 64 FP32 Cores（產品規格通常稱 CUDA Cores）。
- 64 INT32 Cores。
- 8 Turing Tensor Cores。
- 1 RT Core。
- 4 processing blocks；各有 1 warp scheduler 和 1 dispatch unit。
- 96 KB unified L1/shared 實體資源，其中 compute 可配置 32/64 KB 或 64/32 KB shared/L1。

將這份**架構規格**和 Runtime 的 40 active SM 相乘，可得到本機產品的架構推導：

| 單元 | 算式 | 推導值 |
|---|---:|---:|
| FP32 CUDA Cores | 40 SM × 64 | 2,560 |
| INT32 Cores | 40 SM × 64 | 2,560 |
| Tensor Cores | 40 SM × 8 | 320 |
| RT Cores | 40 SM × 1 | 40 |
| Warp schedulers | 40 SM × 4 | 160 |

這不是 `cudaDeviceGetAttribute` 的直接輸出，也不能把「每 Turing SM」公式套到 Ampere、Ada 或 Blackwell。Runtime 的 max shared/SM 是 64 KiB，而白皮書說 unified L1/shared 共 96 KB，也不是矛盾：前者只回答最多能配置多少 shared memory，後者描述 shared 與 L1/texture cache 共用的實體資源。

---

## 6. 未來 RTX 5090：同一名詞，不同內部配置

RTX Blackwell GB20x 白皮書以 full GB202 說明實體階層：

- 每個 full GPC 有 8 TPC、16 SM。
- 每個 TPC 有 2 SM。
- 每個 SM 有 128 CUDA Cores、4 fifth-generation Tensor Cores、1 fourth-generation RT Core。
- 每 SM 有 256 KB register file 與 128 KB configurable L1/shared memory。
- Full GB202 有 192 SM、24,576 CUDA Cores、768 Tensor Cores、192 RT Cores、128 MB L2。
- **RTX 5090 是裁切後的產品，不等於 full GB202**；同一白皮書明列 RTX 5090 的 L2 是 96 MB。

因此未來桌機到手後，正確流程是：

1. 用本課 probe 查該卡實際 active SM、L2、memory bus、register/shared limits。
2. 用當期 NVIDIA RTX 5090 官方 product spec/whitepaper 查 CUDA/Tensor/RT Core 產品數。
3. 用 `nvidia-smi` 查 power/clock/memory state。
4. 用 profiler 與 benchmark 驗真正吞吐量，不把 full-die 圖當成該 SKU 實測。

資料中心 Blackwell（例如 GB100/B200）與 RTX Blackwell（GB20x、未來目標 RTX 5090 為 `sm_120`）也不能只因都叫 Blackwell 就假設指令、SM 資源與產品階層完全相同。課 1.3 會系統化比較各世代。

---

## 7. 實驗：讓 GPU 自己回報架構

### 7.1 程式做了什麼

`src/gpu_arch_probe.cu` 分三層收集：

1. `cudaGetDeviceProperties`：只讀穩定且適合的名稱、compute capability、global memory 欄位。
2. `cudaDeviceGetAttribute`：查 SM、warp、threads、blocks、register/shared、L2、memory clock/bus 等個別屬性。這避免使用 CUDA 13.x 已移除的舊 `cudaDeviceProp::clockRate` 欄位。
3. 一個 64-thread kernel：每個 thread 以 PTX special registers 取 `%laneid`、`%smid`、`%nsmid`，回傳 host 驗證。

Kernel 會確認：

- 64 threads 被分成 2 個 32-thread warps。
- `laneid == linear_thread_id % 32`。
- `%smid < %nsmid`。
- sample record 的 block/thread index 沒有損壞。

它也列出同一 block 的 threads 本次是否讀到不同 `%smid`。本機結果是 0 conflicts，符合這個短 kernel 未被 preempt 時整個 block 在同一 SM 的觀察。但 PTX 規格明訂 `%smid` 是 volatile profiling/diagnostic register，preemption/rescheduling 後可能改變，因此非零值不應被簡化成「CUDA 把同一 block 同時拆到不同 SM」。

同理，`%nsmid` 是 **SM identifier 上界**，SM IDs 不保證連續，所以它可能大於 physical SM count；真正 active SM count 應讀 `cudaDevAttrMultiProcessorCount`。

### 7.2 編譯與執行

在本課目錄：

```bash
../tools/build.sh src/gpu_arch_probe.cu --native \
  -o /tmp/gpu_arch_probe

/tmp/gpu_arch_probe
/tmp/gpu_arch_probe --blocks-per-sm 8
```

查看 compiler 對 sample kernel 的 register/spill 報告：

```bash
nvcc -std=c++20 -O2 -arch=native -Xptxas=-v \
  src/gpu_arch_probe.cu -o /tmp/gpu_arch_probe
```

完整自我驗證：

```bash
./verify.sh --dry-run
./verify.sh
CODEX_COMPUTE_SANITIZER=1 ./verify.sh
```

從 Codex 課程根目錄做四架構編譯加本機 runtime：

```bash
tools/check_build.sh \
  --lesson "第 01.02 課：GPU 硬體架構總覽"
```

`sm_75/sm_89/sm_90/sm_120` 全部編譯成功只代表 source 能為這些 target 產生 code；本機只有 sm_75，可實際驗證的 runtime 也只有 sm_75。

### 7.3 本機實際輸出

以下省略當下會變動的 free-memory 欄位：

```text
probe.format=1
cuda.driver_version=13.3
cuda.runtime_version=13.3
device.index=0
device.name=Quadro RTX 4000 with Max-Q Design
device.pci_bus_id=0000:01:00.0
device.compute_capability=7.5
device.global_memory_mib=7761.375
sm.count=40
sm.warp_size=32
sm.max_threads=1024
sm.max_blocks=16
sm.registers_32bit=65536
sm.shared_memory_bytes=65536
block.max_threads=1024
block.shared_memory_bytes=49152
block.optin_shared_memory_bytes=65536
cache.l2_bytes=4194304
memory.peak_clock_khz=6001000
memory.bus_width_bits=256
memory.theoretical_peak_gb_s=384.064
device.async_engine_count=3
device.concurrent_kernels=1
device.unified_addressing=1
sample.grid_blocks=160
sample.threads_per_block=64
sample.warps_per_block=2
sample.observed_unique_sm_ids=40
sample.ptx_nsmid_upper_bound=40
sample.observed_sm_ids=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39
sample.block_sm_conflicts=0
sample.invalid_records=0
sample.kernel_active_blocks_per_sm=16
sample.kernel_thread_occupancy_limit=1.000
sample.validation=PASS
```

`ptxas -v` 對本次 sm_75 build 另回報：

```text
0 bytes stack frame, 0 bytes spill stores, 0 bytes spill loads
Used 18 registers, used 0 barriers
```

### 7.4 正確解讀這份結果

1. **40 unique SM IDs 是本次觀察，不是 launch contract。** 短 grid 可能沒有觀察到全部 SM；排程順序與分布不保證。
2. 本機剛好 `%nsmid=40` 且 IDs 連續，不代表所有 GPU 都如此。
3. `kernel_thread_occupancy_limit=1.000` 只表示這支 64-thread、18-register、0-shared sample kernel 依 occupancy API 可達 max resident thread 比例；它不是「GPU 100% 忙碌」。
4. `concurrent_kernels=1` 是 true/false 能力值。
5. `memory.theoretical_peak_gb_s` 是規格推算，不是 benchmark。
6. CUDA/Tensor/RT Core 數沒有出現在 output，是刻意的：Runtime 沒有對應通用屬性，應用白皮書與 product spec 補足。

---

## 8. 常見錯誤說法

| 錯誤說法 | 正確版本 |
|---|---|
| 一個 CUDA Core 就是一個 CUDA thread | Thread 是邏輯工作；warp 才是主要排程群組，指令再映射到 architecture-specific pipelines |
| 有 2,560 cores，所以一次一定有 2,560 threads 同時執行 | Resident/issued/active 是不同概念，受 warps、resources、dependencies 與 pipelines 影響 |
| 所有 NVIDIA GPU 都是 GPU→GPC→TPC→2 SM→64 CUDA Cores | GPC/TPC/SM 組態與每 SM lanes 都依架構及產品而變 |
| Tensor Cores 會自動加速所有 float code | 必須產生支援的 matrix instruction/library path，dtype/shape 也要符合 |
| RT Cores 可以拿來跑一般 CUDA arithmetic | RT Core 是 ray tracing 特定功能，不是一般 scalar execution lane |
| `sharedMemPerMultiprocessor` 就是整個 L1/shared 實體容量 | 它是可配置 shared memory 上限；unified cache 的完整實體容量要看架構文件 |
| local memory 在 SM 上，所以很快 | CUDA local address space 通常由 global device memory 支撐，只是 thread-private 語意 |
| occupancy 100% 代表效能已滿 | Occupancy 只描述 resident warps 比例，不代表 pipeline 或 bandwidth utilization |
| probe 看到所有 SM，之後每次 launch 也一定平均分配 | block-to-SM 排程沒有這種一般保證 |

---

## 9. 練習

### 練習 1：sampling grid 與排程觀察

依序執行：

```bash
/tmp/gpu_arch_probe --blocks-per-sm 1
/tmp/gpu_arch_probe --blocks-per-sm 4
/tmp/gpu_arch_probe --blocks-per-sm 32
```

記錄 `sample.observed_unique_sm_ids`。若三次都等於 40，能否證明 CUDA 規格保證平均分配？答案必須區分 observation 與 contract。

### 練習 2：由架構推導，但標明來源

只使用本課的兩種來源：

1. Runtime 的 40 active SM。
2. Turing whitepaper 的 per-SM units。

自行重算本機 CUDA/Tensor/RT Core 數，並在答案旁標出哪個數是 API observation、哪個是 architecture assumption。

### 練習 3：資源限制

假設 kernel 每 thread 需要 80 個 32-bit registers、每 block 256 threads，先忽略 allocation granularity：

1. 每 block 需要多少 registers？
2. 只看 65,536 registers/SM，理論最多幾個 blocks？
3. 再加入 1,024 threads/SM 上限後，答案是否改變？

### 練習 4：比較未來 RTX 5090

到貨後保存本課 probe output，和本機 sm_75 做 diff。至少比較：SM count、compute capability、register/shared/L2、memory bus/bandwidth、max resident threads/blocks。不要只比較 CUDA Core 總數。

---

## 10. 自我檢查與答案

1. **一般 CUDA kernel 可以指定 TPC 嗎？** 不能。它指定 grid/block/thread 與其他 launch parameters；GPC/TPC 是硬體組織。新架構 cluster 可要求 blocks 同 GPC，但仍不是任意 TPC 綁定。

2. **Block、warp、thread 哪一個是 SM 指令排程的主要單位？** Warp。Block 是資源配置與合作邊界，thread 是邏輯 scalar execution context。

3. **Register 與 shared memory 為何會影響 occupancy？** 同一 SM 的 resident blocks/warps 共用有限 register file 與 shared memory；每 block 消耗愈多，能同時駐留的 blocks 可能愈少。

4. **L1 與 L2 最大差異？** L1 每 SM，L2 GPU-wide；現代 SM 的 L1 data cache 常與 shared memory 共用 unified resource。

5. **為什麼 probe 不直接印 CUDA Core 數？** CUDA Runtime 沒有通用屬性；每 SM core 組態跨架構不同，應查相符白皮書/產品規格，不用脆弱公式冒充 API 事實。

6. **練習 3 答案？** 每 block 為 $80\times256=20,480$ registers；只看 register file 可容納 $\lfloor65,536/20,480\rfloor=3$ blocks。threads 上限則為 $\lfloor1,024/256\rfloor=4$ blocks，所以 register 限制先把結果降到 3；實際還要考慮 allocation granularity 和其他限制。

7. **`sample.kernel_thread_occupancy_limit=1.0` 是否表示 sample kernel 使用了全部 ALU？** 否。它只表示 occupancy API 推算的 resident threads 達到該 SM 的 thread 上限。

---

## 11. 本課結論

請保留六句話：

1. CUDA 程式模型是 grid→block→warp/thread；晶片實作則是 GPU→GPC/TPC→SM→pipelines。
2. SM 是 block/warp 執行與資源配置核心；CUDA Core 不是獨立 CPU core，也不是 CUDA scheduler。
3. Tensor/RT Cores 是專用單元，只有匹配的 instruction/software path 才能使用。
4. Registers 與 shared memory 很快但有限，會直接限制 residency/occupancy。
5. L1 屬每 SM，L2 屬整顆 GPU，DRAM 經 memory controllers 提供容量與頻寬。
6. API observation、白皮書規格、一次 runtime sample 是三種不同證據，不能互相冒充。

本課完成證明：

> 我能分開 CUDA 程式階層與 NVIDIA GPU 實體階層，說明 SM 內排程/運算/記憶體資源，並以本機 CUDA Runtime 與 PTX probe 驗證 40 SM、32-lane warp、資源上限及一次真實 block-to-SM 排程觀察。

---

## 12. 延伸閱讀

以下皆為 primary/official sources（本課查證日：2026-07-19）：

1. NVIDIA, [CUDA Programming Guide — Programming Model](https://docs.nvidia.com/cuda/cuda-programming-guide/01-introduction/programming-model.html)：SM/GPC 概念、grid/block、32-thread warp、on-chip memory 與 cache。
2. NVIDIA, [CUDA Programming Guide — Advanced Kernel Programming](https://docs.nvidia.com/cuda/cuda-programming-guide/03-advanced/advanced-kernel-programming.html)：warp scheduler、hardware multithreading 與 resource-limited residency。
3. NVIDIA, [CUDA Programming Guide — Compute Capabilities](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/compute-capabilities.html)：各 compute capability 的 threads/warps/blocks/register/shared 上限。
4. NVIDIA, [CUDA Runtime API — Device Management](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__DEVICE.html)：`cudaDeviceGetAttribute` 等 device query API。
5. NVIDIA, [CUDA Runtime API — `cudaDeviceProp`](https://docs.nvidia.com/cuda/cuda-runtime-api/structcudaDeviceProp.html)：device properties 欄位定義。
6. NVIDIA, [CUDA Runtime API — Occupancy](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__OCCUPANCY.html)：`cudaOccupancyMaxActiveBlocksPerMultiprocessor` 的精確語意。
7. NVIDIA, [PTX ISA — Special Registers](https://docs.nvidia.com/cuda/parallel-thread-execution/#special-registers)：`%laneid`、`%smid`、`%nsmid` 及 SM ID 非連續/volatile 限制。
8. NVIDIA, [Turing GPU Architecture Whitepaper](https://images.nvidia.com/aem-dam/en-zz/Solutions/design-visualization/technologies/turing-architecture/NVIDIA-Turing-Architecture-Whitepaper.pdf)：Turing GPC/TPC/SM、execution units、register 與 L1/shared 結構。
9. NVIDIA, [RTX Blackwell GPU Architecture Whitepaper](https://images.nvidia.com/aem-dam/Solutions/geforce/blackwell/nvidia-rtx-blackwell-gpu-architecture.pdf)：GB20x GPC/TPC/SM 與 full GB202/RTX 5090 差異。
10. David B. Kirk, Wen-mei W. Hwu, Izzat El Hajj, *Programming Massively Parallel Processors*, 5th ed., Ch. 4。
