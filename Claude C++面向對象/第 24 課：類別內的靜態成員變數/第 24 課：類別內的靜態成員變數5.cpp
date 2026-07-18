#include <iostream>
#include <string>
using namespace std;

class BankAccount {
private:
    string owner_;
    double balance_;

    // 私有靜態成員：外部不能直接訪問
    // 這些靜態成員用於追蹤銀行的總存款、帳戶數量和利率
    // 靜態成員變數屬於類別本身，而不是任何特定的對象
    inline static double totalDeposits_ = 0.0;
    inline static int accountCount_ = 0;
    inline static double interestRate_ = 0.03;   // 3% 利率

public:
    BankAccount(const string& owner, double initial)
        : owner_(owner), balance_(initial > 0 ? initial : 0)
    {
        accountCount_++;
        totalDeposits_ += balance_;
        cout << "  [開戶] " << owner_ << " 存入 " << balance_ << endl;
    }

    void deposit(double amount) {
        if (amount <= 0) return;
        balance_ += amount;
        totalDeposits_ += amount;
    }

    double getBalance() const { return balance_; }
    const string& getOwner() const { return owner_; }

    // 計算利息——使用靜態的利率
    double calculateInterest() const {
        return balance_ * interestRate_;
    }

    // 公開靜態資訊的 getter（通過 public 函數控制訪問）
    static int getAccountCount() { return accountCount_; }
    static double getTotalDeposits() { return totalDeposits_; }
    static double getInterestRate() { return interestRate_; }

    // 設定利率（帶驗證）
    static void setInterestRate(double rate) {
        if (rate < 0 || rate > 0.20) {
            cout << "  利率必須在 0%~20% 之間！" << endl;
            return;
        }
        interestRate_ = rate;
        cout << "  利率已調整為 " << (rate * 100) << "%" << endl;
    }

    void printStatement() const {
        cout << "  " << owner_ << "：餘額 " << balance_
             << "，利息 " << calculateInterest() << endl;
    }
};

int main() {
    cout << "=== 靜態成員與訪問控制 ===" << endl;

    cout << "\n--- 開戶 ---" << endl;
    BankAccount a1("陳信安", 10000);
    BankAccount a2("王小明", 5000);
    BankAccount a3("李大華", 20000);

    // 通過公開的靜態函數訪問私有靜態數據
    cout << "\n--- 銀行統計 ---" << endl;
    cout << "  帳戶數：" << BankAccount::getAccountCount() << endl;
    cout << "  總存款：" << BankAccount::getTotalDeposits() << endl;
    cout << "  當前利率：" << (BankAccount::getInterestRate() * 100) << "%" << endl;

    // 計算各帳戶利息
    cout << "\n--- 利息計算（利率 3%）---" << endl;
    a1.printStatement();
    a2.printStatement();
    a3.printStatement();

    // 調整利率——影響所有帳戶
    cout << "\n--- 調整利率 ---" << endl;
    BankAccount::setInterestRate(0.05);

    cout << "\n--- 利息計算（利率 5%）---" << endl;
    a1.printStatement();
    a2.printStatement();
    a3.printStatement();

    // 非法利率
    BankAccount::setInterestRate(0.50);   // 被攔截

    // 不能直接訪問私有靜態成員：
    // BankAccount::totalDeposits_ = 0;   // ❌ 編譯錯誤！private

    return 0;
}
