#include <iostream>
#include <string>
using namespace std;

class BankAccount {
private:
    string owner;
    double balance;
    string accountId;

public:
    BankAccount(string ownerName, double initialBalance, string id) {
        // 驗證帳戶名
        if (ownerName.empty()) {
            cout << "警告：帳戶名不能為空，使用預設值" << endl;
            owner = "未知";
        } else {
            owner = ownerName;
        }
        
        // 驗證初始餘額
        if (initialBalance < 0) {
            cout << "警告：初始餘額不能為負數，設為 0" << endl;
            balance = 0.0;
        } else {
            balance = initialBalance;
        }
        
        // 驗證帳戶 ID
        if (id.length() != 10) {
            cout << "警告：帳戶 ID 必須為 10 位，使用預設 ID" << endl;
            accountId = "0000000000";
        } else {
            accountId = id;
        }
        
        cout << "帳戶創建成功: " << owner << endl;
    }

    void print() const {
        cout << "  帳戶: " << accountId 
             << ", 戶主: " << owner 
             << ", 餘額: $" << balance << endl;
    }
};

int main() {
    cout << "=== 正常創建 ===" << endl;
    BankAccount a1("王五", 10000.0, "1234567890");
    a1.print();
    
    cout << "\n=== 非法數據 ===" << endl;
    BankAccount a2("", -500.0, "123");  // 全部都是非法值
    a2.print();
    
    return 0;
}
