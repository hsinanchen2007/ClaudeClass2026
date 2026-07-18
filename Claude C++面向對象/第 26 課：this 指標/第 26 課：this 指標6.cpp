#include <iostream>
using namespace std;

class TypeDemo {
private:
    int value_;

public:
    TypeDemo(int v) : value_(v) {}

    // 普通成員函數
    void normalFunc() {
        // this 的類型：TypeDemo* const
        // → 指向非 const 的 TypeDemo
        // → 指標本身是 const（不能讓 this 指向別的對象）

        this->value_ = 42;     // ✅ 可以修改
        // this = nullptr;     // ❌ this 本身是 const
    }

    // const 成員函數
    void constFunc() const {
        // this 的類型：const TypeDemo* const
        // → 指向 const 的 TypeDemo
        // → 指標本身也是 const

        // this->value_ = 42;  // ❌ 指向 const 對象，不能修改
        // this = nullptr;     // ❌ this 本身也是 const
        int v = this->value_;  // ✅ 可以讀取
    }
};

int main() {
    cout << "=== this 的類型 ===" << endl;

    cout << "  普通成員函數中：TypeDemo* const this" << endl;
    cout << "  const 成員函數中：const TypeDemo* const this" << endl;
    cout << "  靜態成員函數中：沒有 this" << endl;

    return 0;
}
