#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    int a = 10;
    int b = 20;
    int c = 30;
    
    // 預設以值捕獲，但 b 以參考捕獲
    auto lambda1 = [=, &b]() {
        // a 是副本，b 是參考，c 是副本
        b = a + c;  // 可以修改 b
        // a = 100;  // 錯誤！a 是值捕獲，不能修改（除非 mutable）
    };
    
    lambda1();
    std::cout << "a = " << a << ", b = " << b << ", c = " << c << std::endl;
    
    // 預設以參考捕獲，但 a 以值捕獲
    auto lambda2 = [&, a]() {
        // a 是副本，b 和 c 是參考
        b = 100;
        c = 200;
        // a 不能修改（值捕獲）
    };
    
    lambda2();
    std::cout << "a = " << a << ", b = " << b << ", c = " << c << std::endl;
    
    return 0;
}
