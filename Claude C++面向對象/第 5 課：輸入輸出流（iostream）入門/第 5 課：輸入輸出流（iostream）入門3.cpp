#include <iostream>

int main() {
    int i = 42;
    double d = 3.14159;
    char c = 'A';
    const char* s = "Hello";
    bool b = true;
    
    // 不需要格式符，自動識別類型
    std::cout << "int: " << i << std::endl;
    std::cout << "double: " << d << std::endl;
    std::cout << "char: " << c << std::endl;
    std::cout << "string: " << s << std::endl;
    std::cout << "bool: " << b << std::endl;  // 輸出 1
    
    return 0;
}
