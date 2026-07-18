#include <iostream>
using namespace std;

class MyClass {
    int x = 10;   // 預設 private
public:
    int getX() { return x; }
};

struct MyStruct {
    int x = 10;   // 預設 public
};

int main() {
    MyClass c;
    // cout << c.x;    // ❌ 編譯錯誤！x 是 private
    cout << "class: " << c.getX() << endl;

    MyStruct s;
    cout << "struct: " << s.x << endl;  // ✅ x 是 public

    return 0;
}
