#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3};
    
    auto it = v.insert(v.begin() + 1, 100);
    
    std::cout << "插入的值: " << *it << std::endl;  // 100
    std::cout << "插入位置的索引: " << (it - v.begin()) << std::endl;  // 1
    
    // 可以利用回傳值繼續操作
    v.insert(it, 99);  // 在 100 之前再插入 99
    
    for (int x : v) {
        std::cout << x << " ";  // 1 99 100 2 3
    }
    std::cout << std::endl;
    
    return 0;
}
