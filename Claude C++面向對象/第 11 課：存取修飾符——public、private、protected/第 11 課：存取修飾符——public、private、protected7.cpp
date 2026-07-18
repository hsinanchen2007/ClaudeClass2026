#include <iostream>
using namespace std;

class Box {
private:
    double width = 0;
    double height = 0;

public:
    void setSize(double w, double h) {
        width = w;
        height = h;
    }

    double area() {
        return width * height;
    }

    // 比較兩個 Box —— 可以直接存取 other 的 private 成員！
    // 因為 isLargerThan 是 Box 類別的成員函數，所以它可以存取同類別的其他物件的 private 成員
    // 這是 C++ 的一個特點：同一類別的成員函數可以存取該類別所有物件的 private 成員
    // 注意：這裡的 other 是 Box 類別的另一個物件，所以它的 private 成員對 isLargerThan 來說是可見的
    // 這也是為什麼我們可以在 isLargerThan 中直接使用 other.width 和 other.height，即使它們是 private 的
    // 如果 isLargerThan 不是 Box 類別的成員函數，而是外部的普通函數，那麼它就無法存取 other 的 private 成員了
    // 這裡的設計允許我們在 Box 類別內部比較兩個 Box 的大小，而不需要提供 public 的 getter 函數來暴露 private 成員
    // 這種設計有助於保持類別的封裝性，同時又提供了必要的功能
    bool isLargerThan(const Box& other) {
        // other.width 和 other.height 是 private，但這裡能存取
        // 因為 isLargerThan 是 Box 類別的成員函數
        return (width * height) > (other.width * other.height);
    }

    void show() {
        cout << width << " x " << height << " = " << area() << endl;
    }
};

int main() {
    Box a, b;
    a.setSize(5, 4);
    b.setSize(3, 8);

    cout << "Box a: "; a.show();
    cout << "Box b: "; b.show();

    if (a.isLargerThan(b)) {
        cout << "a 比 b 大" << endl;
    } else {
        cout << "b 比 a 大（或一樣大）" << endl;
    }

    return 0;
}
