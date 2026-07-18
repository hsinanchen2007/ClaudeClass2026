#include <iostream>

// C++ 允許同名函數，只要參數不同
int add(int a, int b) {
    return a + b;
}

double add(double a, double b) {
    return a + b;
}

int add(int a, int b, int c) {
    return a + b + c;
}

int main() {
    std::cout << add(3, 5) << std::endl;        // 調用 int 版本
    std::cout << add(3.14, 2.86) << std::endl;  // 調用 double 版本
    std::cout << add(1, 2, 3) << std::endl;     // 調用三參數版本
    
    return 0;
}
