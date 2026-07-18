#include <iostream>
using namespace std;

class Color {
private:
    int r, g, b;

public:
    // 所有參數都有預設值 → 這也是預設建構函數！
    Color(int red = 0, int green = 0, int blue = 0) {
        r = (red >= 0 && red <= 255) ? red : 0;
        g = (green >= 0 && green <= 255) ? green : 0;
        b = (blue >= 0 && blue <= 255) ? blue : 0;
    }

    void print() const {
        cout << "  RGB(" << r << ", " << g << ", " << b << ")" << endl;
    }
};

int main() {
    Color c1;               // 全部使用預設值 → (0, 0, 0)
    Color c2(255);           // green 和 blue 用預設值 → (255, 0, 0)
    Color c3(0, 128);        // blue 用預設值 → (0, 128, 0)
    Color c4(100, 200, 50);  // 全部指定
    
    cout << "c1: "; c1.print();   // 黑色
    cout << "c2: "; c2.print();   // 紅色
    cout << "c3: "; c3.print();   // 綠色
    cout << "c4: "; c4.print();   // 自定義
    
    return 0;
}
