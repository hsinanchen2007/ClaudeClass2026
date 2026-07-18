#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    
    std::cout << "初始 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    
    v.reserve(100);  // 預先配置至少 100 個元素的空間
    
    std::cout << "reserve(100) 後 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    // size 仍是 0，capacity >= 100
    
    // 現在連續 push_back 100 次都不會觸發擴容
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }
    
    std::cout << "100 次 push_back 後 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    
    return 0;
}
