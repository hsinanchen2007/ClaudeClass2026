#include <iostream>
#include <vector>

int main() {
    std::vector<int> vi = {10, 20, 30};
    int* pi = &vi[0];  // 完全合法，取得元素的位址
    std::cout << "*pi = " << *pi << std::endl;

    std::vector<bool> vb = {true, false, true};
    // bool* pb = &vb[0];  // 編譯錯誤！
    // vb[0] 回傳的是代理物件，不是 bool
    // 你不能對代理物件取位址來得到 bool*

    return 0;
}
