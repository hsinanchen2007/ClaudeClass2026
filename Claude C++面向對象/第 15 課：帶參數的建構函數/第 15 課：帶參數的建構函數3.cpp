#include <iostream>
#include <string>
using namespace std;

class BadExample {
private:
    string name;
    int age;

public:
    // 參數名和成員變數名完全相同！
    // 這裡的 name 和 age 都是參數，成員變數 name 和 age 沒有被修改！
    // 這樣的寫法會導致成員變數 name 和 age 保持未初始化狀態！
    // 這是非常常見的錯誤，會導致程式行為不可預測！
    BadExample(string name, int age) {
        name = name;   // 這裡是把參數 name 賦值給參數 name 自己！
        age = age;     // 成員變數 age 完全沒被修改！
    }
    
    void print() const {
        cout << "name = [" << name << "], age = " << age << endl;
    }
};

int main() {
    BadExample b("張三", 25);
    b.print();  // name 是空的，age 是垃圾值！
    return 0;
}
