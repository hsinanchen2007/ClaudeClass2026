#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // begin() 指向第一個元素
    // end()   指向最後一個元素的「下一個位置」（past-the-end）
    std::vector<int>::iterator first = v.begin();
    std::vector<int>::iterator last  = v.end();

    std::cout << "*first = " << *first << std::endl;
    // *last 是未定義行為！end() 不指向任何有效元素

    // 典型的遍歷方式
    for (std::vector<int>::iterator it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    return 0;
}
