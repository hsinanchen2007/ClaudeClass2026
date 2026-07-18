#include <iostream>

struct Point {
    int x;
    int y;
    
    // C++ 的 struct 可以有成員函數！
    void print() {
        std::cout << "(" << x << ", " << y << ")" << std::endl;
    }
};

int main() {
    // C++ 不需要 struct 關鍵字
    Point p1;
    p1.x = 10;
    p1.y = 20;
    p1.print();  // 調用成員函數
    
    return 0;
}
