#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

double amdahl_speedup(double serial_fraction, double processors) {
    // 固定問題大小：只有可平行部分能被 processors 分攤。
    return 1.0 / (serial_fraction + (1.0 - serial_fraction) / processors);
}

double gustafson_speedup(double serial_fraction, double processors) {
    // 固定執行時間、放大問題規模：平行部分隨 processors 成長。
    return processors - serial_fraction * (processors - 1.0);
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

void print_row(double serial_fraction, double processors) {
    const double parallel_fraction = 1.0 - serial_fraction;
    std::cout << std::setw(8) << serial_fraction * 100.0 << "%"
              << std::setw(11) << parallel_fraction * 100.0 << "%"
              << std::setw(10) << static_cast<unsigned long long>(processors)
              << std::setw(16) << amdahl_speedup(serial_fraction, processors)
              << std::setw(18) << gustafson_speedup(serial_fraction, processors)
              << '\n';
}

void print_header() {
    std::cout << std::fixed << std::setprecision(2)
              << "  串行比例    平行比例      P    Amdahl 加速比   Gustafson 加速比\n"
              << "----------------------------------------------------------------\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 3) {
            const double serial_fraction = parse_double(argv[1], "串行比例");
            const double processors = parse_double(argv[2], "處理器數量");
            if (serial_fraction < 0.0 || serial_fraction > 1.0) {
                throw std::invalid_argument("串行比例必須介於 0 與 1 之間");
            }
            if (processors < 1.0 || std::floor(processors) != processors) {
                throw std::invalid_argument("處理器數量必須是至少 1 的整數");
            }

            print_header();
            print_row(serial_fraction, processors);
            std::cout << "\n解讀：Amdahl 固定問題大小；Gustafson 隨處理器增加問題規模。\n";
            return 0;
        }

        if (argc != 1) {
            std::cerr << "用法：" << argv[0] << " [串行比例(0..1) 處理器數量]\n";
            return 2;
        }

        // 預設表格刻意固定 P、逐步降低串行比例，方便比較何者影響更大。
        const std::vector<double> serial_fractions{0.50, 0.10, 0.05, 0.01};
        const std::vector<double> processor_counts{8.0, 64.0, 1024.0};

        std::cout << "固定問題與等比例擴張問題的理論上限\n\n";
        print_header();
        for (const double serial_fraction : serial_fractions) {
            for (const double processors : processor_counts) {
                print_row(serial_fraction, processors);
            }
            std::cout << '\n';
        }

        std::cout << "試著執行：" << argv[0] << " 0.05 64\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "錯誤：" << error.what() << '\n';
        return 2;
    }
}
