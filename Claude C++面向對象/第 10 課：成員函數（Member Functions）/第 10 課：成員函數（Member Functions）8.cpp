#include <iostream>
using namespace std;

class Circle {
public:
    double radius = 0;

    // 成員函數：自動知道 radius
    double area() {
        return 3.14159 * radius * radius;
    }
};

// 普通函數：必須手動傳入 radius
double circleArea(double radius) {
    return 3.14159 * radius * radius;
}

int main() {
    Circle c;
    c.radius = 5.0;

    // 成員函數風格
    cout << "成員函數: " << c.area() << endl;

    // 普通函數風格
    cout << "普通函數: " << circleArea(c.radius) << endl;

    return 0;
}
