#include <iostream>
#include <string>
using namespace std;

class Knight {
private:
    string name_;
    int hp_;

public:
    Knight(const string& name, int hp)
        : name_(name), hp_(hp)
    {
    }

    void showThis() const {
        cout << "  " << name_ << " 的地址：" << this << endl;
    }

    void takeDamage(int dmg) {
        cout << "  this = " << this << " → " << name_
             << " 受到 " << dmg << " 傷害" << endl;
        hp_ -= dmg;
    }

    void printInfo() const {
        // 以下兩種寫法完全等價：
        cout << "  名字：" << name_ << endl;        // 隱式使用 this
        cout << "  名字：" << this->name_ << endl;   // 顯式使用 this
        // 編譯器會把 name_ 自動轉成 this->name_
    }
};

int main() {
    cout << "=== this 指向誰？ ===" << endl;

    Knight k1("亞瑟", 200);
    Knight k2("蘭斯洛特", 180);

    // 驗證 this 就是對象的地址
    cout << "\n--- 地址比較 ---" << endl;
    cout << "  &k1 = " << &k1 << endl;
    k1.showThis();
    cout << "  &k2 = " << &k2 << endl;
    k2.showThis();

    // 不同對象調用同一個函數，this 不同
    cout << "\n--- 不同對象，不同 this ---" << endl;
    k1.takeDamage(30);
    k2.takeDamage(50);

    // 驗證隱式 vs 顯式
    cout << "\n--- 隱式 vs 顯式 this ---" << endl;
    k1.printInfo();

    return 0;
}
