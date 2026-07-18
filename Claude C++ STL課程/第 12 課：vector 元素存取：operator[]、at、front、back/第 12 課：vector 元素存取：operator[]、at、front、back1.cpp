#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 讀取
    std::cout << "v[0] = " << v[0] << std::endl;  // 10
    std::cout << "v[2] = " << v[2] << std::endl;  // 30
    
    // 修改
    v[1] = 200;
    std::cout << "修改後 v[1] = " << v[1] << std::endl;  // 200
    
    return 0;
}
