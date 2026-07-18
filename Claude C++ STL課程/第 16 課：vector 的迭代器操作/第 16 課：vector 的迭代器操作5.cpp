#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 反向 + 唯讀
    std::cout << "常數反向遍歷：";
    for (auto rit = v.crbegin(); rit != v.crend(); ++rit) {
        std::cout << *rit << " ";
        // *rit = 100;  // 編譯錯誤！
    }
    std::cout << std::endl;

    return 0;
}
