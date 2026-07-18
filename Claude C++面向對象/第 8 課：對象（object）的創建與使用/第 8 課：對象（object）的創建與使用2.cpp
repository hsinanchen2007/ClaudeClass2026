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
    Dog* ptr = new Dog();    // 在堆上建立，返回指標
    ptr->name = "阿花";      // 用 -> 存取成員
    ptr->age = 2;
    ptr->bark();

    delete ptr;              // 必須手動釋放！否則會造成記憶體洩漏
    ptr = nullptr;           // 好習慣：釋放後設為 nullptr, 避免野指標

    return 0;
}
