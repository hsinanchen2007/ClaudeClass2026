#include <iostream>

int main() {
    int a, b, c;
    
    std::cout << "請輸入三個整數（用空格分隔）: ";
    std::cin >> a >> b >> c;
    
    std::cout << "你輸入的是: " << a << ", " << b << ", " << c << std::endl;
    std::cout << "總和: " << (a + b + c) << std::endl;
    
    return 0;
}
