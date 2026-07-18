#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>  // 內建函數物件

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    
    // 預設排序（升序）
    std::sort(vec.begin(), vec.end());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 使用 greater<int> 降序排序
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 其他內建函數物件
    std::cout << "\n內建函數物件示範：" << std::endl;
    std::cout << "plus<int>()(3, 4) = " << std::plus<int>()(3, 4) << std::endl;
    std::cout << "minus<int>()(10, 3) = " << std::minus<int>()(10, 3) << std::endl;
    std::cout << "multiplies<int>()(5, 6) = " << std::multiplies<int>()(5, 6) << std::endl;
    std::cout << "logical_and<bool>()(true, false) = " 
              << std::logical_and<bool>()(true, false) << std::endl;
    
    return 0;
}
