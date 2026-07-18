#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // 不用自己寫動態陣列 - 用 vector
    std::vector<int> numbers = {5, 2, 8, 1, 9};
    
    // 不用自己寫排序 - 用 sort
    std::sort(numbers.begin(), numbers.end());
    
    // 不用自己寫搜尋 - 用 find
    auto it = std::find(numbers.begin(), numbers.end(), 8);
    
    if (it != numbers.end()) {
        std::cout << "找到了: " << *it << std::endl;
    }
    
    return 0;
}
