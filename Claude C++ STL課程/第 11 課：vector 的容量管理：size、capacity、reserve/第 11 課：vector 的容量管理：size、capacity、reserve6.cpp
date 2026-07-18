#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    v.reserve(1000);
    
    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
    }
    
    std::cout << "shrink 前: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=10, capacity=1000
    
    v.shrink_to_fit();
    
    std::cout << "shrink 後: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=10, capacity 約等於 10（不保證）
    
    return 0;
}
