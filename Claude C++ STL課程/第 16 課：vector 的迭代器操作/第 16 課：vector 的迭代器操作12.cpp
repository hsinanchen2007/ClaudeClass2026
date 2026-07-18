#include <iostream>
#include <vector>
#include <iterator>  // std::next, std::prev

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin();

    // std::next() 回傳新迭代器，原迭代器不變
    auto it2 = std::next(it, 3);
    std::cout << "*it = " << *it << std::endl;    // 10（不變）
    std::cout << "*it2 = " << *it2 << std::endl;  // 40

    // std::prev() 回傳前一個位置
    auto it3 = std::prev(v.end());
    std::cout << "*prev(end) = " << *it3 << std::endl;  // 50

    // 也可以指定步數
    auto it4 = std::prev(v.end(), 2);
    std::cout << "*prev(end, 2) = " << *it4 << std::endl;  // 40

    return 0;
}
