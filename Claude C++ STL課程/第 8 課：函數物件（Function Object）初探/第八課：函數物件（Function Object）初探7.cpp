#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

int main() {
    std::cout << "=== 比較類函數物件 ===" << std::endl;
    
    // equal_to<T>：相等
    std::equal_to<int> eq;
    std::cout << "equal_to: 5 == 5? " << (eq(5, 5) ? "true" : "false") << std::endl;
    
    // not_equal_to<T>：不相等
    std::not_equal_to<int> neq;
    std::cout << "not_equal_to: 5 != 3? " << (neq(5, 3) ? "true" : "false") << std::endl;
    
    // greater<T>：大於
    std::greater<int> gt;
    std::cout << "greater: 5 > 3? " << (gt(5, 3) ? "true" : "false") << std::endl;
    
    // less<T>：小於
    std::less<int> lt;
    std::cout << "less: 3 < 5? " << (lt(3, 5) ? "true" : "false") << std::endl;
    
    // greater_equal<T>：大於等於
    std::greater_equal<int> gte;
    std::cout << "greater_equal: 5 >= 5? " << (gte(5, 5) ? "true" : "false") << std::endl;
    
    // less_equal<T>：小於等於
    std::less_equal<int> lte;
    std::cout << "less_equal: 3 <= 5? " << (lte(3, 5) ? "true" : "false") << std::endl;
    
    // 實際應用：降序排序
    std::cout << "\n=== 實際應用：排序 ===" << std::endl;
    std::vector<int> vec = {5, 2, 8, 1, 9, 3};
    
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    std::sort(vec.begin(), vec.end(), std::less<int>());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
