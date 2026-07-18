#include <iostream>
#include <vector>
#include <iterator>  // std::advance

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin();
    std::cout << "*it = " << *it << std::endl;  // 10

    // 前進 3 步
    std::advance(it, 3);
    std::cout << "advance(it, 3): " << *it << std::endl;  // 40

    // 後退 1 步
    std::advance(it, -1);
    std::cout << "advance(it, -1): " << *it << std::endl;  // 30

    return 0;
}
