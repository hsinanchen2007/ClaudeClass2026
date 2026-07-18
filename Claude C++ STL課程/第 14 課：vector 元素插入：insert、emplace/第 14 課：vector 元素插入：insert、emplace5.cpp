#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3};
    
    // 使用初始化串列插入多個元素
    v.insert(v.begin() + 1, {10, 20, 30});
    
    for (int x : v) {
        std::cout << x << " ";  // 1 10 20 30 2 3
    }
    std::cout << std::endl;
    
    return 0;
}
