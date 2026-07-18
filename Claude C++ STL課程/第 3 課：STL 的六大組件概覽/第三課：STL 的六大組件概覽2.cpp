#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    // 方法一：使用迭代器遍歷
    std::cout << "使用迭代器: ";
    for (std::vector<int>::iterator it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";  // *it 取得迭代器指向的值
    }
    std::cout << std::endl;
    
    // 方法二：使用 auto 簡化
    std::cout << "使用 auto: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 方法三：範圍 for（底層也是迭代器）
    std::cout << "範圍 for: ";
    for (int n : vec) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
