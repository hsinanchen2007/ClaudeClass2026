#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    v.reserve(100);
    
    std::cout << "clear 前 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    
    v.clear();
    
    std::cout << "clear 後 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    // size = 0, capacity 不變
    
    return 0;
}
