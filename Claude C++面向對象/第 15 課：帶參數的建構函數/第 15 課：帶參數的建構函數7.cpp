#include <iostream>
#include <string>
using namespace std;

class Point {
private:
    double x, y;

public:
    Point(double x, double y) : x(x), y(y) {
        cout << "  建構 Point(" << x << ", " << y << ")" << endl;
    }
    
    void print() const {
        cout << "  (" << x << ", " << y << ")" << endl;
    }
};

int main() {
    // 語法 1：直接初始化（括號）
    // 語法 2：統一初始化（大括號，C++11）
    // 語法 3：拷貝初始化（等號）
    // 語法 4：等號 + 大括號（C++11）
    // 語法 5：動態分配
    cout << "=== 語法 1：直接初始化（括號）===" << endl;
    Point p1(3.0, 4.0);
    p1.print();
    
    cout << "\n=== 語法 2：統一初始化（大括號，C++11）===" << endl;
    Point p2{5.0, 6.0};
    p2.print();
    
    cout << "\n=== 語法 3：拷貝初始化（等號）===" << endl;
    Point p3 = Point(7.0, 8.0);
    p3.print();
    
    cout << "\n=== 語法 4：等號 + 大括號（C++11）===" << endl;
    Point p4 = {9.0, 10.0};
    p4.print();
    
    cout << "\n=== 語法 5：動態分配 ===" << endl;
    Point* p5 = new Point(1.0, 2.0);
    p5->print();
    delete p5;
    
    return 0;
}
