// gpu_topology.cu — 把一顆 GPU 的硬體階層「印出來」
//
// 課 1.2 動手做（一）。目的：不背規格表，而是從你手上這顆卡問出真實數字，
// 並且分清楚三種資訊的**可信度**：
//   [查詢] CUDA runtime 直接給的 → 一定正確
//   [推導] 由查詢值換算的       → 正確，但依賴我的換算公式
//   [白皮書] CUDA 問不到的      → 只能查架構白皮書，程式無法驗證
//
// 這個分類很重要：網路上大量「GPU 規格」文章把三者混為一談，
// 於是「每個 SM 有 64 個 CUDA core」這種**只對 Turing 成立**的句子被到處複製。
//
// 編譯：
//   nvcc -std=c++20 -O2 -arch=native gpu_topology.cu -o /tmp/gpu_topology
//   （或 ../../tools/build.sh src/gpu_topology.cu --native -o /tmp/gpu_topology）

#include <cstdio>
#include <cuda_runtime.h>

// ── 錯誤檢查巨集 ─────────────────────────────────────────────
// CUDA 的 API 幾乎都回傳 cudaError_t，沉默失敗是初學最大的坑：
// 你以為 kernel 跑了，其實根本沒 launch 成功。一律包起來。
#define CUDA_CHECK(call)                                                      \
    do {                                                                      \
        cudaError_t _e = (call);                                              \
        if (_e != cudaSuccess) {                                              \
            std::fprintf(stderr, "CUDA 錯誤 %s:%d — %s\n",                    \
                         __FILE__, __LINE__, cudaGetErrorString(_e));         \
            return 1;                                                         \
        }                                                                     \
    } while (0)

// ── 每個 SM 的 FP32 CUDA core 數 ─────────────────────────────
// ⚠️ CUDA **沒有** API 可以問這個數字，只能靠 compute capability 查表。
//    這正是「別硬編架構假設」的典型案例：Turing 是 64，Ada/Blackwell 是 128，
//    寫死任何一個數字，換張卡就算錯。
static int fp32CoresPerSM(int major, int minor)
{
    switch (major) {
        case 7:  return (minor == 0) ? 64 : 64;   // Volta 7.0 / Turing 7.5
        case 8:  return (minor == 0) ? 64 : 128;  // A100 8.0 是 64；8.6/8.9 是 128
        case 9:  return 128;                      // Hopper
        case 10:                                  // 資料中心 Blackwell
        case 12: return 128;                      // 消費級 Blackwell（sm_120）
        default: return -1;                       // 未知世代 → 誠實回報不知道
    }
}

// Tensor Core 世代同樣查不到，只能由 CC 對照架構
static const char* tensorCoreGen(int major, int minor)
{
    if (major == 7 && minor == 0) return "第 1 代（Volta）";
    if (major == 7 && minor == 5) return "第 2 代（Turing，加 INT8/INT4）";
    if (major == 8 && minor == 0) return "第 3 代（Ampere，加 TF32/BF16/稀疏）";
    if (major == 8)               return "第 3 代（Ampere/Ada 系）";
    if (major == 9)               return "第 4 代（Hopper，加 FP8、WGMMA）";
    if (major == 10)              return "第 5 代（資料中心 Blackwell，加 FP4、tcgen05）";
    if (major == 12)              return "第 5 代（消費級 Blackwell，FP4；無 tcgen05）";
    return "未知";
}

int main()
{
    int devCount = 0;
    CUDA_CHECK(cudaGetDeviceCount(&devCount));
    if (devCount == 0) {
        std::fprintf(stderr, "找不到 CUDA 裝置\n");
        return 1;
    }

    for (int dev = 0; dev < devCount; ++dev) {
        cudaDeviceProp p{};
        CUDA_CHECK(cudaGetDeviceProperties(&p, dev));

        // ⚠️ 踩雷（承課 1.1）：p.clockRate / p.memoryClockRate 在 CUDA 13.x
        //    已從 cudaDeviceProp 移除，舊教材照抄會編不過。改走 attribute 查詢。
        int smClockKHz = 0, memClockKHz = 0;
        CUDA_CHECK(cudaDeviceGetAttribute(&smClockKHz,  cudaDevAttrClockRate,       dev));
        CUDA_CHECK(cudaDeviceGetAttribute(&memClockKHz, cudaDevAttrMemoryClockRate, dev));

        const int cc      = p.major * 10 + p.minor;
        const int coresSM = fp32CoresPerSM(p.major, p.minor);

        std::printf("══════════════════════════════════════════════════════════\n");
        std::printf(" GPU %d：%s\n", dev, p.name);
        std::printf(" Compute Capability %d.%d（sm_%d）\n", p.major, p.minor, cc);
        std::printf("══════════════════════════════════════════════════════════\n\n");

        // ── ① 晶片層級 ────────────────────────────────────────
        std::printf("① 晶片層級\n");
        std::printf("   [查詢] SM 數量                : %d\n", p.multiProcessorCount);
        std::printf("   [查詢] SM 時脈                : %.0f MHz\n", smClockKHz / 1000.0);
        std::printf("   [查詢] L2 cache               : %.1f MB（全晶片共用）\n",
                    p.l2CacheSize / 1048576.0);
        if (coresSM > 0) {
            std::printf("   [推導] FP32 CUDA core 總數    : %d（%d SM × %d/SM）\n",
                        p.multiProcessorCount * coresSM, p.multiProcessorCount, coresSM);
        } else {
            std::printf("   [推導] FP32 CUDA core 總數    : 未知（CC %d.%d 不在對照表內）\n",
                        p.major, p.minor);
        }
        std::printf("   [白皮書] GPC / TPC 數量        : CUDA 查不到，需查架構白皮書\n\n");

        // ── ② 記憶體子系統 ────────────────────────────────────
        // 頻寬 = 匯流排寬度 × 記憶體時脈 × 2（DDR 雙倍取樣）
        const double bwGBs = 2.0 * memClockKHz * 1e3 * (p.memoryBusWidth / 8.0) / 1e9;
        std::printf("② 記憶體子系統\n");
        std::printf("   [查詢] 全域記憶體             : %.2f GB\n", p.totalGlobalMem / 1073741824.0);
        std::printf("   [查詢] 匯流排寬度             : %d bit\n", p.memoryBusWidth);
        std::printf("   [查詢] 記憶體時脈             : %.0f MHz\n", memClockKHz / 1000.0);
        std::printf("   [推導] 理論頻寬               : %.1f GB/s\n", bwGBs);
        std::printf("   [推導] 記憶體控制器           : %d 組 ×32-bit\n", p.memoryBusWidth / 32);
        std::printf("   [查詢] L2 cache               : %.1f MB\n\n", p.l2CacheSize / 1048576.0);

        // ── ③ 單一 SM 內部 ────────────────────────────────────
        // 這一層是寫 kernel 時真正要放在腦子裡的：
        // 你的 block 就是被放進某個 SM，然後和其他 block 搶這些資源。
        std::printf("③ 單一 SM 內部資源\n");
        std::printf("   [查詢] warp 大小              : %d threads\n", p.warpSize);
        std::printf("   [查詢] 暫存器檔（每 SM）      : %d 個 32-bit（= %.0f KB）\n",
                    p.regsPerMultiprocessor, p.regsPerMultiprocessor * 4 / 1024.0);
        std::printf("   [查詢] 暫存器（每 block 上限）: %d 個\n", p.regsPerBlock);
        std::printf("   [查詢] Shared memory（每 SM） : %.0f KB\n",
                    p.sharedMemPerMultiprocessor / 1024.0);
        std::printf("   [查詢] Shared memory（每 block 預設上限）: %.0f KB\n",
                    p.sharedMemPerBlock / 1024.0);
        std::printf("   [查詢] 常駐 thread 上限       : %d（= %d warps）\n",
                    p.maxThreadsPerMultiProcessor,
                    p.maxThreadsPerMultiProcessor / p.warpSize);
        std::printf("   [查詢] 常駐 block 上限        : %d\n", p.maxBlocksPerMultiProcessor);
        std::printf("   [查詢] 單一 block thread 上限 : %d\n", p.maxThreadsPerBlock);
        std::printf("   [白皮書] warp scheduler / SM   : CUDA 查不到（Turing 及之後為 4）\n");
        std::printf("   [白皮書] Tensor Core 世代      : %s\n\n", tensorCoreGen(p.major, p.minor));

        // ── ④ 資源預算：把上面數字變成「我能開多少 thread」 ──
        // 這是本課從「規格表」跨到「會影響我寫的 code」的關鍵換算。
        const int maxWarpsSM = p.maxThreadsPerMultiProcessor / p.warpSize;
        std::printf("④ 資源預算（決定 occupancy 的三個上限）\n");
        std::printf("   要讓 %d 個 warp 全部同時常駐在一個 SM，平均每個 thread 只能用：\n",
                    maxWarpsSM);
        std::printf("     暫存器 ≤ %d 個 / thread\n",
                    p.regsPerMultiprocessor / p.maxThreadsPerMultiProcessor);
        std::printf("   若每個 block 開 256 threads（= 8 warps），要塞滿 SM 需 %d 個 block，\n",
                    maxWarpsSM / 8);
        std::printf("     此時每個 block 平均只能用 shared memory ≤ %.1f KB\n",
                    (p.sharedMemPerMultiprocessor / 1024.0) / (maxWarpsSM / 8.0));
        std::printf("   → 暫存器、shared memory、block 數，任一項先用完，occupancy 就卡在那裡。\n");
        std::printf("     （課 1.2 動手做（二）sm_residency 會把這個上限真的撞給你看）\n\n");

        // ── ⑤ 幾個常被誤解的旗標 ──────────────────────────────
        std::printf("⑤ 容易誤解的能力旗標\n");
        std::printf("   [查詢] L1 與 shared memory 同一塊 SRAM : %s\n",
                    p.globalL1CacheSupported ? "是（可切分比例）" : "否／不支援 global L1");
        std::printf("   [查詢] 統一定址 (UVA)          : %s\n", p.unifiedAddressing ? "支援" : "不支援");
        std::printf("   [查詢] 並行 kernel             : %s\n", p.concurrentKernels ? "支援" : "不支援");
        std::printf("   [查詢] 非同步引擎（複製引擎）  : %d 條\n", p.asyncEngineCount);
        std::printf("   [查詢] ECC                     : %s\n", p.ECCEnabled ? "開啟" : "關閉");
        std::printf("   [查詢] 是否 integrated（非獨顯）: %s\n", p.integrated ? "是" : "否");
        std::printf("\n");
    }

    return 0;
}
