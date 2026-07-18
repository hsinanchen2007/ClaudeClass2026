#include <vector>
#include <iostream>

int main() {
    // 建立含有 5 個元素的 vector，每個元素都是預設值（int 的預設值是 0）
    std::vector<int> v1(5);
    
    for (int x : v1) {
        std::cout << x << " ";  // 輸出：0 0 0 0 0
    }
    std::cout << std::endl;
    
    // 建立含有 5 個元素的 vector，每個元素都是 42
    std::vector<int> v2(5, 42);
    
    for (int x : v2) {
        std::cout << x << " ";  // 輸出：42 42 42 42 42
    }
    std::cout << std::endl;
    
    return 0;
}
