#include <charconv>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using ProcessorCount = std::uint64_t;
constexpr ProcessorCount kMaxProcessorCount = 1'000'000'000'000ULL;

double amdahl_speedup(double amdahl_serial_fraction, ProcessorCount processor_count) {
    // 固定問題大小：只有可平行部分能被 processors 分攤。
    const double processors = static_cast<double>(processor_count);
    return 1.0 /
           (amdahl_serial_fraction + (1.0 - amdahl_serial_fraction) / processors);
}

double gustafson_speedup(double gustafson_serial_fraction, ProcessorCount processor_count) {
    // 固定執行時間、放大問題規模：平行部分隨 processors 成長。
    const double processors = static_cast<double>(processor_count);
    return processors - gustafson_serial_fraction * (processors - 1.0);
}

double parse_double(const char* text, const char* name) {
    // stod 可能只吃掉前綴（例如 0.1abc）；used 確保整個參數都合法。
    std::size_t used = 0;
    const std::string value{text};
    const double result = std::stod(value, &used);
    if (used != value.size() || !std::isfinite(result)) {
        throw std::invalid_argument(std::string{name} + " 不是有效數字");
    }
    return result;
}

ProcessorCount parse_processor_count(const char* text) {
    // from_chars 不接受小數、正負號或尾端垃圾，也能可靠偵測 64-bit 溢位。
    const std::string_view value{text};
    ProcessorCount result = 0;
    const auto [end, error] =
        std::from_chars(value.data(), value.data() + value.size(), result, 10);
    if (error != std::errc{} || end != value.data() + value.size() || result == 0 ||
        result > kMaxProcessorCount) {
        throw std::invalid_argument(
            "處理器數量必須是 1 到 1000000000000 的十進位整數");
    }
    return result;
}

void print_row(double serial_fraction, ProcessorCount processors) {
    const double parallel_fraction = 1.0 - serial_fraction;
    std::cout << std::setw(8) << serial_fraction * 100.0 << "%"
              << std::setw(11) << parallel_fraction * 100.0 << "%"
              << std::setw(14) << processors
              << std::setw(18) << amdahl_speedup(serial_fraction, processors)
              << std::setw(20) << gustafson_speedup(serial_fraction, processors)
              << '\n';
}

void print_header() {
    std::cout << std::fixed << std::setprecision(2)
              << "  串行比例    平行比例          P      Amdahl 加速比     Gustafson 加速比\n"
              << "------------------------------------------------------------------------\n";
}

void print_fraction_scope_note() {
    std::cout << "\n模型邊界：表中同一百分比只是並列示意；Amdahl 的 s_A 來自固定問題"
                 "基準，Gustafson 的 s_G 來自擴張問題的平行執行，兩者不是同一個實測量。\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 3) {
            const double serial_fraction = parse_double(argv[1], "串行比例");
            const ProcessorCount processors = parse_processor_count(argv[2]);
            if (serial_fraction < 0.0 || serial_fraction > 1.0) {
                throw std::invalid_argument("串行比例必須介於 0 與 1 之間");
            }
            print_header();
            print_row(serial_fraction, processors);
            std::cout << "\n解讀：Amdahl 固定問題大小；Gustafson 隨處理器增加問題規模。\n";
            print_fraction_scope_note();
            return 0;
        }

        if (argc != 1) {
            std::cerr << "用法：" << argv[0] << " [串行比例(0..1) 處理器數量]\n";
            return 2;
        }

        // 預設表格刻意固定 P、逐步降低串行比例，方便比較何者影響更大。
        const std::vector<double> serial_fractions{0.50, 0.10, 0.05, 0.01};
        const std::vector<ProcessorCount> processor_counts{8, 64, 1024};

        std::cout << "固定問題與等比例擴張問題的理論上限\n\n";
        print_header();
        for (const double serial_fraction : serial_fractions) {
            for (const ProcessorCount processors : processor_counts) {
                print_row(serial_fraction, processors);
            }
            std::cout << '\n';
        }

        print_fraction_scope_note();

        std::cout << "試著執行：" << argv[0] << " 0.05 64\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "錯誤：" << error.what() << '\n';
        return 2;
    }
}
