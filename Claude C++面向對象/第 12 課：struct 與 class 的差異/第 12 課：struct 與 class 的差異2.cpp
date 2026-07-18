#include <iostream>
using namespace std;

struct StructExample {
    int x = 10;    // 預設 public, 所以外部可以存取
    void show() { cout << "struct x = " << x << endl; }
};

class ClassExample {
    int x = 10;    // 預設 private, 所以外部無法存取
    void show() { cout << "class x = " << x << endl; }
};

int main() {
    StructExample s;
    cout << s.x << endl;    // ✅ public，可以存取
    s.show();               // ✅ public

    ClassExample c;
    // cout << c.x << endl; // ❌ 編譯錯誤！private
    // c.show();            // ❌ 編譯錯誤！private

    return 0;
}
