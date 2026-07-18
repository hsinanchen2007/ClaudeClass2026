#include <iostream>
#include <string>
using namespace std;

class Demo {
private:
    string name;
    int value;

public:
    // 方式 A：函數體內賦值
    // 過程：name 先被預設建構（空字串），然後被賦值為 n
    // 相當於：string name;  然後  name = n;  （兩步）
    // 注意：這種方式對於複雜類型（如 string）來說，效率較低，因為先預設建構再賦值會有額外的開銷
    // 函數體內賦值的語法：在建構函數的函數體內進行賦值
    Demo(const string& n, int v) {
        cout << "  [賦值方式]" << endl;
        cout << "  賦值前 name = [" << name << "]（已經被預設建構過了）" << endl;
        name = n;     // 賦值
        value = v;
    }

    void print() const {
        cout << "  name = " << name << ", value = " << value << endl;
    }
};

class DemoInit {
private:
    string name;
    int value;

public:
    // 方式 B：初始化列表
    // 過程：name 直接用 n 來建構，一步到位
    // 相當於：string name(n);  或  string name = n;  （一步）
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值
    // 優點：效率更高，特別是對於複雜類型（如 string）來說，避免了先預設建構再賦值的過程
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值
    DemoInit(const string& n, int v) 
        : name(n), value(v) 
    {
        cout << "  [初始化列表方式]" << endl;
        cout << "  name 直接就是 [" << name << "]（一步完成）" << endl;
    }

    void print() const {
        cout << "  name = " << name << ", value = " << value << endl;
    }
};

int main() {
    cout << "=== 函數體內賦值 ===" << endl;
    Demo d1("Hello", 42);
    d1.print();
    
    cout << "\n=== 初始化列表 ===" << endl;
    DemoInit d2("Hello", 42);
    d2.print();
    
    return 0;
}
