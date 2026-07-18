#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 讀取首尾
    std::cout << "front: " << v.front() << std::endl;  // 10
    std::cout << "back:  " << v.back() << std::endl;   // 50
    
    // 等價於
    std::cout << "v[0]:          " << v[0] << std::endl;
    std::cout << "v[size()-1]:   " << v[v.size() - 1] << std::endl;
    
    // 修改首尾
    v.front() = 100;
    v.back() = 500;
    
    for (int x : v) {
        std::cout << x << " ";  // 100 20 30 40 500
    }
    std::cout << std::endl;
    
    return 0;
}
