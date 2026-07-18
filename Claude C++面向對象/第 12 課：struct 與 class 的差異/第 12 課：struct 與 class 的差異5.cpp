#include <iostream>
#include <string>
using namespace std;

// ✅ 適合用 class —— 有行為、有保護
class BankAccount {
public:
    void init(const string& name, double initial) {
        ownerName = name;
        if (initial >= 0) balance = initial;
    }

    bool deposit(double amount) {
        if (amount <= 0) return false;
        balance += amount;
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) return false;
        balance -= amount;
        return true;
    }

    double getBalance() { return balance; }
    string getOwner() { return ownerName; }

private:
    string ownerName;
    double balance = 0.0;   // 不變條件：balance >= 0
};

int main() {
    BankAccount acc;
    acc.init("陳信安", 1000);
    acc.deposit(500);
    acc.withdraw(200);
    cout << acc.getOwner() << ": $" << acc.getBalance() << endl;
    return 0;
}
