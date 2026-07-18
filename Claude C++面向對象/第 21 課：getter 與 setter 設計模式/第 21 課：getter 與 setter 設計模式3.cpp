#include <iostream>
#include <string>
#include <vector>
using namespace std;

class BankAccount {
private:
    string owner_;
    int balance_;
    vector<string> transactionLog_;

public:
    BankAccount(const string& owner, int initial)
        : owner_(owner), balance_(initial)
    {
        transactionLog_.push_back("開戶：" + to_string(initial));
    }

    // ===== 安全的 getter =====
    // 注意：這些 getter 都是 const 成員函數，保證不修改對象狀態
    int getBalance() const { return balance_; }
    const string& getOwner() const { return owner_; }
    const vector<string>& getLog() const { return transactionLog_; }

    // ===== 危險的 getter（反面教材）=====
    // 注意：這些 getter 返回非 const 引用，允許外部直接修改內部狀態，極易導致錯誤！
    // 這樣的設計完全破壞了封裝，外部可以繞過所有驗證邏輯，直接竄改內部數據！
    int& getBalanceDangerous() { return balance_; }
    vector<string>& getLogDangerous() { return transactionLog_; }

    // ===== 正確的修改介面 =====
    // 注意：這些方法帶有驗證邏輯，確保對象保持有效狀態，並且記錄所有操作
    bool deposit(int amount) {
        if (amount <= 0) return false;
        balance_ += amount;
        transactionLog_.push_back("存入：" + to_string(amount));
        return true;
    }

    bool withdraw(int amount) {
        if (amount <= 0 || amount > balance_) return false;
        balance_ -= amount;
        transactionLog_.push_back("取出：" + to_string(amount));
        return true;
    }

    void printStatement() const {
        cout << "  帳戶：" << owner_ << endl;
        cout << "  餘額：" << balance_ << endl;
        cout << "  交易記錄：" << endl;
        for (const auto& log : transactionLog_) {
            cout << "    " << log << endl;
        }
    }
};

int main() {
    cout << "=== 危險的 getter 示範 ===" << endl;

    BankAccount account("陳信安", 1000);

    // 正常操作
    cout << "\n--- 正常操作 ---" << endl;
    account.deposit(500);
    account.withdraw(200);
    account.printStatement();

    // 使用危險的 getter 繞過所有保護！
    // 注意：這樣做會直接修改內部狀態，完全繞過 deposit() 和 withdraw() 的驗證邏輯，極易導致錯誤！
    cout << "\n--- 使用危險的 getter ---" << endl;

    // 直接修改餘額，沒有驗證，沒有記錄！
    // 注意：這樣做會直接修改內部狀態，完全繞過 deposit() 和 withdraw() 的驗證邏輯，極易導致錯誤！
    account.getBalanceDangerous() = 999999;
    cout << "  餘額被直接竄改為：" << account.getBalance() << endl;

    // 直接竄改交易記錄！
    // 注意：這樣做會直接修改內部狀態，完全繞過所有驗證邏輯，極易導致錯誤！
    account.getLogDangerous().clear();
    account.getLogDangerous().push_back("一切正常，沒有異常");

    cout << "\n--- 竄改後的帳戶 ---" << endl;
    account.printStatement();

    return 0;
}
