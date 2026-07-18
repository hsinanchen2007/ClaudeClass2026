#include <iostream>
#include <string>
using namespace std;

class Printer {
public:
    // 同名函數，不同參數 —— 這就是重載
    // 函數重載（Function Overloading）是 C++ 中的一種特性，允許在同一個作用域內定義多個同名但參數列表不同的函數。
    // 編譯器會根據函數調用時提供的參數類型和數量來決定調用哪一個函數。
    // 這使得我們可以使用相同的函數名稱來處理不同類型或數量的輸入，從而提高代碼的可讀性和靈活性。
    void print(int value) {
        cout << "整數: " << value << endl;
    }

    void print(double value) {
        cout << "浮點數: " << value << endl;
    }

    void print(const string& value) {
        cout << "字串: " << value << endl;
    }

    void print(int value, int width) {
        cout << "整數(寬度 " << width << "): ";
        // 簡單的右對齊
        string s = to_string(value);
        for (int i = 0; i < width - (int)s.length(); i++) {
            cout << " ";
        }
        cout << s << endl;
    }
};

int main() {
    Printer p;

    p.print(42);              // 呼叫 print(int)
    p.print(3.14);            // 呼叫 print(double)
    p.print("Hello");         // 呼叫 print(const string&)
    p.print(42, 10);          // 呼叫 print(int, int)

    return 0;
}
