#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 範圍 for 迴圈
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    // 上面等同於：
    for (auto it = v.begin(); it != v.end(); ++it) {
        int x = *it;
        std::cout << x << " ";
    }
    std::cout << std::endl;

    return 0;
}
