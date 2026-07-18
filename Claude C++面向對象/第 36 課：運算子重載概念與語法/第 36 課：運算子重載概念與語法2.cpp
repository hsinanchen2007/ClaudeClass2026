#include <iostream>

class Vector2D {
public:
    double x, y;

    Vector2D(double x, double y) : x(x), y(y) {}

    // 成員函數重載 +
    // this 是左運算元 (a)，rhs 是右運算元 (b)
    Vector2D operator+(const Vector2D& rhs) const {
        return Vector2D(x + rhs.x, y + rhs.y);
    }

    void print() const {
        std::cout << "(" << x << ", " << y << ")" << std::endl;
    }
};

int main() {
    Vector2D a(1.0, 2.0);
    Vector2D b(3.0, 4.0);

    Vector2D c = a + b;   // 等同於 a.operator+(b)
    c.print();             // 輸出：(4, 6)

    // 直接用函數調用語法也行（但沒人這樣寫）
    Vector2D d = a.operator+(b);
    d.print();             // 輸出：(4, 6)

    return 0;
}
