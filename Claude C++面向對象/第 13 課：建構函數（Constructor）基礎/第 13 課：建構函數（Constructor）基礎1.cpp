#include <iostream>
#include <string>
using namespace std;

class Student {
public:
    string name;
    int age;
    float gpa;

    void print() {
        cout << "姓名: " << name 
             << ", 年齡: " << age 
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    Student s;
    s.print();  // name 是空字串, age 和 gpa 是未定義的垃圾值！
    return 0;
}
