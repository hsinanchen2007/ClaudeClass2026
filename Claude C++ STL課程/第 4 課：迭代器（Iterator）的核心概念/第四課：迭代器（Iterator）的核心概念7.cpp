#include <iostream>
#include <vector>
#include <list>
#include <algorithm>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    std::list<int> lst = {5, 2, 8, 1, 9};
    
    // 同一個 find 演算法，用在不同容器
    std::cout << "=== std::find ===" << std::endl;
    
    auto vec_it = std::find(vec.begin(), vec.end(), 8);
    if (vec_it != vec.end()) {
        std::cout << "在 vector 中找到 8" << std::endl;
    }
    
    auto lst_it = std::find(lst.begin(), lst.end(), 8);
    if (lst_it != lst.end()) {
        std::cout << "在 list 中找到 8" << std::endl;
    }
    
    // 同一個 count 演算法
    std::cout << "\n=== std::count ===" << std::endl;
    
    std::vector<int> data = {1, 2, 2, 3, 2, 4, 2};
    int count = std::count(data.begin(), data.end(), 2);
    std::cout << "2 出現了 " << count << " 次" << std::endl;
    
    // 在子區間上操作
    std::cout << "\n=== 子區間操作 ===" << std::endl;
    
    std::vector<int> nums = {10, 20, 30, 40, 50, 60, 70};
    
    // 只在 [begin+1, begin+5) 範圍內查找，即 {20, 30, 40, 50}
    auto sub_it = std::find(nums.begin() + 1, nums.begin() + 5, 40);
    if (sub_it != nums.begin() + 5) {
        std::cout << "在子區間中找到 40" << std::endl;
    }
    
    return 0;
}
