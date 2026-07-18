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
    const int N = 100'000;
    std::vector<int> source(N, 42);

    // 方法一：逐一 push_back
    {
        std::vector<int> v;
        v.reserve(N);
        Timer t("逐一 push_back");
        for (int x : source) {
            v.push_back(x);
        }
    }

    // 方法二：批量 insert（一次性從範圍插入）
    {
        Timer t("批量 insert");
        std::vector<int> v;
        v.reserve(N);
        v.insert(v.end(), source.begin(), source.end());
    }

    // 方法三：用範圍建構子
    {
        Timer t("範圍建構子");
        std::vector<int> v(source.begin(), source.end());
    }

    // 方法四：assign
    {
        Timer t("assign");
        std::vector<int> v;
        v.assign(source.begin(), source.end());
    }

    return 0;
}
