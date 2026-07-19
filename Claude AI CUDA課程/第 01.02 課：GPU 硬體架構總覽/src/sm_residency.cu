// sm_residency.cu — 讓「SM」從名詞變成你看得見的東西
//
// 課 1.2 動手做（二）。gpu_topology 印的是規格；這支證明規格如何**約束你的 kernel**。
//
// 三個實驗：
//   實驗 A：每個 block 報出自己被排到哪個 SM → 證明 block 真的被撒到各個 SM 上
//   實驗 B：同一份計算、三種暫存器壓力 → 看暫存器如何把 occupancy 壓下來
//   實驗 C：shared memory 用量 vs 每 SM 常駐 block 數 → 另一條獨立的資源上限
//
// 編譯：
//   nvcc -std=c++20 -O2 -arch=native sm_residency.cu -o /tmp/sm_residency
//   （或 ../../tools/build.sh src/sm_residency.cu --native -o /tmp/sm_residency）

#include <cstdio>
#include <vector>
#include <algorithm>
#include <cuda_runtime.h>

#define CUDA_CHECK(call)                                                      \
    do {                                                                      \
        cudaError_t _e = (call);                                              \
        if (_e != cudaSuccess) {                                              \
            std::fprintf(stderr, "CUDA 錯誤 %s:%d — %s\n",                    \
                         __FILE__, __LINE__, cudaGetErrorString(_e));         \
            return 1;                                                         \
        }                                                                     \
    } while (0)

// ─────────────────────────────────────────────────────────────
// 實驗 A：問硬體「我在哪個 SM 上？」
//
// %smid 是 PTX 的特殊暫存器，沒有對應的 C++ API，只能用 inline PTX 讀。
// ⚠️ 注意語意：%smid 是「**此刻**這個 block 跑在哪個 SM」——block 一旦被排上去
//    就不會遷移，但這個值不保證跨 launch 穩定，也不該用來寫演算法邏輯。
//    它是**觀測工具**，不是可依賴的介面。
// ─────────────────────────────────────────────────────────────
__global__ void reportSM(unsigned int* smIdOut)
{
    if (threadIdx.x == 0) {
        unsigned int smid;
        asm volatile("mov.u32 %0, %%smid;" : "=r"(smid));
        smIdOut[blockIdx.x] = smid;
    }
}

// ─────────────────────────────────────────────────────────────
// 實驗 B：暫存器壓力可調的 kernel
//
// N 個獨立累加器 → 全部展開後會同時「活著」，逼編譯器配 N 份暫存器。
// 這是刻意寫得「暫存器很吃」的教學程式碼，不是好的計算範例。
// ─────────────────────────────────────────────────────────────
template <int N>
__global__ void regPressure(float* out, const float* in, int n)
{
    float acc[N];

    // 全展開 → acc[i] 變成 N 個獨立的具名暫存器（而不是 local memory 陣列）
#pragma unroll
    for (int i = 0; i < N; ++i)
        acc[i] = in[(threadIdx.x + i * 32) % n];

    for (int it = 0; it < 4; ++it) {
#pragma unroll
        for (int i = 0; i < N; ++i)
            acc[i] = fmaf(acc[i], acc[i], 1.0f);   // 互不相依 → 全部得同時保持存活
    }

    float s = 0.0f;
#pragma unroll
    for (int i = 0; i < N; ++i)
        s += acc[i];

    out[blockIdx.x * blockDim.x + threadIdx.x] = s;   // 寫回去，避免整段被最佳化掉
}

// 實驗 C 用：dynamic shared memory 的消費者
__global__ void sharedUser(float* out, int nElem)
{
    extern __shared__ float tile[];
    for (int i = threadIdx.x; i < nElem; i += blockDim.x)
        tile[i] = static_cast<float>(i);
    __syncthreads();
    if (threadIdx.x == 0)
        out[blockIdx.x] = tile[nElem - 1];
}

// 印出某個 kernel 的暫存器用量與理論 occupancy
template <typename Fn>
static int reportOccupancy(const char* label, Fn kernel, int blockSize,
                           size_t dynShmem, const cudaDeviceProp& p)
{
    cudaFuncAttributes attr{};
    CUDA_CHECK(cudaFuncGetAttributes(&attr, reinterpret_cast<const void*>(kernel)));

    int blocksPerSM = 0;
    CUDA_CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(
        &blocksPerSM, reinterpret_cast<const void*>(kernel), blockSize, dynShmem));

    const int warpsPerBlock = blockSize / p.warpSize;
    const int maxWarpsSM    = p.maxThreadsPerMultiProcessor / p.warpSize;
    const int activeWarps   = blocksPerSM * warpsPerBlock;
    const double occ        = 100.0 * activeWarps / maxWarpsSM;

    std::printf("   %-22s 暫存器/thread=%-4d local=%-5zu B  → block/SM=%-2d  "
                "warp/SM=%-3d  occupancy=%.0f%%\n",
                label, attr.numRegs, attr.localSizeBytes,
                blocksPerSM, activeWarps, occ);
    return 0;
}

int main()
{
    int dev = 0;
    CUDA_CHECK(cudaGetDevice(&dev));
    cudaDeviceProp p{};
    CUDA_CHECK(cudaGetDeviceProperties(&p, dev));

    std::printf("裝置：%s（sm_%d%d）｜SM 數 %d｜每 SM 上限 %d threads（%d warps）\n\n",
                p.name, p.major, p.minor, p.multiProcessorCount,
                p.maxThreadsPerMultiProcessor,
                p.maxThreadsPerMultiProcessor / p.warpSize);

    // ═══ 實驗 A：block → SM 的分佈 ═══════════════════════════
    // 開「SM 數 × 4」個 block，看排程器怎麼撒。
    const int nBlocks = p.multiProcessorCount * 4;
    unsigned int* dSmId = nullptr;
    CUDA_CHECK(cudaMalloc(&dSmId, nBlocks * sizeof(unsigned int)));

    reportSM<<<nBlocks, 32>>>(dSmId);
    CUDA_CHECK(cudaGetLastError());          // 抓 launch 失敗（非同步錯誤要另外抓）
    CUDA_CHECK(cudaDeviceSynchronize());

    std::vector<unsigned int> hSmId(nBlocks);
    CUDA_CHECK(cudaMemcpy(hSmId.data(), dSmId, nBlocks * sizeof(unsigned int),
                          cudaMemcpyDeviceToHost));

    std::vector<int> hist(p.multiProcessorCount, 0);
    unsigned int maxSeen = 0;
    for (unsigned int id : hSmId) {
        maxSeen = std::max(maxSeen, id);
        if (id < hist.size()) hist[id]++;
    }

    std::printf("═══ 實驗 A：%d 個 block 落在哪些 SM ═══\n", nBlocks);
    std::printf("   實際出現的 SM 編號範圍：0 – %u\n", maxSeen);
    std::printf("   有拿到 block 的 SM 數  ：%zu / %d\n",
                std::count_if(hist.begin(), hist.end(), [](int c) { return c > 0; }),
                p.multiProcessorCount);
    std::printf("   每個 SM 拿到的 block 數：min=%d  max=%d\n",
                *std::min_element(hist.begin(), hist.end()),
                *std::max_element(hist.begin(), hist.end()));
    std::printf("   前 8 個 SM 的分佈      ：");
    for (int i = 0; i < std::min(8, p.multiProcessorCount); ++i)
        std::printf("SM%d=%d ", i, hist[i]);
    std::printf("\n   → block 不是排隊等一個 SM，而是被撒開；這就是 GPU 的 scale-out 單位。\n\n");
    CUDA_CHECK(cudaFree(dSmId));

    // ═══ 實驗 B：暫存器壓力 → occupancy ═════════════════════
    const int blockSize = 256;
    float *dOut = nullptr, *dIn = nullptr;
    const int nOut = nBlocks * blockSize;
    CUDA_CHECK(cudaMalloc(&dOut, nOut * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&dIn,  1024 * sizeof(float)));
    CUDA_CHECK(cudaMemset(dIn, 0, 1024 * sizeof(float)));

    std::printf("═══ 實驗 B：暫存器用量如何壓低 occupancy（block=%d threads）═══\n", blockSize);
    std::printf("   每個 SM 共有 %d 個 32-bit 暫存器；每個 thread 用得越多，能常駐的 warp 越少。\n",
                p.regsPerMultiprocessor);
    if (reportOccupancy("regPressure<4>  ", regPressure<4>,   blockSize, 0, p)) return 1;
    if (reportOccupancy("regPressure<32> ", regPressure<32>,  blockSize, 0, p)) return 1;
    if (reportOccupancy("regPressure<96> ", regPressure<96>,  blockSize, 0, p)) return 1;
    if (reportOccupancy("regPressure<128>", regPressure<128>, blockSize, 0, p)) return 1;
    if (reportOccupancy("regPressure<256>", regPressure<256>, blockSize, 0, p)) return 1;

    // 真的跑一次，確認它們不只是能編、還能跑
    regPressure<4><<<nBlocks, blockSize>>>(dOut, dIn, 1024);
    regPressure<32><<<nBlocks, blockSize>>>(dOut, dIn, 1024);
    regPressure<96><<<nBlocks, blockSize>>>(dOut, dIn, 1024);
    regPressure<128><<<nBlocks, blockSize>>>(dOut, dIn, 1024);
    regPressure<256><<<nBlocks, blockSize>>>(dOut, dIn, 1024);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    std::printf("   （五個 kernel 皆已實際執行成功）\n");
    std::printf("   → 看兩個轉折點：\n");
    std::printf("     ① occupancy 從 100%% 掉下來 = 暫存器需求超過「塞滿 SM 的預算」\n");
    std::printf("     ② local≠0 = **register spilling**——放不下的變數被搬到 local memory\n");
    std::printf("        （名字叫 local，其實住在 global memory），效能會斷崖式下滑。\n\n");

    // ═══ 實驗 C：shared memory → 常駐 block 數 ═══════════════
    std::printf("═══ 實驗 C：shared memory 是另一條獨立上限（block=%d threads）═══\n", blockSize);
    std::printf("   每個 SM 的 shared memory：%.0f KB\n", p.sharedMemPerMultiprocessor / 1024.0);
    for (size_t kb : {4u, 8u, 16u, 32u, 48u}) {
        const size_t bytes = kb * 1024;
        int blocksPerSM = 0;
        CUDA_CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(
            &blocksPerSM, reinterpret_cast<const void*>(sharedUser), blockSize, bytes));
        std::printf("   dynamic shared = %2zu KB/block  → block/SM=%-2d  "
                    "shared 實際用掉 %3zu KB/SM\n",
                    kb, blocksPerSM, kb * blocksPerSM);
    }
    std::printf("   → block/SM 幾乎和 shared memory 用量成反比：這是你在寫 tiling kernel 時\n");
    std::printf("     真正要做的取捨——tile 開大（少跑幾趟 global memory）vs 常駐 warp 變少\n");
    std::printf("     （能藏的延遲變少）。Part 5 會把這個取捨量化。\n\n");

    // 順帶證明 shared memory kernel 真的能跑
    sharedUser<<<nBlocks, blockSize, 4096>>>(dOut, 1024);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaFree(dOut));
    CUDA_CHECK(cudaFree(dIn));
    std::printf("全部實驗完成。\n");
    return 0;
}
