#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 刪除 [begin+2, begin+5) 範圍，即索引 2, 3, 4
    v.erase(v.begin() + 2, v.begin() + 5);
    
    std::cout << "刪除範圍後: ";
    for (int x : v) std::cout << x << " ";  // 1 2 6 7 8 9 10
    std::cout << std::endl;
    
    // 刪除前三個元素
    v.erase(v.begin(), v.begin() + 3);
    
    std::cout << "再刪除前三個: ";
    for (int x : v) std::cout << x << " ";  // 7 8 9 10
    std::cout << std::endl;
    
    // 刪除所有元素（等同 clear）
    v.erase(v.begin(), v.end());
    std::cout << "size: " << v.size() << std::endl;  // 0
    
    return 0;
}
