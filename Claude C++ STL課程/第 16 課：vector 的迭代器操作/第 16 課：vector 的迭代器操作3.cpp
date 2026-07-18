#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // rbegin() 指向最後一個元素
    // rend()   指向第一個元素的「前一個位置」
    // 型別是 reverse_iterator
    std::vector<int>::reverse_iterator rit;

    std::cout << "反向遍歷：";
    for (rit = v.rbegin(); rit != v.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    return 0;
}
