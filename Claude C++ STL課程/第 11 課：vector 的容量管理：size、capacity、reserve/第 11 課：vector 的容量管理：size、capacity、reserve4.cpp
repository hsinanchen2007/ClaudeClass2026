#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    std::cout << "初始: size=" << v.size() << std::endl;  // 5
    
    // 擴大：新元素用預設值（0）填充
    v.resize(8);
    std::cout << "resize(8): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3 4 5 0 0 0
    std::cout << std::endl;
    
    // 擴大並指定填充值
    v.resize(10, 99);
    std::cout << "resize(10, 99): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3 4 5 0 0 0 99 99
    std::cout << std::endl;
    
    // 縮小：多餘的元素被銷毀
    v.resize(3);
    std::cout << "resize(3): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3
    std::cout << std::endl;
    
    // 注意：縮小後 capacity 通常不變
    std::cout << "縮小後 capacity: " << v.capacity() << std::endl;  // 仍然 >= 10
    
    return 0;
}
