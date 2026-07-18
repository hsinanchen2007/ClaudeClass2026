#include <iostream>
#include <string>
using namespace std;

// ========== 類別定義 ==========
class Dog {
public:
    // 成員變數（Member Variables）—— 描述「狗有什麼」
    string name;
    int age;
    string breed;  // 品種

    // 成員函數（Member Functions）—— 描述「狗能做什麼」
    void bark() {
        cout << name << " 說：汪汪！" << endl;
    }

    void sit() {
        cout << name << " 乖乖坐下了" << endl;
    }

    void showInfo() {
        cout << "名字: " << name << endl;
        cout << "年齡: " << age << " 歲" << endl;
        cout << "品種: " << breed << endl;
    }
};  // ← 別忘了分號！

// ========== 主程式 ==========
int main() {
    Dog myDog;              // 根據 Dog 類別建造一個對象
    myDog.name = "旺財";    // 設定屬性
    myDog.age = 3;
    myDog.breed = "柴犬";

    myDog.showInfo();       // 調用成員函數
    myDog.bark();
    myDog.sit();

    return 0;
}
