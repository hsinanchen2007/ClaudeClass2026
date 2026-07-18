#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    // 半開區間的好處：
    
    // 1. 空容器的判斷很簡單
    std::vector<int> empty_vec;
    if (empty_vec.begin() == empty_vec.end()) {
        std::cout << "空容器：begin() == end()" << std::endl;
    }
    
    // 2. 遍歷邏輯統一
    std::cout << "\n遍歷非空容器: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 3. 計算元素個數
    std::cout << "\n元素個數: " << (vec.end() - vec.begin()) << std::endl;
    
    // 4. 子區間表示
    auto start = vec.begin() + 1;  // 指向 20
    auto finish = vec.begin() + 4;  // 指向 50（不包含）
    
    std::cout << "子區間 [1, 4): ";
    for (auto it = start; it != finish; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
