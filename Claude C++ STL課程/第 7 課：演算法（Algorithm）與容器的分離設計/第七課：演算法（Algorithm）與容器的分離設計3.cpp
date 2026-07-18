#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 30, 50, 30};
    
    // find：找第一個等於某值的元素, 如果找不到會回傳 end() 迭代器
    std::cout << "=== find ===" << std::endl;
    auto it = std::find(vec.begin(), vec.end(), 30);
    if (it != vec.end()) {
        std::cout << "找到 30，位置: " << (it - vec.begin()) << std::endl;
    }
    
    // find_if：找第一個滿足條件的元素, 例如找第一個大於 25 的元素
    std::cout << "\n=== find_if ===" << std::endl;
    auto it2 = std::find_if(vec.begin(), vec.end(), 
        [](int n) { return n > 25; });
    if (it2 != vec.end()) {
        std::cout << "第一個大於 25 的元素: " << *it2 << std::endl;
    }
    
    // find_if_not：找第一個不滿足條件的元素, 例如找第一個不小於 30 的元素
    std::cout << "\n=== find_if_not ===" << std::endl;
    auto it3 = std::find_if_not(vec.begin(), vec.end(),
        [](int n) { return n < 30; });
    if (it3 != vec.end()) {
        std::cout << "第一個不小於 30 的元素: " << *it3 << std::endl;
    }
    
    // adjacent_find：找連續相同的元素, 例如找第一對相鄰相同的元素
    std::cout << "\n=== adjacent_find ===" << std::endl;
    std::vector<int> v2 = {1, 2, 3, 3, 4, 5, 5, 5};
    auto it4 = std::adjacent_find(v2.begin(), v2.end());
    if (it4 != v2.end()) {
        std::cout << "第一對相鄰相同元素: " << *it4 << std::endl;
    }
    
    return 0;
}
