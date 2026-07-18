#include <iostream>
#include <vector>
#include <utility>
#include <chrono>

class Timer {
    std::chrono::high_resolution_clock::time_point start_;
    const char* label_;
public:
    Timer(const char* label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto elapsed = std::chrono::high_resolution_clock::now() - start_;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        std::cout << label_ << ": " << ms << " ms\n";
    }
};

void bench(size_t size) {
    const int N = 5000;
    std::vector<int> source(size, 42);

    long long copy_ns, move_ns;

    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            std::vector<int> copy = source;
        }
        copy_ns = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start).count();
    }

    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            std::vector<int> temp = source;
            std::vector<int> moved = std::move(temp);
        }
        move_ns = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start).count();
    }

    std::cout << "size=" << size
              << "\t複製: " << copy_ns << " us"
              << "\t移動: " << move_ns << " us"
              << "\t加速比: " << (double)copy_ns / move_ns << "x\n";
}

int main() {
    bench(100);
    bench(1000);
    bench(10000);
    bench(100000);
    bench(1000000);
    return 0;
}
