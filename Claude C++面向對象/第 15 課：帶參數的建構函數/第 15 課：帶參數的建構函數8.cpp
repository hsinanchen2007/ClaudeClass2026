#include <iostream>
using namespace std;

class Holder {
public:
    int value;
    Holder(int v) : value(v) { }
};

int main() {
    double pi = 3.14;
    
    Holder h1(pi);     // OK：double → int，截斷為 3（小括號允許）
    // Holder h2{pi};  // 編譯錯誤（或警告）！大括號禁止窄化轉換！
                       // 這裡的錯誤是因為大括號初始化禁止了從 double 到 int 的窄化轉換，這樣可以避免意外的數據丟失。
    
    Holder h3{3};      // OK：int → int，沒有窄化
                       // 這裡的初始化是合法的，因為從 int 到 int 沒有任何數據丟失。
                       // 這裡的錯誤是因為大括號初始化禁止了從 double 到 int 的窄化轉換，這樣可以避免意外的數據丟失。
    
    cout << "h1.value = " << h1.value << endl;  // 3
    cout << "h3.value = " << h3.value << endl;  // 3
    
    return 0;
}
