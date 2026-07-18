#include <iostream>
using namespace std;

class Demo {
private:
    int instanceVar_ = 10;
    inline static int staticVar_ = 20;

public:
    void normalFunc() {
        // 有 this
        cout << "  this = " << this << endl;
        cout << "  instanceVar_ = " << this->instanceVar_ << endl;  // ✅
        cout << "  staticVar_ = " << staticVar_ << endl;             // ✅ 不用 this
    }

    static void staticFunc() {
        // 沒有 this
        // cout << this;               // ❌ 編譯錯誤
        // cout << instanceVar_;        // ❌ 需要 this
        cout << "  staticVar_ = " << staticVar_ << endl;  // ✅
    }
};

int main() {
    cout << "=== this 與靜態成員 ===" << endl;

    Demo d;
    cout << "\n--- 普通函數（有 this）---" << endl;
    d.normalFunc();

    cout << "\n--- 靜態函數（沒有 this）---" << endl;
    Demo::staticFunc();

    return 0;
}
