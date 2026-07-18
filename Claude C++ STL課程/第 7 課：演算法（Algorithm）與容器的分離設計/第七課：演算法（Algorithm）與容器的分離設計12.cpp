#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // partial_sort：只排序前 n 個, 這裡將 vec 中的元素部分排序，使得前 3 個元素是最小的，且已經排序好
    std::cout << "=== partial_sort ===" << std::endl;
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 只需要最小的 3 個，排好放在最前面, 其他元素不保證順序
    std::partial_sort(vec.begin(), vec.begin() + 3, vec.end());
    std::cout << "前 3 個最小（已排序）: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // nth_element：找第 n 個元素, 這裡將 vec2 中的元素重新排列，使得第 5 個元素（索引 4）是 vec2 中的第 5 小元素，且在它之前的元素都不大於它，在它之後的元素都不小於它
    std::cout << "\n=== nth_element ===" << std::endl;
    std::vector<int> vec2 = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 找中位數（第 4 個元素，索引 4）, 使得 vec2[4] 是 vec2 中的第 5 小元素，且在它之前的元素都不大於它，在它之後的元素都不小於它
    std::nth_element(vec2.begin(), vec2.begin() + 4, vec2.end());
    std::cout << "nth_element(4) 後: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "第 5 個元素（索引 4）: " << vec2[4] << std::endl;
    
    return 0;
}
