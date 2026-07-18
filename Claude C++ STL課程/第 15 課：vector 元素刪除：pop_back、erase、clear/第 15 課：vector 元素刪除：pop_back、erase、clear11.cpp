#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1 = {1, 2, 3, 2, 4, 2, 5};
    
    // C++20：刪除所有值為 2 的元素
    auto count1 = std::erase(v1, 2);
    std::cout << "刪除了 " << count1 << " 個 2: ";
    for (int x : v1) std::cout << x << " ";  // 1 3 4 5
    std::cout << std::endl;
    
    std::vector<int> v2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // C++20：刪除所有偶數
    auto count2 = std::erase_if(v2, [](int x) { return x % 2 == 0; });
    std::cout << "刪除了 " << count2 << " 個偶數: ";
    for (int x : v2) std::cout << x << " ";  // 1 3 5 7 9
    std::cout << std::endl;
    
    return 0;
}
