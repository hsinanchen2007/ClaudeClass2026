#include <iostream>
#include <string>
using namespace std;

// 基類（父類）
class Animal {
public:
    string name;

    void eat() {
        cout << name << " 正在吃東西" << endl;
    }
};

// 派生類（子類）—— 繼承了 Animal 的一切
class Dog : public Animal {
public:
    void bark() {
        cout << name << " 汪汪叫！" << endl;
    }
};

class Cat : public Animal {
public:
    void meow() {
        cout << name << " 喵喵叫！" << endl;
    }
};

int main() {
    Dog d;
    d.name = "旺財";
    d.eat();   // 繼承自 Animal
    d.bark();  // Dog 自己的行為

    Cat c;
    c.name = "咪咪";
    c.eat();   // 繼承自 Animal
    c.meow();  // Cat 自己的行為

    return 0;
}
