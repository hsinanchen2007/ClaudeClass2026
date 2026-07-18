#include <iostream>

int main() {
    bool flag = true;  // bool 是內建類型，不需要標頭檔
    
    if (flag) {
        std::cout << "flag is true" << std::endl;
    }
    
    // 可以直接輸出布林值
    std::cout << "flag = " << flag << std::endl;              // 輸出 1
    std::cout << "flag = " << std::boolalpha << flag << std::endl;  // 輸出 true
    
    return 0;
}
