#include <iostream>
using namespace std;

class Empty {
    // 什麼都沒有
};

class OnlyFunction {
public:
    void doSomething() {
        cout << "doing" << endl;
    }
};

class WithData {
public:
    int x;      // 4 bytes
    int y;      // 4 bytes
    double z;   // 8 bytes

    void show() {
        cout << x << ", " << y << ", " << z << endl;
    }
};

class Mixed {
public:
    char a;     // 1 byte
    int b;      // 4 bytes
    char c;     // 1 byte
};

int main() {
    cout << "sizeof(Empty)        = " << sizeof(Empty) << " bytes" << endl;
    cout << "sizeof(OnlyFunction) = " << sizeof(OnlyFunction) << " bytes" << endl;
    cout << "sizeof(WithData)     = " << sizeof(WithData) << " bytes" << endl;
    cout << "sizeof(Mixed)        = " << sizeof(Mixed) << " bytes" << endl;

    return 0;
}
