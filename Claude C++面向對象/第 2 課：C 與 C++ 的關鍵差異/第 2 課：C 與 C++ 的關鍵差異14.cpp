#include <iostream>

void swap(int& a, int& b) {  // 使用引用
    int temp = a;
    a = b;
    b = temp;
}

int main() {
    int x = 10, y = 20;
    swap(x, y);  // 不需要 &，語法更簡潔
    std::cout << "x = " << x << ", y = " << y << std::endl;
    return 0;
}
