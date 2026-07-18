#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

int main() {
    std::vector<int> source = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> destination;
    
    // 組合多個 STL 元件：
    // 1. copy_if - 演算法
    // 2. back_inserter - 迭代器配接器
    // 3. lambda - 函數物件
    std::copy_if(
        source.begin(),           // 來源開始
        source.end(),             // 來源結束
        std::back_inserter(destination),  // 目標（自動擴展）
        [](int n) { return n % 2 == 0; }  // 條件：偶數
    );
    
    std::cout << "偶數: ";
    for (int n : destination) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
