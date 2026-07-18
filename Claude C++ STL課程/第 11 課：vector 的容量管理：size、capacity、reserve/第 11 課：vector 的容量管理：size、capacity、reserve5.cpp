#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1, v2;
    
    v1.reserve(5);
    v2.resize(5);
    
    std::cout << "reserve(5): size=" << v1.size() 
              << ", capacity=" << v1.capacity() << std::endl;
    // size=0, capacity>=5
    
    std::cout << "resize(5):  size=" << v2.size() 
              << ", capacity=" << v2.capacity() << std::endl;
    // size=5, capacity>=5
    
    // reserve 後不能直接用 operator[]
    // v1[0] = 10;  // 未定義行為！size 是 0
    
    // resize 後可以直接用 operator[]
    v2[0] = 10;  // 合法，因為 size 是 5
    
    return 0;
}
