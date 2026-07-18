#include <iostream>
#include <string>
using namespace std;

class Rectangle {
private:
    double width;
    double height;
    string color;

public:
    // 建構函數 1：無參數 → 預設的矩形
    Rectangle() {
        width = 1.0;
        height = 1.0;
        color = "白色";
        cout << "[建構1] 預設矩形" << endl;
    }

    // 建構函數 2：正方形（只給一個邊長）
    Rectangle(double side) {
        width = side;
        height = side;
        color = "白色";
        cout << "[建構2] 正方形, 邊長 = " << side << endl;
    }

    // 建構函數 3：指定寬高
    Rectangle(double w, double h) {
        width = w;
        height = h;
        color = "白色";
        cout << "[建構3] 矩形 " << w << " x " << h << endl;
    }

    // 建構函數 4：指定寬高和顏色
    Rectangle(double w, double h, string c) {
        width = w;
        height = h;
        color = c;
        cout << "[建構4] " << c << "矩形 " << w << " x " << h << endl;
    }

    void print() const {
        cout << "  " << color << " 矩形: " 
             << width << " x " << height 
             << ", 面積 = " << width * height << endl;
    }
};

int main() {
    Rectangle r1;                         // 調用建構函數 1
    r1.print();
    
    Rectangle r2(5.0);                    // 調用建構函數 2
    r2.print();
    
    Rectangle r3(4.0, 6.0);              // 調用建構函數 3
    r3.print();
    
    Rectangle r4(3.0, 7.0, "紅色");       // 調用建構函數 4
    r4.print();
    
    return 0;
}
