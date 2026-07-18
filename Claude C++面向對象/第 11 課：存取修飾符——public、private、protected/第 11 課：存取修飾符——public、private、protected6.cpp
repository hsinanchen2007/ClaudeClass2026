#include <iostream>
#include <string>
using namespace std;

class BankAccount {
public:
    // === 對外介面 ===

    // 建立帳戶（目前用普通函數，第 13 課會改用建構函數）
    void init(const string& ownerName, const string& accId, double initialBalance) {
        owner = ownerName;
        accountId = accId;
        if (initialBalance >= 0) {
            balance = initialBalance;
        } else {
            balance = 0;
            cout << "警告：初始餘額不能為負，已設為 0" << endl;
        }
    }

    // 存款
    bool deposit(double amount) {
        if (amount <= 0) {
            cout << "錯誤：存款金額必須大於 0" << endl;
            return false;
        }
        balance += amount;
        addTransaction("存款", amount);
        return true;
    }

    // 提款
    bool withdraw(double amount) {
        if (amount <= 0) {
            cout << "錯誤：提款金額必須大於 0" << endl;
            return false;
        }
        if (amount > balance) {
            cout << "錯誤：餘額不足（目前: $" << balance << "）" << endl;
            return false;
        }
        balance -= amount;
        addTransaction("提款", amount);
        return true;
    }

    // 查詢（只讀操作）
    double getBalance() {
        return balance;
    }

    string getOwner() {
        return owner;
    }

    string getAccountId() {
        return accountId;
    }

    int getTransactionCount() {
        return transactionCount;
    }

    // 顯示帳戶資訊
    void display() {
        cout << "================================" << endl;
        cout << "帳戶持有人: " << owner << endl;
        cout << "帳號:       " << accountId << endl;
        cout << "餘額:       $" << balance << endl;
        cout << "交易次數:   " << transactionCount << endl;
        cout << "================================" << endl;
    }

private:
    // === 內部資料（外界不能直接碰）===
    string owner = "";
    string accountId = "";
    double balance = 0.0;
    int transactionCount = 0;

    // === 內部輔助函數 ===
    void addTransaction(const string& type, double amount) {
        transactionCount++;
        cout << "[交易 #" << transactionCount << "] "
             << type << " $" << amount
             << " → 餘額: $" << balance << endl;
    }
};

int main() {
    BankAccount acc;
    acc.init("陳信安", "ACC-001", 5000.0);
    acc.display();

    acc.deposit(2000);
    acc.withdraw(1500);
    acc.withdraw(10000);   // 餘額不足
    acc.deposit(-500);     // 無效金額

    cout << endl;
    acc.display();

    // 安全地查詢
    cout << "\n查詢餘額: $" << acc.getBalance() << endl;

    // 以下操作全部會被編譯器攔截：
    // acc.balance = 999999;         // ❌ private！
    // acc.owner = "";               // ❌ private！
    // acc.transactionCount = 0;     // ❌ private！
    // acc.addTransaction("偷", 1);  // ❌ private 函數！

    return 0;
}
