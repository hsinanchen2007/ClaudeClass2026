#include <iostream>
#include <string>

// 泛型版本：一次搞定所有型別
template <typename T>
void my_swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

int main() {
    // 交換 int
    int x = 10, y = 20;
    my_swap(x, y);  // 編譯器自動推導 T = int
    std::cout << "x = " << x << ", y = " << y << std::endl;
    
    // 交換 double
    double a = 3.14, b = 2.72;
    my_swap(a, b);  // 編譯器自動推導 T = double
    std::cout << "a = " << a << ", b = " << b << std::endl;
    
    // 交換 string
    std::string s1 = "Hello", s2 = "World";
    my_swap(s1, s2);  // 編譯器自動推導 T = std::string
    std::cout << "s1 = " << s1 << ", s2 = " << s2 << std::endl;
    
    return 0;
}
