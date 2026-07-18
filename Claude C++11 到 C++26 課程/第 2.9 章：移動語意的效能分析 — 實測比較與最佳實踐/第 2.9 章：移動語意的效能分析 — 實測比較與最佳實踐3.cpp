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


int main() {
    const int N = 10000;
    std::vector<int> source(100000, 42);  // 10 萬個 int

    {
        Timer t("複製 vector");
        for (int i = 0; i < N; ++i) {
            std::vector<int> copy = source;
        }
    }

    {
        Timer t("移動 vector");
        for (int i = 0; i < N; ++i) {
            std::vector<int> temp = source;
            std::vector<int> moved = std::move(temp);
        }
    }

    return 0;
}
