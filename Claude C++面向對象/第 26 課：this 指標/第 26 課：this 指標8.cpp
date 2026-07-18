#include <iostream>
#include <string>
using namespace std;

class Dangerous {
private:
    int value_;

public:
    Dangerous(int v) : value_(v) {
        // ⚠ 在建構函數中，對象還沒完全初始化
        // 如果把 this 傳給外部，外部可能會使用未初始化的部分

        cout << "  建構中... value_ = " << value_ << endl;
        cout << "  this = " << this << " (對象可能還沒完全就緒)" << endl;

        // 這裡把 this 傳出去是危險的：
        // someGlobalFunction(this);  // ⚠ 對象可能還沒完全建構
    }
};

int main() {
    cout << "=== 誤區一：建構中洩漏 this ===" << endl;
    Dangerous d(42);
    cout << "  建構完成，現在使用 this 才安全" << endl;
    return 0;
}
