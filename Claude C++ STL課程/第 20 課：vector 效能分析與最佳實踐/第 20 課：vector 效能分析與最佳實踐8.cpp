#include <iostream>
#include <vector>
#include <string>
#include <chrono>

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
    std::vector<std::string> data(100'000, "this_is_a_reasonably_long_string_for_testing");

    // 拷貝遍歷（每次迭代都拷貝一個 string）
    {
        Timer t("for (string s : data)");
        long long total = 0;
        for (std::string s : data) {
            total += s.size();
        }
    }

    // const 引用遍歷（零拷貝）
    {
        Timer t("for (const string& s : data)");
        long long total = 0;
        for (const std::string& s : data) {
            total += s.size();
        }
    }

    return 0;
}
