#include <iostream>

// 交換兩個 int
void swap_int(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

// 交換兩個 double
void swap_double(double& a, double& b) {
    double temp = a;
    a = b;
    b = temp;
}

// 交換兩個 string... 還要再寫一次？

int main() {
    int x = 10, y = 20;
    swap_int(x, y);
    std::cout << "x = " << x << ", y = " << y << std::endl;
    
    double a = 3.14, b = 2.72;
    swap_double(a, b);
    std::cout << "a = " << a << ", b = " << b << std::endl;
    
    return 0;
}
