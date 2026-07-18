#include <iostream>
using namespace std;

class Rectangle {
public:
    double width = 0;
    double height = 0;

    // 類內定義（隱式 inline）
    double area() {
        return width * height;
    }

    // 類內宣告，類外定義
    double perimeter();
    void scale(double factor);
    void print();
};

// 類外定義
double Rectangle::perimeter() {
    return 2 * (width + height);
}

void Rectangle::scale(double factor) {
    width *= factor;
    height *= factor;
}

void Rectangle::print() {
    cout << "Rectangle(" << width << " x " << height << ")" << endl;
    cout << "  面積: " << area() << endl;            // 成員函數調用另一個成員函數
    cout << "  周長: " << perimeter() << endl;       // 同上
}

int main() {
    Rectangle r;
    r.width = 5.0;
    r.height = 3.0;

    r.print();

    r.scale(2.0);
    cout << "\n放大 2 倍後:" << endl;
    r.print();

    return 0;
}
