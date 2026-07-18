#include <iostream>
using namespace std;

class OrderDemo {
private:
    int a;    // 第 1 個宣告
    int b;    // 第 2 個宣告
    int c;    // 第 3 個宣告

public:
    // 注意：初始化列表故意寫成 c, a, b 的順序
    // 但實際初始化順序是 a, b, c（按宣告順序）
    // 優點：效率更高，特別是對於 const 成員和參考成員來說，必須使用初始化列表來初始化，因為它們不能在函數體內賦值
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值
    OrderDemo(int val) 
        : c(val + 2),     // 寫在第 1 個，但第 3 個執行
          a(val),          // 寫在第 2 個，但第 1 個執行
          b(val + 1)       // 寫在第 3 個，但第 2 個執行
    {
        cout << "a = " << a << ", b = " << b << ", c = " << c << endl;
    }
};

int main() {
    OrderDemo d(10);
    return 0;
}
