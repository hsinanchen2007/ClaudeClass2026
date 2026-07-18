#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

    std::cout << "刪除前：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // 正確寫法
    for (auto it = v.begin(); it != v.end(); /* 不在這裡 ++it */) {
        if (*it % 2 == 0) {
            it = v.erase(it);  // erase 回傳下一個有效迭代器
        } else {
            ++it;              // 只在不刪除時才前進
        }
    }

    std::cout << "刪除後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
