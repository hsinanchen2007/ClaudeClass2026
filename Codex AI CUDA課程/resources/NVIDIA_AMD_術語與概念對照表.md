# NVIDIA ↔ AMD 術語與概念對照表

> **用途**：本課程以 NVIDIA/CUDA 為脊椎；這張表把 Part 1 起學到的每個 NVIDIA 概念橋接到 AMD/ROCm 世界。原理相同、名詞不同——查表即可遷移。
> **版本**：v1.8 新增（2026-06）。實作課見大綱 **課 10.14（HIP 移植實戰）／課 10.15（效能對齊實戰）**。
> **事實基準**：HIP/ROCm 官方文件（rocm.docs.amd.com）；工具命名以 ROCm 6.3+ 為準（Omniperf → ROCm Compute Profiler、rocm-smi → amd-smi）。

---

## 1. 硬體執行單元

| NVIDIA | AMD | 備註 |
|---|---|---|
| SM（Streaming Multiprocessor） | **CU（Compute Unit）** | 都是「工作引擎」；課 1.2 的結構觀念直接適用 |
| CUDA core | stream processor / SIMD lane | 行銷數字同樣別直接比，看吞吐 |
| **Tensor Core** | **Matrix Core（MFMA 指令）** | 都是矩陣乘加引擎；指令路線完全不同（mma/tcgen05 ↔ MFMA） |
| warp scheduler | SIMD scheduler | 同為「切換待命執行緒束藏延遲」 |
| GPC/TPC 階層 | Shader Engine / Compute 階層 | 階層化平鋪的思想一致 |

## 2. 執行模型（最重要的心智差異）

| NVIDIA | AMD | 備註 |
|---|---|---|
| **warp = 32 threads** | **wavefront = 64 threads（CDNA 資料中心）**；RDNA 可 32/64 | ⚠️ 影響 divergence 代價、occupancy 計算、shuffle/vote 寬度；block 大小慣例取 **64 的倍數** |
| thread / block / grid | work-item / work-group / grid（HIP 沿用 thread/block 名） | HIP 的 `threadIdx`/`blockIdx` 同名同義 |
| `__syncthreads()` | `__syncthreads()`（HIP 同名） | HIP 刻意沿用 CUDA 關鍵字 |
| occupancy | occupancy | 概念相同；但以 wavefront 為單位計 |

## 3. 記憶體階層

| NVIDIA | AMD | 備註 |
|---|---|---|
| register file | **VGPR + SGPR（向量/純量暫存器）** | ⚠️ AMD 有獨立**純量暫存器**（整個 wavefront 共用一份的值放 SGPR）——真實的架構差異 |
| **shared memory** | **LDS（Local Data Share）** | 同為 SM/CU 內、程式設計師掌控的 SRAM；bank conflict 概念同在 |
| L1 / L2 cache | L1 / L2（部分卡另有 Infinity Cache） | 階層邏輯相同（課 1.2/1.6 直接適用） |
| global memory（GDDR/HBM） | global memory（HBM/GDDR） | MI300X = 192GB HBM3、~5.3 TB/s——machine balance 演算法（**課 8.1**）照用 |
| local memory（register 溢位、實在 VRAM） | **scratch**（同樣在 VRAM） | 「名快實慢」的陷阱兩家都有 |
| constant memory | constant | 廣播語意類似 |

## 4. 編譯工具鏈（含一個本質差異）

| NVIDIA | AMD | 備註 |
|---|---|---|
| CUDA C++ | **HIP**（C++ runtime API + kernel 語言） | HIP 刻意長得像 CUDA；`__global__`/`<<<>>>` 皆可用 |
| nvcc | **hipcc**（clang 包裝；ROCm 6.3 起預設 amdclang） | |
| **PTX（虛擬 ISA，前向相容、可 JIT）** | **沒有對應層** —— 直接編到 GCN/CDNA ISA（code object） | ⚠️ 本質差異：AMD 的可攜策略是「multi-arch fat binary + 重編」，沒有 PTX 式的前向 JIT。課 1.4 的兩層模型是 NVIDIA 特有 |
| SASS | GCN / CDNA ISA | 真機器碼層 |
| cubin / fatbin | code object / fat binary（多 `--offload-arch` 打包） | |
| `-arch=sm_120`（5090） | `--offload-arch=gfx942`（MI300X）、`gfx90a`（MI200）、`gfx12xx`（RDNA4） | compute capability ↔ **gfx 代號** |
| `__CUDA_ARCH__` | `__gfx942__` 等架構巨集 / `__HIP_DEVICE_COMPILE__` | 編譯期分支同思想 |

## 5. Runtime API（hipify 的主戰場）

| NVIDIA | AMD（HIP） |
|---|---|
| `cudaMalloc` / `cudaMemcpy` / `cudaFree` | `hipMalloc` / `hipMemcpy` / `hipFree` |
| `cudaStream_t` / `cudaEvent_t` | `hipStream_t` / `hipEvent_t` |
| `cudaGetDeviceProperties` | `hipGetDeviceProperties`（`hipDeviceProp_t`） |
| `cudaDeviceSynchronize` | `hipDeviceSynchronize` |
| CUDA Graphs | hipGraph |

> 規律：多數 host API 就是 `cuda*` → `hip*` 改名——這正是 `hipify` 自動化的部分；**裝不進規律的（intrinsics、PTX inline asm、Tensor Core 路徑、架構專屬功能）才是手工活**（HIP Porting Guide：HIP 不是 drop-in replacement）。

## 6. 函式庫對照

| NVIDIA | AMD | 備註 |
|---|---|---|
| cuBLAS | **rocBLAS**（hipBLAS 為可攜介面層） | hip* 系列 = 同名 API 殼，AMD 上呼叫 roc*、NVIDIA 上呼叫 cu* |
| cuDNN | **MIOpen** | PyTorch 在 AMD 上的 NN 後端 |
| NCCL | **RCCL** | 集體通訊 |
| cuFFT | rocFFT / hipFFT | |
| Thrust / CUB | rocThrust / hipCUB | |
| CUTLASS / CuTe | **Composable Kernel（CK）** | GEMM/attention 模板積木 |
| TensorRT | **MIGraphX** | 圖優化推論引擎 |
| Triton（NVIDIA 後端→PTX） | Triton（AMD 後端→AMDGCN） | **同一份 Triton 原始碼、兩個後端**——10.15 的主角 |

## 7. 工具對照

| NVIDIA | AMD | 備註 |
|---|---|---|
| nvidia-smi | **amd-smi**（rocm-smi 已被取代） | ROCm 6.3+ 命名 |
| Nsight Systems | ROCm Systems Profiler（原 Omnitrace） | 時間軸/系統級 |
| Nsight Compute | **ROCm Compute Profiler**（原 Omniperf；底層 rocprof） | kernel 級 metrics——10.15 用它做對齊 |
| cuda-gdb | rocgdb | |
| compute-sanitizer | ASan 建置目標（gfx9xx:xnack+） | 記憶體檢查思路相近 |

## 8. 互連與系統

| NVIDIA | AMD | 備註 |
|---|---|---|
| NVLink / NVSwitch | **Infinity Fabric / xGMI** | 多卡高速互連 |
| GPUDirect RDMA | ROCm RDMA | |
| MIG | 分割方案不同（依產品） | 兩家皆資料中心特性 |

## 9. 七個「換腦袋」重點（移植時最容易踩的）

1. **wavefront 寬度**：**CDNA（MI300X 等資料中心卡）= 64**；**RDNA 遊戲卡多為 32**（部分模式可 64）——occupancy 與 divergence 的帳要依實際寬度重算；warp-shuffle 寬度假設 32 的程式碼要改。**本 lab 以 CDNA MI300X（64）為準，別一概而論。**
2. **無 PTX 層**：別假設「留一份中間碼就能前向相容」；AMD 靠 multi-arch 重編。
3. **SGPR 的存在**：uniform 值有專屬純量暫存器——編譯器行為與暫存器壓力分析不同。
4. **Tensor Core 路徑完全不可攜**：mma/WGMMA/tcgen05 ↔ MFMA 是兩個世界；走 Triton/CK/函式庫才有可攜性。
5. **hipify ≠ 完工**：自動轉換解決改名，**效能對齊才是工作量所在**（10.15 的方法論）。
6. **⚠️ HIP 的 NVIDIA 後端不能驗證 AMD 行為**（**已依大綱 10.14 更正；舊版曾寫成「本機優勢、可先在 5090 開發驗證」是錯的**）：`HIP_PLATFORM=nvidia` 雖存在，但屬**低優先維護**，且它**編出來的是 CUDA、跑的是 NVIDIA 卡**——驗證不了 wavefront-64／LDS／MFMA／效能等任何 AMD 行為。本機只做**靜態轉換（hipify）與抽象層設計**；編譯／執行／profiling／正確性與效能驗證一律在**雲端 MI300X**。
7. **量測對齊**：兩邊都先算 roofline 天花板（**課 8.1** 的 roofline／machine balance 方法 + 該卡規格），再比「達成峰值百分比」，不要比絕對毫秒。

## 10. 官方資源（v1.8 查證）

- HIP 文件（含 Porting Guide）：`https://rocm.docs.amd.com/projects/HIP/en/latest/`
- HIPIFY：`https://github.com/ROCm/HIPIFY`
- ROCm 文件入口：`https://rocm.docs.amd.com/`
- Triton：`https://github.com/triton-lang/triton`
