#include <iostream>
using namespace std;

class Point {
public:
    int x, y;
    
    // 告訴編譯器：請幫我生成預設建構函數
    // 這樣做的效果和完全不寫建構函數是一樣的
    // 但是這樣寫可以讓我們在定義了其他建構函數後，仍然保留預設建構函數的功能
    // 如果我們定義了其他建構函數，但又想保留預設建構函數，就可以使用 = default
    Point() = default;
    
    Point(int px, int py) {
        x = px;
        y = py;
        cout << "帶參建構: Point(" << x << ", " << y << ")" << endl;
    }
};

int main() {
    Point p1;          // OK：使用編譯器生成的預設建構函數
    Point p2(3, 4);    // OK
    
    // 注意：p1.x 和 p1.y 仍然是未初始化的垃圾值！
    // 因為 = default 生成的建構函數和編譯器原本自動生成的行為一樣
    cout << "p1.x = " << p1.x << ", p1.y = " << p1.y << endl;  // 垃圾值
    
    cout << "p2.x = " << p2.x << ", p2.y = " << p2.y << endl;  // 正確值

    return 0;
}
