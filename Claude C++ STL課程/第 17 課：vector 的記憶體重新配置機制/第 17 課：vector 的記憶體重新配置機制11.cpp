#include <iostream>
#include <vector>
#include <chrono>

int main() {
    const int N = 10'000'000;

    // ===== 不用 reserve =====
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<int> v;
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "不用 reserve：" << ms << " ms" << std::endl;
    }

    // ===== 使用 reserve =====
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "使用 reserve：" << ms << " ms" << std::endl;
    }

    return 0;
}
