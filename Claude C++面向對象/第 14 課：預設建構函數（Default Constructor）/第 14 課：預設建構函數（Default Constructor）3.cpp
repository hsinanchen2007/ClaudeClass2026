#include <iostream>
using namespace std;

class Point {
public:
    int x, y;
    
    // 手動加回預設建構函數
    Point() {
        x = 0;
        y = 0;
        cout << "預設建構: Point(0, 0)" << endl;
    }
    
    Point(int px, int py) {
        x = px;
        y = py;
        cout << "帶參建構: Point(" << x << ", " << y << ")" << endl;
    }
};

int main() {
    Point p1;          // OK：調用預設建構函數
    Point p2(3, 4);    // OK：調用帶參建構函數
    return 0;
}
