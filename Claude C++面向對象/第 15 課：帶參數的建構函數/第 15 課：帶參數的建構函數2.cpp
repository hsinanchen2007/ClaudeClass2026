#include <iostream>
#include <string>
using namespace std;

class Demo {
public:
    string data;
    
    // 如果用普通引用（非 const）
    // Demo(string& s) { data = s; }
    
    // 用 const 引用（推薦）
    Demo(const string& s) { data = s; }
};

int main() {
    string name = "測試";
    
    // === 情境 1：傳入變數 ===
    // 普通引用和 const 引用都 OK！
    Demo d1(name);      // 普通引用和 const 引用都 OK
    
    // === 情境 2：傳入字面值 ===
    // 普通引用會編譯錯誤，因為字面值不能綁定到非 const 引用
    // const 引用可以綁定到字面值，因為它不會修改參數
    Demo d2("直接傳字串");   // 普通引用會編譯錯誤！
                              // const 引用 OK！
    
    // === 情境 3：傳入臨時對象 ===
    // 普通引用會編譯錯誤，因為臨時對象不能綁定到非 const 引用
    // const 引用可以綁定到臨時對象，因為它不會修改參數
    Demo d3(string("臨時字串"));  // 普通引用會編譯錯誤！
                                    // const 引用 OK！
    
    cout << d1.data << endl;
    cout << d2.data << endl;
    cout << d3.data << endl;
    
    return 0;
}
