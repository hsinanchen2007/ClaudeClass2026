#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3};
    
    // 在位置 1 插入 4 個 0
    v.insert(v.begin() + 1, 4, 0);
    
    for (int x : v) {
        std::cout << x << " ";  // 1 0 0 0 0 2 3
    }
    std::cout << std::endl;
    
    return 0;
}
