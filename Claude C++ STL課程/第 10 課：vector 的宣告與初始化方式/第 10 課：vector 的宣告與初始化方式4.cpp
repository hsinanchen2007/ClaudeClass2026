#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1(5, 10);   // 5 個元素，每個都是 10
    std::vector<int> v2{5, 10};   // 2 個元素，分別是 5 和 10
    
    std::cout << "v1 size: " << v1.size() << std::endl;  // 輸出：5
    std::cout << "v2 size: " << v2.size() << std::endl;  // 輸出：2
    
    std::cout << "v1: ";
    for (int x : v1) std::cout << x << " ";  // 輸出：10 10 10 10 10
    std::cout << std::endl;
    
    std::cout << "v2: ";
    for (int x : v2) std::cout << x << " ";  // 輸出：5 10
    std::cout << std::endl;
    
    return 0;
}
