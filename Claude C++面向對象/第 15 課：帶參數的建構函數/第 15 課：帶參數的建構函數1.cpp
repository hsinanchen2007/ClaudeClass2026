#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int age;

public:
    // 方式 A：值傳遞（Value Pass）
    // 每次調用都會複製一份 string
    Student(string n, int a) {
        name = n;    // 這裡又複製了一次
        age = a;
        cout << "[值傳遞] 創建: " << name << endl;
    }

    void print() const {
        cout << "  " << name << ", " << age << " 歲" << endl;
    }
};

class StudentBetter {
private:
    string name;
    int age;

public:
    // 方式 B：const 引用傳遞（推薦）
    // 不會複製 string，只傳遞引用
    StudentBetter(const string& n, int a) {
        name = n;    // 只在這裡複製一次
        age = a;
        cout << "[const 引用] 創建: " << name << endl;
    }

    void print() const {
        cout << "  " << name << ", " << age << " 歲" << endl;
    }
};

int main() {
    string myName = "張三";
    
    cout << "=== 值傳遞 ===" << endl;
    Student s1(myName, 20);
    s1.print();
    
    cout << "\n=== const 引用傳遞 ===" << endl;
    StudentBetter s2(myName, 20);
    s2.print();
    
    return 0;
}
