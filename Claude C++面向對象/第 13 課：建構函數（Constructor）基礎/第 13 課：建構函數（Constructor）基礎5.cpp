#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int age;
    float gpa;

public:
    // 無參數建構函數
    Student() {
        name = "未命名";
        age = 0;
        gpa = 0.0f;
        cout << "[預設建構] 創建學生: " << name << endl;
    }

    // 帶參數的建構函數
    Student(string n, int a, float g) {
        name = n;
        age = a;
        gpa = g;
        cout << "[帶參建構] 創建學生: " << name << endl;
    }

    void print() const {
        cout << "  姓名: " << name 
             << ", 年齡: " << age 
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    cout << "=== 使用預設建構函數 ===" << endl;
    Student s1;           // 調用無參數建構函數
    s1.print();
    
    cout << "\n=== 使用帶參數建構函數 ===" << endl;
    Student s2("張三", 20, 3.8f);   // 調用帶參數建構函數
    s2.print();
    
    Student s3("李四", 22, 3.5f);   // 另一個帶參數建構
    s3.print();
    
    return 0;
}
