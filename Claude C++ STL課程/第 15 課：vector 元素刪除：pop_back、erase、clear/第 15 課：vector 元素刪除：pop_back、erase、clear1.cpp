#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    
    std::cout << "原始: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    
    v.pop_back();  // 刪除 5
    v.pop_back();  // 刪除 4
    
    std::cout << "pop_back 兩次後: ";
    for (int x : v) std::cout << x << " ";  // 1 2 3
    std::cout << std::endl;
    
    std::cout << "size: " << v.size() << std::endl;        // 3
    std::cout << "capacity: " << v.capacity() << std::endl; // 仍然 >= 5
    
    return 0;
}
