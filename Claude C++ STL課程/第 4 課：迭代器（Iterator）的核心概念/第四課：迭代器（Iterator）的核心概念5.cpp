#include <iostream>
#include <vector>

void print_vector(const std::vector<int>& vec) {
    // const 容器只能用 const_iterator
    // 使用 cbegin() 和 cend() 明確取得 const_iterator
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        std::cout << *it << " ";
        // *it = 100;  // 編譯錯誤！不能透過 const_iterator 修改
    }
    std::cout << std::endl;
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    // 一般迭代器：可讀可寫
    std::cout << "=== 一般迭代器 ===" << std::endl;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        *it *= 2;  // 可以修改
    }
    print_vector(vec);
    
    // const_iterator：只能讀
    std::cout << "\n=== const_iterator ===" << std::endl;
    
    // 方法一：從 const 容器取得
    const std::vector<int>& const_ref = vec;
    for (auto it = const_ref.begin(); it != const_ref.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 方法二：使用 cbegin() / cend()
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
