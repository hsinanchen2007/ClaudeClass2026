#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    // 二分搜尋要求已排序！
    
    // binary_search：是否存在, 這裡檢查 vec 中是否存在元素 5 和 11
    std::cout << "=== binary_search ===" << std::endl;
    std::cout << "存在 5: " << (std::binary_search(vec.begin(), vec.end(), 5) ? "是" : "否") << std::endl;
    std::cout << "存在 11: " << (std::binary_search(vec.begin(), vec.end(), 11) ? "是" : "否") << std::endl;
    
    // lower_bound：第一個 >= 值的位置, 這裡找到 vec 中第一個大於或等於 5 的元素的位置和數值
    std::cout << "\n=== lower_bound ===" << std::endl;
    auto lb = std::lower_bound(vec.begin(), vec.end(), 5);
    std::cout << "lower_bound(5): 位置 " << (lb - vec.begin()) << ", 值 " << *lb << std::endl;
    
    // upper_bound：第一個 > 值的位置, 這裡找到 vec 中第一個大於 5 的元素的位置和數值
    std::cout << "\n=== upper_bound ===" << std::endl;
    auto ub = std::upper_bound(vec.begin(), vec.end(), 5);
    std::cout << "upper_bound(5): 位置 " << (ub - vec.begin()) << ", 值 " << *ub << std::endl;
    
    // equal_range：lower_bound 和 upper_bound 的組合, 這裡找到 vec 中等於 2 的元素的範圍（位置和數量）
    std::cout << "\n=== equal_range ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 2, 2, 3, 4, 5};
    auto range = std::equal_range(vec2.begin(), vec2.end(), 2);
    std::cout << "equal_range(2): [" << (range.first - vec2.begin()) 
              << ", " << (range.second - vec2.begin()) << ")" << std::endl;
    std::cout << "2 的數量: " << (range.second - range.first) << std::endl;
    
    return 0;
}
