// ============================================================================
//  課 1.1 — 為什麼需要 GPU
//  gpu_vs_cpu_profile.cu — 用「你這台機器的真實規格」算出 CPU vs GPU 的設計差異
//
//  這支程式【不寫任何 kernel】（那是 Part 3 的事），只做兩件事：
//    1. 向 CUDA driver 查詢 GPU 的硬體屬性
//    2. 用課堂學到的公式，把規格換算成「峰值算力」與「峰值頻寬」
//
//  重點不是數字本身，而是看懂兩件事：
//    - GPU 把電晶體預算砸在「算術單元」，CPU 砸在「控制邏輯與快取」
//    - 因此 GPU 的算力/頻寬遠高於 CPU，但**單一工作的延遲並不會比較低**
//
//  編譯：nvcc -std=c++20 -O3 -arch=native gpu_vs_cpu_profile.cu -o gpu_vs_cpu_profile
//    或：../../tools/build.sh src/gpu_vs_cpu_profile.cu --native
// ============================================================================

#include <cstdio>
#include <cuda_runtime.h>

// CUDA API 的錯誤檢查巨集。
// Part 3.7 會正式教錯誤處理，這裡先建立「每個 API 都要檢查回傳值」的習慣。
#define CUDA_CHECK(call)                                                       \
    do {                                                                       \
        cudaError_t err_ = (call);                                             \
        if (err_ != cudaSuccess) {                                             \
            std::fprintf(stderr, "CUDA 錯誤 %s:%d — %s\n",                     \
                         __FILE__, __LINE__, cudaGetErrorString(err_));        \
            return 1;                                                          \
        }                                                                      \
    } while (0)

// 每個 SM 有多少個 FP32 CUDA core，依架構（compute capability）而異。
// 這是硬體設計決定的常數，CUDA runtime 不會直接告訴你，要自己對照。
static int fp32CoresPerSM(int major, int minor)
{
    switch (major) {
        case 7:  return (minor == 0) ? 64 : 64;   // Volta(7.0) / Turing(7.5)
        case 8:  return (minor == 0) ? 64 : 128;  // A100(8.0) 64；Ampere 消費級/Ada(8.6/8.9) 128
        case 9:  return 128;                      // Hopper
        case 10:                                  // Blackwell 資料中心
        case 12: return 128;                      // Blackwell 消費級（sm_120）
        default: return 0;                        // 未知架構 → 不猜
    }
}

int main()
{
    int deviceCount = 0;
    CUDA_CHECK(cudaGetDeviceCount(&deviceCount));
    if (deviceCount == 0) {
        std::printf("找不到 CUDA 裝置。\n");
        return 1;
    }

    for (int dev = 0; dev < deviceCount; ++dev) {
        cudaDeviceProp p{};
        CUDA_CHECK(cudaGetDeviceProperties(&p, dev));

        std::printf("═══ GPU %d：%s ═══\n", dev, p.name);
        std::printf("  Compute capability : %d.%d (sm_%d%d)\n",
                    p.major, p.minor, p.major, p.minor);
        std::printf("  SM 數量            : %d\n", p.multiProcessorCount);

        const int coresPerSM = fp32CoresPerSM(p.major, p.minor);
        const int totalCores = coresPerSM * p.multiProcessorCount;
        if (coresPerSM > 0)
            std::printf("  FP32 CUDA core     : %d  (%d SM × %d core/SM)\n",
                        totalCores, p.multiProcessorCount, coresPerSM);

        // ⚠️ 踩雷（本課實測發現）：
        //    舊教材／舊程式碼幾乎都寫 p.clockRate 與 p.memoryClockRate，
        //    但這兩個欄位在 CUDA 12.x 標為 deprecated、**CUDA 13.x 已從
        //    cudaDeviceProp 直接移除**，照抄會編不過：
        //        error: class "cudaDeviceProp" has no member "clockRate"
        //    現行做法是用 cudaDeviceGetAttribute 查屬性（單位同樣是 kHz）。
        int smClockKHz = 0, memClockKHz = 0;
        CUDA_CHECK(cudaDeviceGetAttribute(&smClockKHz,  cudaDevAttrClockRate,       dev));
        CUDA_CHECK(cudaDeviceGetAttribute(&memClockKHz, cudaDevAttrMemoryClockRate, dev));
        const double smClockGHz  = smClockKHz  / 1e6;
        const double memClockGHz = memClockKHz / 1e6;

        std::printf("  SM 時脈            : %.3f GHz\n", smClockGHz);
        std::printf("  記憶體             : %.1f GB, %d-bit @ %.3f GHz(effective)\n",
                    p.totalGlobalMem / (1024.0 * 1024.0 * 1024.0),
                    p.memoryBusWidth, memClockGHz);
        std::printf("  L2 cache           : %.1f MB\n", p.l2CacheSize / (1024.0 * 1024.0));
        std::printf("  Shared mem / block : %.0f KB\n", p.sharedMemPerBlock / 1024.0);
        std::printf("  Warp size          : %d\n", p.warpSize);
        std::printf("  每 SM 最大常駐執行緒: %d\n", p.maxThreadsPerMultiProcessor);
        // 「常駐執行緒總數」＝ GPU 能同時「在飛」的執行緒數。
        // 這個數字是理解 latency hiding 的關鍵（課 4.1 會深入）。
        std::printf("  → 全 GPU 可常駐執行緒: %d\n",
                    p.maxThreadsPerMultiProcessor * p.multiProcessorCount);

        // ── 峰值算力 ──
        // 每個 CUDA core 每個 cycle 能做 1 次 FMA = 2 個浮點運算（乘 + 加）
        if (coresPerSM > 0) {
            const double tflops = totalCores * 2.0 * smClockGHz / 1000.0;
            std::printf("\n  峰值 FP32 算力     : %.2f TFLOPS\n", tflops);
            std::printf("    = %d core × 2 FLOP/cycle(FMA) × %.3f GHz\n",
                        totalCores, smClockGHz);
        }

        // ── 峰值頻寬 ──
        // GDDR 是 double data rate，故 ×2；bus width 是 bit，故 ÷8 轉 byte
        const double bwGBs = p.memoryBusWidth / 8.0 * memClockGHz * 2.0;
        std::printf("\n  峰值記憶體頻寬     : %.1f GB/s\n", bwGBs);
        std::printf("    = %d bit ÷ 8 × %.3f GHz × 2 (DDR)\n",
                    p.memoryBusWidth, memClockGHz);

        // ── Machine balance ──
        // 每從記憶體搬 1 byte，硬體「有能力」做幾次浮點運算？
        // 這個比值決定了「你的 kernel 想跑滿算力，需要多高的資料重用率」，
        // 是 Roofline 模型（課 8.1）的核心，也是整個 Part 8 優化的出發點。
        if (coresPerSM > 0) {
            const double flops = totalCores * 2.0 * smClockGHz * 1e9;
            std::printf("\n  Machine balance    : %.1f FLOP/byte\n",
                        flops / (bwGBs * 1e9));
            std::printf("    → 每搬 1 byte，硬體有能力做這麼多次浮點運算。\n");
            std::printf("      演算法的 arithmetic intensity 若低於這個值，\n");
            std::printf("      就是 memory-bound —— 再快的算力也用不上（見課 8.1）。\n");
        }
        std::printf("\n");
    }
    return 0;
}
