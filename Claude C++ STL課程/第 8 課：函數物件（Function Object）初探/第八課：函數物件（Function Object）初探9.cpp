#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 最簡單的 Lambda
    std::cout << "=== 基本 Lambda ===" << std::endl;
    auto print = [](int n) { std::cout << n << " "; };
    
    std::for_each(vec.begin(), vec.end(), print);
    std::cout << std::endl;
    
    // 直接內聯使用
    std::cout << "\n=== 內聯 Lambda ===" << std::endl;
    std::for_each(vec.begin(), vec.end(), [](int n) {
        std::cout << n * n << " ";
    });
    std::cout << std::endl;
    
    // 帶回傳值的 Lambda
    std::cout << "\n=== 帶回傳值的 Lambda ===" << std::endl;
    auto square = [](int n) { return n * n; };
    std::cout << "5 的平方: " << square(5) << std::endl;
    
    // 用在 count_if
    std::cout << "\n=== 用在 count_if ===" << std::endl;
    int even_count = std::count_if(vec.begin(), vec.end(), 
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;
    
    return 0;
}
