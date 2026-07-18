#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int age;
    float gpa;

public:
    // ====== 建構函數 ======
    // 函數名 = 類別名 "Student"
    // 沒有返回值（連 void 都不寫）
    Student() {
        cout << ">>> 建構函數被調用了！ <<<" << endl;
        name = "未命名";
        age = 0;
        gpa = 0.0f;
    }

    void print() const {
        cout << "姓名: " << name 
             << ", 年齡: " << age 
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    cout << "=== 準備創建對象 ===" << endl;
    
    Student s;  // 創建對象的瞬間，建構函數自動被調用
    
    cout << "=== 對象已創建 ===" << endl;
    s.print();  // 成員變數已被初始化，不再是垃圾值
    
    return 0;
}
