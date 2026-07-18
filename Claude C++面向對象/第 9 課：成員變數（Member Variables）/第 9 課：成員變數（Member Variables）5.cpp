#include <iostream>
using namespace std;

class Dangerous {
public:
    int x;
    double y;
    bool flag;
};

int main() {
    Dangerous d;

    // 以下輸出是「垃圾值」—— 每次執行可能不同！
    cout << "x    = " << d.x << endl;
    cout << "y    = " << d.y << endl;
    cout << "flag = " << d.flag << endl;

    return 0;
}
