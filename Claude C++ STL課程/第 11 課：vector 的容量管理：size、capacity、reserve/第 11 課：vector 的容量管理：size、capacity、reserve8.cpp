#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    v.reserve(100);
    
    std::cout << "初始: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    
    // 方法一：clear（保留 capacity）
    v.clear();
    std::cout << "clear(): size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=0, capacity 不變
    
    // 重新填入資料
    v = {1, 2, 3, 4, 5};
    
    // 方法二：swap 技巧（C++11 前常用，釋放記憶體）
    std::vector<int>().swap(v);
    std::cout << "swap 技巧: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=0, capacity=0
    
    // 重新填入資料
    v = {1, 2, 3, 4, 5};
    v.reserve(100);
    
    // 方法三：clear + shrink_to_fit（C++11 後推薦）
    v.clear();
    v.shrink_to_fit();
    std::cout << "clear + shrink: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=0, capacity 約為 0
    
    return 0;
}
