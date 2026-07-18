#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 排序
    std::sort(vec.begin(), vec.end());
    std::cout << "排序後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 查找
    auto it = std::find(vec.begin(), vec.end(), 7);
    if (it != vec.end()) {
        std::cout << "找到 7，位置: " << (it - vec.begin()) << std::endl;
    }
    
    // 計數
    std::vector<int> data = {1, 2, 2, 3, 2, 4, 2, 5};
    int count = std::count(data.begin(), data.end(), 2);
    std::cout << "2 出現次數: " << count << std::endl;
    
    // 累加
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "總和: " << sum << std::endl;
    
    // 最大最小
    auto minmax = std::minmax_element(vec.begin(), vec.end());
    std::cout << "最小值: " << *minmax.first << std::endl;
    std::cout << "最大值: " << *minmax.second << std::endl;
    
    return 0;
}
