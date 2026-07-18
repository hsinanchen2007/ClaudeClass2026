#include <iostream>
using namespace std;

class Trap {
private:
    int value_;

public:
    Trap(int v) : value_(v) {}

    // ❌ 危險！返回指向局部對象的指標
    // static Trap* createBad() {
    //     Trap local(99);
    //     return &local;   // local 離開作用域就死了！
    // }

    // ✅ 安全：返回動態分配的對象
    static Trap* createGood() {
        return new Trap(99);   // 堆上的對象不會自動銷毀
    }

    // ✅ 安全：返回值（拷貝）
    static Trap createBest() {
        return Trap(99);       // 返回值，拷貝/移動到調用方
    }

    int getValue() const { return value_; }
};

int main() {
    cout << "=== 誤區二：安全的創建方式 ===" << endl;

    Trap* p = Trap::createGood();
    cout << "  動態創建：" << p->getValue() << endl;
    delete p;

    Trap t = Trap::createBest();
    cout << "  值返回：" << t.getValue() << endl;

    return 0;
}
