#include <vector>
#include <iostream>
#include <chrono>

int main() {
    const int N = 100000000;
    std::vector<int> v(1000);
    
    // 填入資料
    for (int i = 0; i < 1000; ++i) {
        v[i] = i;
    }
    
    long long sum1 = 0, sum2 = 0;
    
    // 測試 operator[]
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        sum1 += v[i % 1000];
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    
    // 測試 at()
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        sum2 += v.at(i % 1000);
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
    std::cout << "operator[]: " << duration1.count() << " ms" << std::endl;
    std::cout << "at():       " << duration2.count() << " ms" << std::endl;
    
    // 防止編譯器優化掉
    std::cout << "sums: " << sum1 << ", " << sum2 << std::endl;
    
    return 0;
}
