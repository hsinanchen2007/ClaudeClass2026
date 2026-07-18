#include <iostream>
#include <string>
using namespace std;

class BankAccount {
public:
    string owner;
    double balance = 0.0;
};

int main() {
    BankAccount acc;
    acc.owner = "陳信安";
    acc.balance = 10000.0;

    // 任何人都能這樣做！
    acc.balance = -999999.0;   // 餘額變成負數？
    acc.balance = 0.0;         // 直接清空別人的錢？
    acc.owner = "";            // 帳戶名被清空？

    cout << acc.owner << ": $" << acc.balance << endl;
    return 0;
}
