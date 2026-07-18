#include <iostream>
#include <string>
using namespace std;

class Animal {
protected:
    string species = "未知";   // 外界不能存取，但子類可以
    int legs = 0;

public:
    void showInfo() {
        cout << species << ", " << legs << " 條腿" << endl;
    }
};

class Dog : public Animal {
public:
    void setup() {
        species = "犬科";   // ✅ 子類可以存取 protected
        legs = 4;           // ✅
    }
};

class Spider : public Animal {
public:
    void setup() {
        species = "蛛形綱"; // ✅ 子類可以存取 protected
        legs = 8;           // ✅
    }
};

int main() {
    Dog d;
    d.setup();
    d.showInfo();

    Spider s;
    s.setup();
    s.showInfo();

    // d.species = "test";  // ❌ 編譯錯誤！外界不能存取 protected
    // d.legs = 100;        // ❌ 編譯錯誤！

    return 0;
}
