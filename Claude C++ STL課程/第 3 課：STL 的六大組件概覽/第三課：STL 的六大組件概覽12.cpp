#include <iostream>
#include <vector>
#include <memory>

int main() {
    // 所有容器都有一個預設的配置器
    // vector<int> 實際上是 vector<int, allocator<int>>
    
    std::vector<int> vec1;  // 使用預設配置器
    std::vector<int, std::allocator<int>> vec2;  // 明確指定（效果相同）
    
    vec1.push_back(1);
    vec2.push_back(2);
    
    std::cout << "vec1[0] = " << vec1[0] << std::endl;
    std::cout << "vec2[0] = " << vec2[0] << std::endl;
    
    // allocator 的基本用法
    std::allocator<int> alloc;
    
    // 配置記憶體（但不建構物件）
    int* ptr = alloc.allocate(5);  // 配置 5 個 int 的空間
    
    // 建構物件
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::construct(alloc, ptr + i, i * 10);
    }
    
    std::cout << "手動配置的陣列: ";
    for (int i = 0; i < 5; ++i) {
        std::cout << ptr[i] << " ";
    }
    std::cout << std::endl;
    
    // 解構物件
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::destroy(alloc, ptr + i);
    }
    
    // 釋放記憶體
    alloc.deallocate(ptr, 5);
    
    return 0;
}
