#include <vector>
#include <iostream>
#include <chrono>

int main() {
    const int N = 100000;
    
    // 測試在開頭插入
    auto start1 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.insert(v.begin(), i);  // 每次都要搬移所有元素
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    
    // 測試在結尾插入
    auto start2 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(i);  // 不需要搬移
        }
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
    std::cout << "在開頭插入 " << N << " 次: " << duration1.count() << " ms" << std::endl;
    std::cout << "在結尾插入 " << N << " 次: " << duration2.count() << " ms" << std::endl;
    
    return 0;
}
