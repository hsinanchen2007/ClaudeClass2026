#include <iostream>
#include <string>
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
    const int N = 2000000;
    std::string source(10000, 'x');  // 10000 字元

    {
        Timer t("複製");
        for (int i = 0; i < N; ++i) {
            std::string copy = source;
        }
    }

    {
        Timer t("移動");
        for (int i = 0; i < N; ++i) {
            std::string temp = source;            // 先複製一份供移動
            std::string moved = std::move(temp);  // 移動
        }
    }

    return 0;
}
