#include <iostream>

int main() {
    int counter = 0;
    
    // 錯誤：預設情況下不能修改值捕獲的變數
    // auto increment = [counter]() { return ++counter; };  // 編譯錯誤
    
    // 使用 mutable
    auto increment = [counter]() mutable {
        return ++counter;  // 修改的是 Lambda 內部的副本
    };
    
    std::cout << "increment(): " << increment() << std::endl;  // 1
    std::cout << "increment(): " << increment() << std::endl;  // 2
    std::cout << "increment(): " << increment() << std::endl;  // 3
    std::cout << "外部 counter: " << counter << std::endl;     // 0（未被修改）
    
    return 0;
}
