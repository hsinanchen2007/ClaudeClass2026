#include <iostream>
using namespace std;

class Point {
public:
    int x, y;
    
    // 只定義了帶參數的建構函數
    Point(int px, int py) {
        x = px;
        y = py;
        cout << "Point(" << x << ", " << y << ") 建構完成" << endl;
    }
};

int main() {
    Point p1(3, 4);    // OK：匹配帶參建構函數
    
    // Point p2;        // 編譯錯誤！
    // error: no matching function for call to 'Point::Point()'
    
    // Point p3{};      // 編譯錯誤！同樣需要預設建構函數
    
    return 0;
}
