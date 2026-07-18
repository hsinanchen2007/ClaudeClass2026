#include <iostream>
#include <vector>
#include <algorithm>
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

    // 方法一：逐一 erase（最慢）
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);

        Timer t("逐一 erase");
        for (auto it = v.begin(); it != v.end(); ) {
            if (*it % 2 == 0) {
                it = v.erase(it);  // 每次 erase 都搬移後面的元素
            } else {
                ++it;
            }
        }
    }

    // 方法二：erase-remove 慣用法（快很多）
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);

        Timer t("erase-remove");
        v.erase(
            std::remove_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; }),
            v.end()
        );
    }

    // 方法三：C++20 std::erase_if（最簡潔）
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);

        Timer t("std::erase_if (C++20)");
        std::erase_if(v, [](int x) { return x % 2 == 0; });
    }

    return 0;
}
