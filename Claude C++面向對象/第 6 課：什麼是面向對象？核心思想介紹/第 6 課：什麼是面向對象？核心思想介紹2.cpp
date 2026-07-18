#include <iostream>
#include <string>
using namespace std;

class Shape {
public:
    virtual double area() {  // virtual = 虛函數，允許派生類覆寫
        return 0;
    }
    virtual ~Shape() {}  // 虛解構函數（後續課程會詳解）
};

class Circle : public Shape {
private:
    double radius;
public:
    Circle(double r) : radius(r) {}

    double area() override {  // 覆寫基類的 area()
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
private:
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}

    double area() override {
        return width * height;
    }
};

// 這個函數接受任何 Shape！
void printArea(Shape* s) {
    cout << "面積 = " << s->area() << endl;
}

int main() {
    Circle c(5.0);
    Rectangle r(4.0, 6.0);

    printArea(&c);  // 呼叫 Circle::area()
    printArea(&r);  // 呼叫 Rectangle::area()

    return 0;
}
