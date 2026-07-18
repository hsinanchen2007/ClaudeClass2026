#include <iostream>
#include <string>
using namespace std;

class Dog {
public:
    string name;

    void bark() {
        // 你寫的：
        cout << name << " 汪汪！" << endl;

        // 編譯器實際看到的（等價的寫法）：
        // cout << this->name << " 汪汪！" << endl;
    }
};

int main() {
    Dog d1;
    d1.name = "旺財";

    Dog d2;
    d2.name = "小黑";

    d1.bark();   // 編譯器轉換為：Dog::bark(&d1)  → this = &d1, 所以在 bark 函數內部，this->name 就是 d1.name
    d2.bark();   // 編譯器轉換為：Dog::bark(&d2)  → this = &d2. 所以在 bark 函數內部，this->name 就是 d2.name

    return 0;
}
