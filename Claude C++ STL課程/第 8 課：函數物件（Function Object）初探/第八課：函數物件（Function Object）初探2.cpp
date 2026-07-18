#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3};
    
    // sort 預設是升序
    std::sort(vec.begin(), vec.end());
    
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 如果想要降序呢？
    // sort 需要一個「告訴它如何比較」的東西
    
    return 0;
}
