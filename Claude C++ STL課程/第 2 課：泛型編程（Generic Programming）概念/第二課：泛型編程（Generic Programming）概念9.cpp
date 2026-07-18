#include <iostream>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

class Point {
public:
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
    
    // 定義 > 運算子，以距離原點的平方和比較
    bool operator>(const Point& other) const {
        return (x*x + y*y) > (other.x*other.x + other.y*other.y);
    }
};

// 為了輸出
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p.x << ", " << p.y << ")";
}

int main() {
    Point p1(1, 2), p2(3, 4);
    Point max_point = find_max(p1, p2);  // 現在可以了！
    std::cout << "max point: " << max_point << std::endl;
    
    return 0;
}
