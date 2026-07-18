#include <iostream>
using namespace std;

class Demo {
private:
    int value_;

public:
    Demo(int v) : value_(v) {}

    // 普通成員函數
    // 在普通成員函數中，this 的類型是 Demo* const，表示 this 是一個指向 Demo 對象的常量指針（指針本身不可修改，但指向的對象可以修改）
    // 這裡的 modify() 函數是非 const 成員函數，可以修改對象的狀態（成員變量），因此 this 的類型是 Demo* const
    // 這裡的 inspect() 函數是 const 成員函數，承諾不修改對象的狀態，因此 this 的類型是 const Demo* const
    void modify() {
        // this 的類型是 Demo* const
        this->value_ = 999;        // ✅ 可以修改
        cout << "  modify(): value_ = " << value_ << endl;
    }

    // const 成員函數
    // 在 const 成員函數中，this 的類型是 const Demo* const，表示 this 是一個指向 Demo 對象的常量指針，並且指向的對象也是常量（不可修改）
    // 這裡的 inspect() 函數是 const 成員函數，承諾不修改對象的狀態，因此 this 的類型是 const Demo* const
    void inspect() const {
        // this 的類型是 const Demo* const
        // this->value_ = 999;     // ❌ 編譯錯誤！
        cout << "  inspect(): value_ = " << value_ << endl;
    }
};

int main() {
    cout << "=== this 指標與 const ===" << endl;

    Demo d(42);
    d.inspect();    // const 函數
    d.modify();     // 非 const 函數
    d.inspect();    // 再次查看

    return 0;
}
