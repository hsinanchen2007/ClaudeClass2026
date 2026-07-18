#include <iostream>
#include <vector>
#include <list>
#include <algorithm>

int main() {
    // 方法一：使用 erase 的回傳值（適用於所有容器）
    std::cout << "=== 方法一：erase 回傳值 ===" << std::endl;
    std::vector<int> vec1 = {1, 2, 3, 2, 4, 2, 5};
    
    for (auto it = vec1.begin(); it != vec1.end(); ) {
        if (*it == 2) {
            it = vec1.erase(it);
        } else {
            ++it;
        }
    }
    
    for (int n : vec1) std::cout << n << " ";
    std::cout << std::endl;
    
    // 方法二：使用 erase-remove 慣用法（適用於 vector）
    std::cout << "\n=== 方法二：erase-remove 慣用法 ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 3, 2, 4, 2, 5};
    
    vec2.erase(
        std::remove(vec2.begin(), vec2.end(), 2),
        vec2.end()
    );
    
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;
    
    // 方法三：對於 list，使用成員函數 remove（更高效）
    std::cout << "\n=== 方法三：list::remove ===" << std::endl;
    std::list<int> lst = {1, 2, 3, 2, 4, 2, 5};
    
    lst.remove(2);  // list 有自己的 remove 成員函數
    
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
