#include <iostream>
using namespace std;

class Demo {
public:
    void func1() { cout << "func1" << endl; }

private:
    int data1 = 0;

public:     // 第二個 public 區塊 —— 完全合法
    void func2() { cout << "func2, data1=" << data1 << endl; }

private:    // 第二個 private 區塊
    int data2 = 0;

public:     // 第三個 public 區塊
    void func3() {
        cout << "func3, data1=" << data1
             << ", data2=" << data2 << endl;
    }
};

int main() {
    Demo d;
    d.func1();    // ✅
    d.func2();    // ✅
    d.func3();    // ✅
    // d.data1;   // ❌ private
    // d.data2;   // ❌ private
    return 0;
}
