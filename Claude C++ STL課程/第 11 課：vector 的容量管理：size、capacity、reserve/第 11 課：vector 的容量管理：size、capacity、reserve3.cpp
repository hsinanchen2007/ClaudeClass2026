#include <vector>
#include <iostream>
#include <chrono>

int main() {
    const int N = 1000000;
    
    // 不使用 reserve
    auto start1 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v1;
        for (int i = 0; i < N; ++i) {
            v1.push_back(i);
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    
    // 使用 reserve
    auto start2 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v2;
        v2.reserve(N);
        for (int i = 0; i < N; ++i) {
            v2.push_back(i);
        }
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);
    
    std::cout << "不使用 reserve: " << duration1.count() << " 微秒" << std::endl;
    std::cout << "使用 reserve:   " << duration2.count() << " 微秒" << std::endl;
    
    return 0;
}
