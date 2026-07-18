#include <iostream>
#include <string>
using namespace std;

class Cat {
public:
    string name;

    void meow() {
        cout << name << " 喵～" << endl;
    }
};

int main() {
    // ===== 棧上 =====
    Cat c1;                // 自動分配
    c1.name = "咪咪";
    c1.meow();             // 用 . 存取

    // ===== 堆上 =====
    Cat* c2 = new Cat();   // 手動分配
    c2->name = "花花";
    c2->meow();            // 用 -> 存取

    // c2 也可以用 (*c2).meow()，但 -> 更方便
    (*c2).meow();          // 和上一行等價

    delete c2;             // 手動釋放
    c2 = nullptr;

    return 0;
}   // c1 在這裡自動銷毀
