#include <cuda_runtime.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

class UsageError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class CudaError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

void check_cuda(cudaError_t status, const char *expression, const char *file,
                int line) {
  if (status == cudaSuccess) {
    return;
  }
  std::ostringstream message;
  message << file << ':' << line << ": " << expression
          << " failed: " << cudaGetErrorName(status) << " ("
          << cudaGetErrorString(status) << ')';
  throw CudaError(message.str());
}

#define CUDA_CHECK(expression)                                                 \
  check_cuda((expression), #expression, __FILE__, __LINE__)

struct Options {
  int device = 0;
  int blocks_per_sm = 4;
};

struct Sample {
  std::uint32_t block;
  std::uint32_t thread;
  std::uint32_t warp_in_block;
  std::uint32_t lane;
  std::uint32_t sm_id;
  std::uint32_t sm_id_upper_bound;
};

__device__ __forceinline__ std::uint32_t read_special_register_smid() {
  std::uint32_t value;
  asm volatile("mov.u32 %0, %%smid;" : "=r"(value));
  return value;
}

__device__ __forceinline__ std::uint32_t read_special_register_nsmid() {
  std::uint32_t value;
  asm volatile("mov.u32 %0, %%nsmid;" : "=r"(value));
  return value;
}

__device__ __forceinline__ std::uint32_t read_special_register_laneid() {
  std::uint32_t value;
  asm volatile("mov.u32 %0, %%laneid;" : "=r"(value));
  return value;
}

__global__ void sample_execution_topology(Sample *samples) {
  const std::size_t index =
      static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  samples[index] = Sample{
      blockIdx.x,
      threadIdx.x,
      threadIdx.x / warpSize,
      read_special_register_laneid(),
      read_special_register_smid(),
      read_special_register_nsmid(),
  };
}

void print_usage(std::ostream &stream) {
  stream << R"(gpu_arch_probe - query CUDA-visible GPU architecture resources

Usage:
  gpu_arch_probe [--device N] [--blocks-per-sm N]

Options:
  --device N         CUDA device ordinal (default: 0)
  --blocks-per-sm N  Sampling grid multiplier, 1..32 (default: 4)
  -h, --help         Show this help

Exit codes:
  0  Probe and all runtime invariants passed
  1  CUDA/runtime or validation failure
  2  Invalid command-line usage
)";
}

int parse_bounded_integer(std::string_view text, int minimum, int maximum,
                          std::string_view option) {
  int value = 0;
  const char *begin = text.data();
  const char *end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end || value < minimum ||
      value > maximum) {
    std::ostringstream message;
    message << option << " requires an integer in [" << minimum << ", "
            << maximum << "], got '" << text << '\'';
    throw UsageError(message.str());
  }
  return value;
}

Options parse_options(int argc, char **argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument(argv[index]);
    if (argument == "-h" || argument == "--help") {
      print_usage(std::cout);
      std::exit(0);
    }
    if (argument == "--device" || argument == "--blocks-per-sm") {
      if (index + 1 >= argc) {
        throw UsageError(std::string(argument) + " requires a value");
      }
      const std::string_view value(argv[++index]);
      if (argument == "--device") {
        options.device = parse_bounded_integer(
            value, 0, std::numeric_limits<int>::max(), argument);
      } else {
        options.blocks_per_sm = parse_bounded_integer(value, 1, 32, argument);
      }
      continue;
    }
    throw UsageError("unknown argument '" + std::string(argument) + "'");
  }
  return options;
}

int device_attribute(cudaDeviceAttr attribute, int device) {
  int value = 0;
  CUDA_CHECK(cudaDeviceGetAttribute(&value, attribute, device));
  return value;
}

std::string cuda_version_string(int encoded) {
  std::ostringstream value;
  value << encoded / 1000 << '.' << (encoded % 1000) / 10;
  return value.str();
}

double bytes_to_mib(std::size_t bytes) {
  return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

void print_sm_ids(const std::set<std::uint32_t> &ids) {
  bool first = true;
  for (const std::uint32_t id : ids) {
    if (!first) {
      std::cout << ',';
    }
    first = false;
    std::cout << id;
  }
  std::cout << '\n';
}

int run(const Options &options) {
  int device_count = 0;
  CUDA_CHECK(cudaGetDeviceCount(&device_count));
  if (options.device >= device_count) {
    std::ostringstream message;
    message << "--device " << options.device << " is out of range; found "
            << device_count << " CUDA device(s)";
    throw UsageError(message.str());
  }

  CUDA_CHECK(cudaSetDevice(options.device));

  cudaDeviceProp properties{};
  CUDA_CHECK(cudaGetDeviceProperties(&properties, options.device));

  int driver_version = 0;
  int runtime_version = 0;
  CUDA_CHECK(cudaDriverGetVersion(&driver_version));
  CUDA_CHECK(cudaRuntimeGetVersion(&runtime_version));

  char pci_bus_id[32]{};
  CUDA_CHECK(
      cudaDeviceGetPCIBusId(pci_bus_id, sizeof(pci_bus_id), options.device));

  std::size_t free_global_bytes = 0;
  std::size_t total_global_bytes = 0;
  CUDA_CHECK(cudaMemGetInfo(&free_global_bytes, &total_global_bytes));

  const int sm_count =
      device_attribute(cudaDevAttrMultiProcessorCount, options.device);
  const int warp_size = device_attribute(cudaDevAttrWarpSize, options.device);
  const int max_threads_per_sm =
      device_attribute(cudaDevAttrMaxThreadsPerMultiProcessor, options.device);
  const int max_threads_per_block =
      device_attribute(cudaDevAttrMaxThreadsPerBlock, options.device);
  const int max_blocks_per_sm =
      device_attribute(cudaDevAttrMaxBlocksPerMultiprocessor, options.device);
  const int registers_per_sm = device_attribute(
      cudaDevAttrMaxRegistersPerMultiprocessor, options.device);
  const int shared_bytes_per_sm = device_attribute(
      cudaDevAttrMaxSharedMemoryPerMultiprocessor, options.device);
  const int shared_bytes_per_block =
      device_attribute(cudaDevAttrMaxSharedMemoryPerBlock, options.device);
  const int optin_shared_bytes_per_block =
      device_attribute(cudaDevAttrMaxSharedMemoryPerBlockOptin, options.device);
  const int l2_bytes = device_attribute(cudaDevAttrL2CacheSize, options.device);
  const int memory_clock_khz =
      device_attribute(cudaDevAttrMemoryClockRate, options.device);
  const int memory_bus_width_bits =
      device_attribute(cudaDevAttrGlobalMemoryBusWidth, options.device);
  const int async_engine_count =
      device_attribute(cudaDevAttrAsyncEngineCount, options.device);
  const int concurrent_kernels =
      device_attribute(cudaDevAttrConcurrentKernels, options.device);
  const int unified_addressing =
      device_attribute(cudaDevAttrUnifiedAddressing, options.device);

  if (sm_count <= 0 || warp_size <= 0 || max_threads_per_sm <= 0 ||
      max_threads_per_block <= 0 || max_blocks_per_sm <= 0 ||
      registers_per_sm <= 0 || shared_bytes_per_sm <= 0 || l2_bytes <= 0 ||
      memory_clock_khz <= 0 || memory_bus_width_bits <= 0) {
    throw std::runtime_error(
        "CUDA returned a non-positive required architecture attribute");
  }

  constexpr int threads_per_block = 64;
  if (threads_per_block > max_threads_per_block ||
      threads_per_block % warp_size != 0) {
    throw std::runtime_error(
        "the teaching kernel requires a warp size dividing 64 and at "
        "least 64 threads per block");
  }

  const int grid_blocks = sm_count * options.blocks_per_sm;
  const std::size_t sample_count =
      static_cast<std::size_t>(grid_blocks) * threads_per_block;
  std::vector<Sample> samples(sample_count);
  Sample *device_samples = nullptr;
  CUDA_CHECK(cudaMalloc(reinterpret_cast<void **>(&device_samples),
                        sample_count * sizeof(Sample)));
  try {
    sample_execution_topology<<<grid_blocks, threads_per_block>>>(
        device_samples);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaMemcpy(samples.data(), device_samples,
                          sample_count * sizeof(Sample),
                          cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(device_samples));
    device_samples = nullptr;
  } catch (...) {
    if (device_samples != nullptr) {
      cudaFree(device_samples);
    }
    throw;
  }

  std::vector<std::int64_t> block_to_sm(static_cast<std::size_t>(grid_blocks),
                                        -1);
  std::set<std::uint32_t> observed_sm_ids;
  std::uint32_t observed_sm_id_upper_bound = 0;
  std::size_t invalid_samples = 0;
  std::size_t block_sm_conflicts = 0;

  for (std::size_t index = 0; index < samples.size(); ++index) {
    const Sample &sample = samples[index];
    const std::uint32_t expected_block =
        static_cast<std::uint32_t>(index / threads_per_block);
    const std::uint32_t expected_thread =
        static_cast<std::uint32_t>(index % threads_per_block);
    const std::uint32_t expected_lane =
        expected_thread % static_cast<std::uint32_t>(warp_size);
    const std::uint32_t expected_warp =
        expected_thread / static_cast<std::uint32_t>(warp_size);

    const bool valid =
        sample.block == expected_block && sample.thread == expected_thread &&
        sample.lane == expected_lane && sample.warp_in_block == expected_warp &&
        sample.sm_id_upper_bound > 0 && sample.sm_id < sample.sm_id_upper_bound;
    if (!valid) {
      ++invalid_samples;
      continue;
    }

    if (observed_sm_id_upper_bound == 0) {
      observed_sm_id_upper_bound = sample.sm_id_upper_bound;
    } else if (observed_sm_id_upper_bound != sample.sm_id_upper_bound) {
      ++invalid_samples;
    }

    std::int64_t &assigned_sm = block_to_sm[sample.block];
    if (assigned_sm == -1) {
      assigned_sm = sample.sm_id;
    } else if (assigned_sm != sample.sm_id) {
      ++block_sm_conflicts;
    }
    observed_sm_ids.insert(sample.sm_id);
  }

  int active_blocks_per_sm = 0;
  CUDA_CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(
      &active_blocks_per_sm, sample_execution_topology, threads_per_block, 0));
  const int active_threads_per_sm = active_blocks_per_sm * threads_per_block;
  const double occupancy_limit =
      static_cast<double>(active_threads_per_sm) / max_threads_per_sm;

  const double theoretical_memory_bandwidth_gb_s =
      2.0 * static_cast<double>(memory_clock_khz) * 1000.0 *
      (static_cast<double>(memory_bus_width_bits) / 8.0) / 1.0e9;

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "probe.format=1\n";
  std::cout << "cuda.driver_version=" << cuda_version_string(driver_version)
            << '\n';
  std::cout << "cuda.runtime_version=" << cuda_version_string(runtime_version)
            << '\n';
  std::cout << "device.index=" << options.device << '\n';
  std::cout << "device.name=" << properties.name << '\n';
  std::cout << "device.pci_bus_id=" << pci_bus_id << '\n';
  std::cout << "device.compute_capability=" << properties.major << '.'
            << properties.minor << '\n';
  std::cout << "device.global_memory_mib="
            << bytes_to_mib(properties.totalGlobalMem) << '\n';
  std::cout << "device.global_memory_free_now_mib="
            << bytes_to_mib(free_global_bytes) << '\n';
  std::cout << "device.global_memory_runtime_total_mib="
            << bytes_to_mib(total_global_bytes) << '\n';
  std::cout << "sm.count=" << sm_count << '\n';
  std::cout << "sm.warp_size=" << warp_size << '\n';
  std::cout << "sm.max_threads=" << max_threads_per_sm << '\n';
  std::cout << "sm.max_blocks=" << max_blocks_per_sm << '\n';
  std::cout << "sm.registers_32bit=" << registers_per_sm << '\n';
  std::cout << "sm.shared_memory_bytes=" << shared_bytes_per_sm << '\n';
  std::cout << "block.max_threads=" << max_threads_per_block << '\n';
  std::cout << "block.shared_memory_bytes=" << shared_bytes_per_block << '\n';
  std::cout << "block.optin_shared_memory_bytes="
            << optin_shared_bytes_per_block << '\n';
  std::cout << "cache.l2_bytes=" << l2_bytes << '\n';
  std::cout << "memory.peak_clock_khz=" << memory_clock_khz << '\n';
  std::cout << "memory.bus_width_bits=" << memory_bus_width_bits << '\n';
  std::cout << "memory.theoretical_peak_gb_s="
            << theoretical_memory_bandwidth_gb_s << '\n';
  std::cout << "device.async_engine_count=" << async_engine_count << '\n';
  std::cout << "device.concurrent_kernels=" << concurrent_kernels << '\n';
  std::cout << "device.unified_addressing=" << unified_addressing << '\n';
  std::cout << "sample.grid_blocks=" << grid_blocks << '\n';
  std::cout << "sample.threads_per_block=" << threads_per_block << '\n';
  std::cout << "sample.warps_per_block=" << threads_per_block / warp_size
            << '\n';
  std::cout << "sample.observed_unique_sm_ids=" << observed_sm_ids.size()
            << '\n';
  std::cout << "sample.ptx_nsmid_upper_bound=" << observed_sm_id_upper_bound
            << '\n';
  std::cout << "sample.observed_sm_ids=";
  print_sm_ids(observed_sm_ids);
  std::cout << "sample.block_sm_conflicts=" << block_sm_conflicts << '\n';
  std::cout << "sample.invalid_records=" << invalid_samples << '\n';
  std::cout << "sample.kernel_active_blocks_per_sm=" << active_blocks_per_sm
            << '\n';
  std::cout << "sample.kernel_thread_occupancy_limit=" << occupancy_limit
            << '\n';

  // %smid is a volatile diagnostic register. A block normally reports one
  // SM in this short kernel, but PTX permits the sampled value to change
  // after preemption/rescheduling, so conflicts are reported, not failed.
  const bool passed =
      invalid_samples == 0 && !observed_sm_ids.empty() &&
      observed_sm_ids.size() <= static_cast<std::size_t>(sm_count) &&
      active_blocks_per_sm > 0 && occupancy_limit > 0.0 &&
      occupancy_limit <= 1.0;
  std::cout << "sample.validation=" << (passed ? "PASS" : "FAIL") << '\n';
  return passed ? 0 : 1;
}

} // namespace

int main(int argc, char **argv) {
  try {
    return run(parse_options(argc, argv));
  } catch (const UsageError &error) {
    std::cerr << "usage error: " << error.what() << "\n\n";
    print_usage(std::cerr);
    return 2;
  } catch (const std::exception &error) {
    std::cerr << "probe failed: " << error.what() << '\n';
    return 1;
  }
}
