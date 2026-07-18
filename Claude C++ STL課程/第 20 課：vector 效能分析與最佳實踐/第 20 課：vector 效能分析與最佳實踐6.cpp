#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <numeric>  // std::accumulate

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

// 錯誤：參數用值傳遞，每次呼叫都會拷貝整個 vector
long long sum_by_value(std::vector<int> v) {
    return std::accumulate(v.begin(), v.end(), 0LL);
}

// 正確：參數用 const 引用，零拷貝
long long sum_by_ref(const std::vector<int>& v) {
    return std::accumulate(v.begin(), v.end(), 0LL);
}

int main() {
    std::vector<int> data(1'000'000, 1);

    {
        Timer t("值傳遞（拷貝）");
        for (int i = 0; i < 100; ++i) {
            sum_by_value(data);
        }
    }

    {
        Timer t("const 引用（零拷貝）");
        for (int i = 0; i < 100; ++i) {
            sum_by_ref(data);
        }
    }

    return 0;
}
