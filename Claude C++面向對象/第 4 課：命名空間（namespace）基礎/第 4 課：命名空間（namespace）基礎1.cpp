#include <iostream>

// 定義命名空間 math_utils
namespace math_utils {
    const double PI = 3.14159265358979;
    
    int add(int a, int b) {
        return a + b;
    }
    
    int square(int x) {
        return x * x;
    }
}

int main() {
    // 使用命名空間中的成員，需要加上 命名空間:: 前綴
    std::cout << "PI = " << math_utils::PI << std::endl;
    std::cout << "add(3, 5) = " << math_utils::add(3, 5) << std::endl;
    std::cout << "square(4) = " << math_utils::square(4) << std::endl;
    
    return 0;
}
