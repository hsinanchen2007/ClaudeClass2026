#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    
    auto it = v.begin() + 2;  // 指向 3
    std::cout << "插入前 *it = " << *it << std::endl;  // 3
    
    // 在 it 之前插入元素
    auto new_it = v.insert(it, 100);
    
    // it 現在已失效！不應該再使用
    // std::cout << *it << std::endl;  // 未定義行為
    
    // 應該使用 insert 回傳的新迭代器
    std::cout << "new_it 指向: " << *new_it << std::endl;  // 100
    std::cout << "new_it + 1 指向: " << *(new_it + 1) << std::endl;  // 3
    
    return 0;
}
