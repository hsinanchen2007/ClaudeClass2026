#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

int main() {
    // 同一個 sort，處理所有型別
    std::vector<int> ints = {3, 1, 4, 1, 5};
    std::vector<double> doubles = {3.14, 1.41, 2.72};
    std::vector<std::string> strings = {"cherry", "apple", "banana"};
    
    std::sort(ints.begin(), ints.end());
    std::sort(doubles.begin(), doubles.end());
    std::sort(strings.begin(), strings.end());
    
    std::cout << "排序後的整數: ";
    for (int n : ints) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "排序後的浮點數: ";
    for (double d : doubles) std::cout << d << " ";
    std::cout << std::endl;
    
    std::cout << "排序後的字串: ";
    for (const auto& s : strings) std::cout << s << " ";
    std::cout << std::endl;
    
    return 0;
}
