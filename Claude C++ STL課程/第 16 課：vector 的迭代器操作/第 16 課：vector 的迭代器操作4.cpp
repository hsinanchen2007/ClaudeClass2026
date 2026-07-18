#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // cbegin() / cend() 回傳 const_iterator
    // 只能讀取，不能修改元素
    for (std::vector<int>::const_iterator it = v.cbegin(); it != v.cend(); ++it) {
        std::cout << *it << " ";
        // *it = 100;  // 編譯錯誤！不可透過 const_iterator 修改元素
    }
    std::cout << std::endl;

    return 0;
}
