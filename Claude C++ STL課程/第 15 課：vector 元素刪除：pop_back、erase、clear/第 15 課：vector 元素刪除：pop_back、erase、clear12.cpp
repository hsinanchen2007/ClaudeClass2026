// ### 十、效能比較
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>

int main() {
    const int N = 100000;
    
    // 方法 1：逐一 erase（O(n²)）
    auto start1 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
        
        for (auto it = v.begin(); it != v.end(); ) {
            if (*it % 2 == 0) {
                it = v.erase(it);
            } else {
                ++it;
            }
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    
    // 方法 2：Erase-Remove 慣用法（O(n)）
    auto start2 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v;
        for (int i = 0; i < N; ++i) v.push_back(i);
        
        v.erase(std::remove_if(v.begin(), v.end(),
                               [](int x) { return x % 2 == 0; }),
                v.end());
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
    std::cout << "逐一 erase:      " << duration1.count() << " ms" << std::endl;
    std::cout << "Erase-Remove:    " << duration2.count() << " ms" << std::endl;
    
    return 0;
}