#include <iostream>
#include <vector>
#include <memory>

// 基底類別
class Shape {
public:
    virtual double area() const = 0;
    virtual ~Shape() = default;
};

// 衍生類別
class Circle : public Shape {
    double radius;
public:
    Circle(double r) : radius(r) {}
    double area() const override {
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    double area() const override {
        return width * height;
    }
};

int main() {
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(5.0));
    shapes.push_back(std::make_unique<Rectangle>(4.0, 3.0));
    
    for (const auto& shape : shapes) {
        std::cout << "Area: " << shape->area() << std::endl;  // 執行期決定
    }
    
    return 0;
}
