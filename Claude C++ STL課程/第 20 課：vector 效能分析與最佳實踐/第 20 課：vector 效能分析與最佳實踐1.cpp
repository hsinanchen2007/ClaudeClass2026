#include <iostream>
#include <vector>
#include <chrono>
#include <string>

struct Timer {
    std::chrono::high_resolution_clock::time_point start_;
    std::string label_;

    Timer(const std::string& label) : label_(label) {
        start_ = std::chrono::high_resolution_clock::now();
    }

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << label_ << ": " << us << " μs" << std::endl;
    }
};

int main() {
    const int N = 1'000'000;

    // 測試一：不使用 reserve
    {
        Timer t("不用 reserve");
        std::vector<int> v;
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
        }
    }

    // 測試二：使用 reserve
    {
        Timer t("使用 reserve");
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
        }
    }

    // 測試三：直接用 resize + 索引賦值
    {
        Timer t("resize + 索引");
        std::vector<int> v(N);
        v.resize(N);
        for (int i = 0; i < N; ++i) {
            v[i] = i;
        }
    }

    return 0;
}
