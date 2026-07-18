#include <iostream>
#include <string>
using namespace std;

class Solution1 {
private:
    string name;
    int age;

public:
    Solution1(string name, int age) {
        this->name = name;   // this->name 是成員變數，name 是參數, 這裡是把參數 name 賦值給成員變數 name！
        this->age = age;     // 同理，this->age 是成員變數，age 是參數, 這裡是把參數 age 賦值給成員變數 age！
    }
    
    void print() const {
        cout << "name = " << name << ", age = " << age << endl;
    }
};

int main() {
    Solution1 s("李四", 30);
    s.print();  // name = 李四, age = 30
    return 0;
}