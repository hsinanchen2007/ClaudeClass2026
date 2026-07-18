#include <iostream>
#include <cmath>
using namespace std;

struct Point {
    double x = 0;
    double y = 0;

    // 便利函數 —— 不改變「純資料集合」的本質
    // 注意：這裡的 distanceTo 和 print 是純粹的「功能函數」，不會修改 Point 的資料成員
    // 這些函數只是提供了對 Point 資料的操作，但不會改變 Point 的內部狀態，因此仍然符合 struct 的設計理念
    // 這些函數不會改變 Point 的資料成員，因此仍然符合 struct 的設計理念
    double distanceTo(const Point& other) {
        double dx = x - other.x;
        double dy = y - other.y;
        return sqrt(dx * dx + dy * dy);
    }

    void print() {
        cout << "(" << x << ", " << y << ")" << endl;
    }
};

struct Rectangle {
    double width = 0;
    double height = 0;

    double area() { return width * height; }
    double perimeter() { return 2 * (width + height); }
};

int main() {
    Point a, b;
    a.x = 0; a.y = 0;
    b.x = 3; b.y = 4;

    a.print();
    b.print();
    cout << "距離: " << a.distanceTo(b) << endl;

    Rectangle r;
    r.width = 5;
    r.height = 3;
    cout << "面積: " << r.area() << ", 周長: " << r.perimeter() << endl;

    return 0;
}
