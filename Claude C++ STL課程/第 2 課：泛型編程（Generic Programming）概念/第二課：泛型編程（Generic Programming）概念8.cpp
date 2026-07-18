#include <iostream>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;  // 使用了 > 運算子
}

// 自訂類別，沒有定義 > 運算子
class Point {
public:
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
};

int main() {
    Point p1(1, 2), p2(3, 4);
    // Point max_point = find_max(p1, p2);  // 編譯錯誤！
    // 錯誤訊息：no match for 'operator>'
    
    return 0;
}
