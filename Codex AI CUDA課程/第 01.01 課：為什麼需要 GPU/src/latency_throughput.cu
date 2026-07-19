#include <cuda_runtime.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

// 每個 Runtime API 都立即檢查；否則錯誤可能延後到下一個同步點才出現。
#define CUDA_CHECK(call)                                                          \
    do {                                                                          \
        const cudaError_t error_code = (call);                                    \
        if (error_code != cudaSuccess) {                                          \
            std::cerr << "CUDA 錯誤 " << __FILE__ << ':' << __LINE__ << "："       \
                      << cudaGetErrorString(error_code) << '\n';                  \
            std::exit(EXIT_FAILURE);                                              \
        }                                                                         \
    } while (false)

__global__ void saxpy_kernel(std::size_t count, float scale, const float* x, float* y) {
    // 一個邏輯 CUDA thread 處理一個元素；超過陣列尾端的 thread 不工作。
    const std::size_t index =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (index < count) {
        y[index] = scale * x[index] + y[index];
    }
}

void saxpy_cpu(std::size_t count, float scale, const float* x, float* y) {
    // 單一 host thread 的 reference；-O3 仍可讓編譯器自動向量化。
    for (std::size_t index = 0; index < count; ++index) {
        y[index] = scale * x[index] + y[index];
    }
}

double elapsed_ms(const std::chrono::steady_clock::time_point start,
                  const std::chrono::steady_clock::time_point stop) {
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

void make_memory_observable(const void* address) {
    // GCC/Clang 的空 asm 不產生指令；memory clobber 只阻止編譯器刪除或跨越
    // 這個位置搬動前面的陣列寫入，避免 benchmark 被最佳化成假快結果。
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("" : : "g"(address) : "memory");
#else
    (void)address;
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}

struct Result {
    std::size_t elements{};
    double cpu_ms{};
    double kernel_ms{};
    double end_to_end_ms{};
    bool correct{};
};

Result benchmark(std::size_t count) {
    constexpr float scale = 2.5F;
    constexpr int cpu_trials = 7;
    constexpr int gpu_trials = 7;
    constexpr int end_to_end_trials = 5;
    constexpr int threads = 256;

    std::vector<float> x(count);
    std::vector<float> initial_y(count, 1.0F);
    std::vector<float> cpu_y(count);
    std::vector<float> gpu_y(count);
    for (std::size_t index = 0; index < count; ++index) {
        x[index] = static_cast<float>(index % 251U) * 0.001F;
    }

    // 教學實驗取多次最短值，以降低背景程序干擾；正式報告還應看中位數與分位數。
    double best_cpu_ms = std::numeric_limits<double>::infinity();
    for (int trial = 0; trial < cpu_trials; ++trial) {
        cpu_y = initial_y;  // 重設不列入核心計算時間。
        const auto start = std::chrono::steady_clock::now();
        saxpy_cpu(count, scale, x.data(), cpu_y.data());
        make_memory_observable(cpu_y.data());
        const auto stop = std::chrono::steady_clock::now();
        best_cpu_ms = std::min(best_cpu_ms, elapsed_ms(start, stop));
    }

    // 配置放在量測外，避免把一次性 cudaMalloc 成本混入每次操作。
    float* device_x = nullptr;
    float* device_y = nullptr;
    const std::size_t bytes = count * sizeof(float);
    CUDA_CHECK(cudaMalloc(&device_x, bytes));
    CUDA_CHECK(cudaMalloc(&device_y, bytes));
    CUDA_CHECK(cudaMemcpy(device_x, x.data(), bytes, cudaMemcpyHostToDevice));

    const int blocks = static_cast<int>((count + threads - 1U) / threads);
    // 先暖機，避免 CUDA context 初始化成本污染 kernel 時間。
    CUDA_CHECK(cudaMemcpy(device_y, initial_y.data(), bytes, cudaMemcpyHostToDevice));
    saxpy_kernel<<<blocks, threads>>>(count, scale, device_x, device_y);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    cudaEvent_t start_event{};
    cudaEvent_t stop_event{};
    CUDA_CHECK(cudaEventCreate(&start_event));
    CUDA_CHECK(cudaEventCreate(&stop_event));

    // CUDA event 與 kernel 位於同一 default stream，只量 GPU 上的執行時間。
    float best_kernel_ms = std::numeric_limits<float>::infinity();
    for (int trial = 0; trial < gpu_trials; ++trial) {
        CUDA_CHECK(cudaMemcpy(device_y, initial_y.data(), bytes, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaEventRecord(start_event));
        saxpy_kernel<<<blocks, threads>>>(count, scale, device_x, device_y);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaEventRecord(stop_event));
        CUDA_CHECK(cudaEventSynchronize(stop_event));
        float trial_ms = 0.0F;
        CUDA_CHECK(cudaEventElapsedTime(&trial_ms, start_event, stop_event));
        best_kernel_ms = std::min(best_kernel_ms, trial_ms);
    }

    // 端到端路徑刻意包含每次 H2D、kernel 與 D2H，呈現一次性 offload 成本。
    double best_end_to_end_ms = std::numeric_limits<double>::infinity();
    for (int trial = 0; trial < end_to_end_trials; ++trial) {
        const auto start = std::chrono::steady_clock::now();
        CUDA_CHECK(cudaMemcpy(device_x, x.data(), bytes, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(device_y, initial_y.data(), bytes, cudaMemcpyHostToDevice));
        saxpy_kernel<<<blocks, threads>>>(count, scale, device_x, device_y);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaMemcpy(gpu_y.data(), device_y, bytes, cudaMemcpyDeviceToHost));
        const auto stop = std::chrono::steady_clock::now();
        best_end_to_end_ms =
            std::min(best_end_to_end_ms, elapsed_ms(start, stop));
    }

    // 浮點結果使用相對尺度容差；效能數字只有在答案正確時才有意義。
    bool correct = true;
    for (std::size_t index = 0; index < count; ++index) {
        const float tolerance = 1.0e-5F * (1.0F + std::abs(cpu_y[index]));
        if (std::abs(cpu_y[index] - gpu_y[index]) > tolerance) {
            correct = false;
            break;
        }
    }

    CUDA_CHECK(cudaEventDestroy(start_event));
    CUDA_CHECK(cudaEventDestroy(stop_event));
    CUDA_CHECK(cudaFree(device_x));
    CUDA_CHECK(cudaFree(device_y));

    return Result{count, best_cpu_ms, static_cast<double>(best_kernel_ms),
                  best_end_to_end_ms, correct};
}

}  // namespace

int main() {
    int device = 0;
    cudaDeviceProp properties{};
    CUDA_CHECK(cudaGetDevice(&device));
    CUDA_CHECK(cudaGetDeviceProperties(&properties, device));

    std::cout << "裝置：" << properties.name << "（sm_" << properties.major
              << properties.minor << "）\n"
              << "實驗：SAXPY y = 2.5*x + y；每欄取多次測量的最短時間。\n"
              << "GPU 核心時間不含傳輸；端到端時間包含 H2D + kernel + D2H，"
                 "但不含配置。\n\n";

    // 欄名採 ASCII，避免不同終端對中文字寬的計算方式造成錯位。
    std::cout << std::right << std::setw(12) << "N"
              << std::setw(15) << "CPU ms"
              << std::setw(15) << "kernel ms"
              << std::setw(15) << "E2E ms"
              << std::setw(15) << "CPU/kernel"
              << std::setw(15) << "CPU/E2E"
              << std::setw(10) << "check" << '\n';
    std::cout << std::string(97, '-') << '\n';

    bool all_correct = true;
    const std::vector<std::size_t> sizes{1U << 10U, 1U << 20U, 1U << 24U};
    std::cout << std::fixed << std::setprecision(6);
    for (const std::size_t size : sizes) {
        const Result result = benchmark(size);
        all_correct = all_correct && result.correct;
        std::cout << std::setw(12) << result.elements
                  << std::setw(15) << result.cpu_ms
                  << std::setw(15) << result.kernel_ms
                  << std::setw(15) << result.end_to_end_ms
                  << std::setw(14) << result.cpu_ms / result.kernel_ms << 'x'
                  << std::setw(14) << result.cpu_ms / result.end_to_end_ms << 'x'
                  << std::setw(10) << (result.correct ? "PASS" : "FAIL") << '\n';
    }

    std::cout << "\n提醒：這是本機觀察，不是 CPU/GPU 的普遍排名。工作量、資料位置、"
                 "編譯器與硬體都會改變結果。\n";
    return all_correct ? 0 : 1;
}
