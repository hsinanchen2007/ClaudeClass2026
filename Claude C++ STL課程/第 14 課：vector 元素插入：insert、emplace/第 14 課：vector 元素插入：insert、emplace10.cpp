#include <vector>
#include <iostream>
#include <algorithm>

void sorted_insert(std::vector<int>& v, int value) {
    // 找到第一個大於等於 value 的位置
    auto pos = std::lower_bound(v.begin(), v.end(), value);
    v.insert(pos, value);
}

int main() {
    std::vector<int> v = {1, 3, 5, 7, 9};
    
    sorted_insert(v, 4);
    sorted_insert(v, 0);
    sorted_insert(v, 10);
    
    for (int x : v) {
        std::cout << x << " ";  // 0 1 3 4 5 7 9 10
    }
    std::cout << std::endl;
    
    return 0;
}
