#include <iostream>
#include <vector>
#include <iterator>  // std::distance

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it1 = v.begin();
    auto it2 = v.begin() + 3;

    // 方法一：直接相減（僅限隨機存取迭代器）
    std::cout << "it2 - it1 = " << (it2 - it1) << std::endl;

    // 方法二：std::distance()（適用於所有迭代器類型）
    std::cout << "distance = " << std::distance(it1, it2) << std::endl;

    return 0;
}
