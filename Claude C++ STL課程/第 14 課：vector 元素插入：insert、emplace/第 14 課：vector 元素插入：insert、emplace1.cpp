#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    
    // insert 需要一個迭代器指定插入位置
    // 新元素會插入在該迭代器「之前」
    
    // 在開頭插入
    v.insert(v.begin(), 0);
    // v: {0, 1, 2, 3, 4, 5}
    
    // 在第三個位置插入（索引 2）
    v.insert(v.begin() + 2, 100);
    // v: {0, 1, 100, 2, 3, 4, 5}
    
    // 在結尾插入（等同於 push_back）
    v.insert(v.end(), 6);
    // v: {0, 1, 100, 2, 3, 4, 5, 6}
    
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
