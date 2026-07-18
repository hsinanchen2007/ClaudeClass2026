#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

int main() {
    const int SIZE = 1000000;
    std::vector<int> data(SIZE);
    
    // 填充隨機數據
    for (int i = 0; i < SIZE; ++i) {
        data[i] = rand();
    }
    
    // 測量 std::sort 的時間
    auto start = std::chrono::high_resolution_clock::now();
    std::sort(data.begin(), data.end());
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "排序 " << SIZE << " 個元素耗時: " 
              << duration.count() << " 毫秒" << std::endl;
    
    return 0;
}
