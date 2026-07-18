#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 刪除第一個元素
    v.erase(v.begin());
    std::cout << "刪除第一個: ";
    for (int x : v) std::cout << x << " ";  // 20 30 40 50
    std::cout << std::endl;
    
    // 刪除索引 2 的元素（現在是 40）
    v.erase(v.begin() + 2);
    std::cout << "刪除索引 2: ";
    for (int x : v) std::cout << x << " ";  // 20 30 50
    std::cout << std::endl;
    
    // 刪除最後一個元素（等同 pop_back）
    v.erase(v.end() - 1);
    std::cout << "刪除最後: ";
    for (int x : v) std::cout << x << " ";  // 20 30
    std::cout << std::endl;
    
    return 0;
}
