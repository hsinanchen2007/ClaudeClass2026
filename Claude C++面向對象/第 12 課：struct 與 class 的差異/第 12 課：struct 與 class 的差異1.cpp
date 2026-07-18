#include <iostream>
#include <string>
using namespace std;

// 用 struct 寫的「完整 OOP」
struct DogStruct {
private:
    string name;
    int age = 0;

public:
    void setName(const string& n) { name = n; }
    void setAge(int a) { age = a; }

    void bark() {
        cout << name << "（struct）汪汪！" << endl;
    }
};

// 用 class 寫的完全等價版本
class DogClass {
private:
    string name;
    int age = 0;

public:
    void setName(const string& n) { name = n; }
    void setAge(int a) { age = a; }

    void bark() {
        cout << name << "（class）汪汪！" << endl;
    }
};

int main() {
    DogStruct ds;
    ds.setName("旺財");
    ds.bark();

    DogClass dc;
    dc.setName("小黑");
    dc.bark();

    return 0;
}
