#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cctype>

int main() {
    // 一元 transform：對每個元素套用轉換, 產生一個新的範圍
    std::cout << "=== 一元 transform ===" << std::endl;
    std::vector<int> src = {1, 2, 3, 4, 5};
    std::vector<int> squares(src.size());
    
    std::transform(src.begin(), src.end(), squares.begin(),
        [](int n) { return n * n; });
    
    std::cout << "平方: ";
    for (int n : squares) std::cout << n << " ";
    std::cout << std::endl;
    
    // 二元 transform：結合兩個範圍, 產生一個新的範圍
    std::cout << "\n=== 二元 transform ===" << std::endl;
    std::vector<int> v1 = {1, 2, 3, 4, 5};
    std::vector<int> v2 = {10, 20, 30, 40, 50};
    std::vector<int> sums(v1.size());
    
    std::transform(v1.begin(), v1.end(), v2.begin(), sums.begin(),
        [](int a, int b) { return a + b; });
    
    std::cout << "v1 + v2: ";
    for (int n : sums) std::cout << n << " ";
    std::cout << std::endl;
    
    // 實用案例：字串轉大寫, 使用 lambda 表達式將每個字元轉換為大寫
    std::cout << "\n=== 字串轉大寫 ===" << std::endl;
    std::string str = "Hello, World!";
    std::transform(str.begin(), str.end(), str.begin(),
        [](char c) { return std::toupper(c); });
    std::cout << "轉換後: " << str << std::endl;
    
    return 0;
}
