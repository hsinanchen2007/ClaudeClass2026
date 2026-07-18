#include <iostream>
#include <vector>
#include <chrono>
#include <string>

struct Timer {
    std::chrono::high_resolution_clock::time_point start_;
    std::string label_;
    Timer(const std::string& label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << label_ << ": " << us << " μs" << std::endl;
    }
};

int main() {
    const int N = 500'000;

    // 測試：string 的建構（拷貝成本較高的型別）
    {
        Timer t("push_back string");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(std::string("hello_world_test_string"));
        }
    }

    {
        Timer t("emplace_back string");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.emplace_back("hello_world_test_string");
        }
    }

    // 測試：int（拷貝成本極低的型別）
    {
        Timer t("push_back int");
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
        }
    }

    {
        Timer t("emplace_back int");
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.emplace_back(i);
        }
    }

    return 0;
}
