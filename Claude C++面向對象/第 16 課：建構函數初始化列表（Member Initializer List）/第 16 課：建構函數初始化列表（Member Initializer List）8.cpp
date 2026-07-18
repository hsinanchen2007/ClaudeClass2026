#include <iostream>
#include <string>
#include <cmath>
using namespace std;

#define M_PI 3.14159265358979323846

class Circle {
private:
    double radius;
    double area;
    double circumference;

public:
    // 在初始化列表中使用表達式計算
    // Circle 的建構函數使用了初始化列表，並且在其中計算了 area 和 circumference
    // 優點：效率更高，特別是對於 const 成員和參考成員來說，必須使用初始化列表來初始化，因為它們不能在函數體內賦值
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值，且可以使用表達式來計算初始值
    Circle(double r) 
        : radius(r),
          area(M_PI * r * r),              // 用公式計算
          circumference(2.0 * M_PI * r)     // 用公式計算
    { }

    void print() const {
        cout << "  半徑: " << radius << endl;
        cout << "  面積: " << area << endl;
        cout << "  周長: " << circumference << endl;
    }
};

class FullName {
private:
    string firstName;
    string lastName;
    string fullName;

public:
    // 初始化列表中做字串串接
    // FullName 的建構函數使用了初始化列表，並且在其中串接了 fullName
    // 優點：效率更高，特別是對於 const 成員和參考成員來說，必須使用初始化列表來初始化，因為它們不能在函數體內賦值
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值，且可以使用表達式來計算初始值
    FullName(const string& first, const string& last) 
        : firstName(first),
          lastName(last),
          fullName(last + " " + first)    // 串接
    { }

    void print() const {
        cout << "  姓: " << lastName << endl;
        cout << "  名: " << firstName << endl;
        cout << "  全名: " << fullName << endl;
    }
};

int main() {
    cout << "=== Circle ===" << endl;
    Circle c(5.0);
    c.print();
    
    cout << "\n=== FullName ===" << endl;
    FullName fn("信安", "陳");
    fn.print();
    
    return 0;
}
