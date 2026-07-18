#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    
    // 錯誤示範：迭代器失效
    /*
    auto it = v.begin() + 2;
    v.insert(it, 10);
    v.insert(it, 20);  // it 已失效！未定義行為
    */
    
    // 正確做法 1：使用回傳值更新迭代器
    auto it = v.begin() + 2;
    it = v.insert(it, 10);
    it = v.insert(it, 20);  // 在 10 之前插入 20
    
    std::cout << "做法 1: ";
    for (int x : v) {
        std::cout << x << " ";  // 1 2 20 10 3 4 5
    }
    std::cout << std::endl;
    
    // 正確做法 2：使用索引
    v = {1, 2, 3, 4, 5};
    size_t index = 2;
    v.insert(v.begin() + index, 10);
    ++index;  // 更新索引
    v.insert(v.begin() + index, 20);
    
    std::cout << "做法 2: ";
    for (int x : v) {
        std::cout << x << " ";  // 1 2 10 20 3 4 5
    }
    std::cout << std::endl;
    
    // 正確做法 3：一次插入多個（最高效）
    v = {1, 2, 3, 4, 5};
    v.insert(v.begin() + 2, {10, 20, 30});
    
    std::cout << "做法 3: ";
    for (int x : v) {
        std::cout << x << " ";  // 1 2 10 20 30 3 4 5
    }
    std::cout << std::endl;
    
    return 0;
}
