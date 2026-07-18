#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    
    std::cout << "size: " << v.size() << std::endl;         // 5（實際元素數量）
    std::cout << "capacity: " << v.capacity() << std::endl; // >= 5（配置的空間）
    std::cout << "empty: " << v.empty() << std::endl;       // 0（false）
    
    return 0;
}
