#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // count：計算某值出現次數, count_if：計算滿足條件的元素數
    std::cout << "=== count ===" << std::endl;
    std::vector<int> v2 = {1, 2, 2, 3, 2, 4, 2, 5};
    std::cout << "2 出現次數: " << std::count(v2.begin(), v2.end(), 2) << std::endl;
    
    // count_if：計算滿足條件的元素數, 使用 lambda 表達式作為條件
    std::cout << "\n=== count_if ===" << std::endl;
    int even_count = std::count_if(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;
    
    // all_of：是否全部滿足條件, any_of：是否有任一滿足條件, none_of：是否全部不滿足條件
    std::cout << "\n=== all_of ===" << std::endl;
    bool all_positive = std::all_of(vec.begin(), vec.end(),
        [](int n) { return n > 0; });
    std::cout << "全部是正數: " << (all_positive ? "是" : "否") << std::endl;
    
    // any_of：是否有任一滿足條件, 使用 lambda 表達式檢查是否有偶數
    std::cout << "\n=== any_of ===" << std::endl;
    bool has_even = std::any_of(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "有偶數: " << (has_even ? "是" : "否") << std::endl;
    
    // none_of：是否全部不滿足條件, 使用 lambda 表達式檢查是否有負數
    std::cout << "\n=== none_of ===" << std::endl;
    bool no_negative = std::none_of(vec.begin(), vec.end(),
        [](int n) { return n < 0; });
    std::cout << "沒有負數: " << (no_negative ? "是" : "否") << std::endl;
    
    return 0;
}
