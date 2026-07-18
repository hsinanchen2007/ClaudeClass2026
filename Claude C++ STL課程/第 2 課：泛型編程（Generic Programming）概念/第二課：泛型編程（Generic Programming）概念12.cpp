#include <iostream>

class Circle {
    double radius;
public:
    Circle(double r) : radius(r) {}
    double area() const {
        return 3.14159 * radius * radius;
    }
};

class Rectangle {
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    double area() const {
        return width * height;
    }
};

// 泛型函數：不需要繼承關係
template <typename Shape>
void print_area(const Shape& shape) {
    std::cout << "Area: " << shape.area() << std::endl;  // 編譯期決定
}

int main() {
    Circle c(5.0);
    Rectangle r(4.0, 3.0);
    
    print_area(c);  // 編譯期展開為 print_area(const Circle&)
    print_area(r);  // 編譯期展開為 print_area(const Rectangle&)
    
    return 0;
}
