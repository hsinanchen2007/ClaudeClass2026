#include <iostream>
#include <string>
using namespace std;

class Temp {
private:
    string name;
public:
    Temp(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~Temp() {
        cout << "  [-] " << name << endl;
    }
    
    string getName() const { return name; }
    
    // 返回自身引用，用於鏈式調用
    // 注意：返回 const Temp& 可以防止修改臨時對象
    // 這裡的 const Temp& 是為了展示臨時對象的生命週期，實際上在這個例子中並不會有修改行為
    // 返回 const Temp& 可以讓我們在調用 show() 後仍然能夠使用該對象，因為它的生命週期被延長了
    const Temp& show() const {
        cout << "  [=] " << name << " 展示中" << endl;
        return *this;
    }
};

Temp createTemp(const string& n) {
    return Temp(n);   // 返回一個臨時對象
}

int main() {
    cout << "=== 臨時對象的生命週期 ===" << endl;
    
    // 情境 1：臨時對象在語句結束時死亡
    cout << "\n--- 情境 1：純臨時對象 ---" << endl;
    Temp("短命鬼").show();
    cout << "  (臨時對象已死亡)\n" << endl;
    // Temp("短命鬼") 在這行的分號處就被解構了
    
    // 情境 2：用變數接住，延長生命
    cout << "--- 情境 2：用變數接住 ---" << endl;
    Temp saved = createTemp("被拯救的");
    saved.show();
    cout << "  (saved 還活著)\n" << endl;
    
    // 情境 3：const 引用延長臨時對象的生命
    cout << "--- 情境 3：const 引用延長生命 ---" << endl;
    const Temp& ref = Temp("引用續命");
    ref.show();
    cout << "  (ref 綁定的臨時對象還活著)\n" << endl;
    // 臨時對象的生命被延長到 ref 離開作用域時
    
    cout << "=== main() 結束 ===" << endl;
    return 0;
}
// 這段程式碼展示了 C++ 中臨時對象的生命週期：
// - 在情境 1 中，臨時對象在語句結束時被解構，無法再使用。
// - 在情境 2 中，將臨時對象賦值給  變數 saved，延長了它的生命週期，直到 saved 離開作用域時才被解構。
// - 在情境 3 中，使用 const 引用 ref 綁定臨時對象，延長了它的生命週期，直到 ref 離開作用域時才被解構。 
// 這些情境展示了 C++ 中對象生命週期的不同管理方式，以及如何利用變數和引用來控制對象的存活時間。