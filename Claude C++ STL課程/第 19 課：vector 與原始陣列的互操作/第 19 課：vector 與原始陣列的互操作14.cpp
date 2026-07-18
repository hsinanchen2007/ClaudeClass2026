#include <iostream>
#include <vector>

int main() {
    int* dangerous_ptr = nullptr;

    {
        std::vector<int> v = {10, 20, 30};
        dangerous_ptr = v.data();
        std::cout << "在作用域內：" << dangerous_ptr[0] << std::endl;  // 10，OK
    }
    // v 已經被銷毀，底層記憶體已釋放

    // dangerous_ptr 現在是懸空指標（dangling pointer）
    // std::cout << dangerous_ptr[0] << std::endl;  // 未定義行為！

    return 0;
}
