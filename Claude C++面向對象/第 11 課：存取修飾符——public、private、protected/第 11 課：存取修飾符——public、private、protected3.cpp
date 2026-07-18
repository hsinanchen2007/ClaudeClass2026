#include <iostream>
#include <string>
using namespace std;

class Safe {
private:
    string password = "secret123";
    double money = 50000.0;

public:
    bool unlock(const string& input) {
        if (input == password) {    // ✅ 類別內部可以存取 private
            cout << "保險箱已開啟，裡面有 $" << money << endl;
            return true;
        } else {
            cout << "密碼錯誤！" << endl;
            return false;
        }
    }
};

int main() {
    Safe mySafe;

    mySafe.unlock("wrong");      // ✅ 呼叫 public 函數
    mySafe.unlock("secret123");  // ✅ 呼叫 public 函數

    // mySafe.password;          // ❌ 編譯錯誤！private 不能從外部存取
    // mySafe.money = 0;         // ❌ 編譯錯誤！

    return 0;
}
