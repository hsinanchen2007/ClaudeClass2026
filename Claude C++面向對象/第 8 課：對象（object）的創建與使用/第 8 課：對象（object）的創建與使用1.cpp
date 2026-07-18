#include <iostream>
#include <string>
using namespace std;

class Dog {
public:
    string name;
    int age = 0;

    void bark() {
        cout << name << " 汪汪！" << endl;
    }
};

int main() {
    Dog d1;           // 建立對象，就像宣告一個普通變數
    d1.name = "旺財";
    d1.age = 3;
    d1.bark();

    Dog d2;           // 再建一個，完全獨立
    d2.name = "小黑";
    d2.age = 5;
    d2.bark();

    return 0;
}   // d1 和 d2 在這裡自動銷毀
