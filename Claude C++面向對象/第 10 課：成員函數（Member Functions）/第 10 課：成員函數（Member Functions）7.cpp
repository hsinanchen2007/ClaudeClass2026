#include <iostream>
#include <string>
using namespace std;

class Widget {
public:
    int id = 0;

    void showAddress() {
        cout << "對象 id=" << id
             << " 的地址: " << this << endl;
    }
};

int main() {
    Widget a, b, c;
    a.id = 1;
    b.id = 2;
    c.id = 3;

    a.showAddress();   // this == &a
    b.showAddress();   // this == &b
    c.showAddress();   // this == &c

    // 驗證：直接取地址比較
    cout << "\n直接取地址驗證:" << endl;
    cout << "&a = " << &a << endl;
    cout << "&b = " << &b << endl;
    cout << "&c = " << &c << endl;

    return 0;
}
