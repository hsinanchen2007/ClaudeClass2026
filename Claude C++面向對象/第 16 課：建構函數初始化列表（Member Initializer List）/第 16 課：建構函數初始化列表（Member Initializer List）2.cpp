#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    const int studentId;  // const 成員：一旦初始化就不能修改
    string name;

public:
    // 錯誤寫法：
    // Student(int id, const string& n) {
    //     studentId = id;  // 編譯錯誤！不能給 const 賦值！
    //     name = n;
    // }
    
    // 正確寫法：使用初始化列表
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值
    // 優點：效率更高，特別是對於 const 成員來說，必須使用初始化列表來初始化，因為它們不能在函數體內賦值
    Student(int id, const string& n) 
        : studentId(id), name(n) 
    { }

    void print() const {
        cout << "  學號: " << studentId << ", 姓名: " << name << endl;
    }
};

int main() {
    Student s(20250001, "張三");
    s.print();
    
    // s.studentId = 999;  // 編譯錯誤！const 不能修改
    
    return 0;
}
