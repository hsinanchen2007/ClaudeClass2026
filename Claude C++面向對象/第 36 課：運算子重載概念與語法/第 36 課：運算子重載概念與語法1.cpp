#include <iostream>

class Vector2D {
public:
    double x, y;
    Vector2D(double x, double y) : x(x), y(y) {}
};

int main() {
    Vector2D a(1.0, 2.0);
    Vector2D b(3.0, 4.0);

    // Vector2D c = a + b;  // ❌ 編譯錯誤！編譯器不知道如何 "+" 兩個 Vector2D
    // if (a == b) {}       // ❌ 編譯錯誤！編譯器不知道如何比較

    return 0;
}
