#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 用指標的思維來理解迭代器
    // p 指向第一個元素
    std::vector<int>::iterator it = v.begin();

    std::cout << *it << std::endl;  // 10（解參考，就像指標一樣）
    ++it;
    std::cout << *it << std::endl;  // 20（往前移一個位置）

    return 0;
}
