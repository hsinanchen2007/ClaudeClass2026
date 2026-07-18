// 檔案：lesson_5_5_bank_account.cpp
// 說明：執行緒安全的銀行帳戶類別

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <stdexcept>

class BankAccount {
private:
    mutable std::mutex mtx;
    std::string accountId;
    double balance;
    
public:
    BankAccount(const std::string& id, double initialBalance)
        : accountId(id), balance(initialBalance) {
        if (initialBalance < 0) {
            throw std::invalid_argument("初始餘額不能為負");
        }
    }
    
    // 禁止複製
    BankAccount(const BankAccount&) = delete;
    BankAccount& operator=(const BankAccount&) = delete;
    
    // 允許移動
    BankAccount(BankAccount&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mtx);
        accountId = std::move(other.accountId);
        balance = other.balance;
        other.balance = 0;
    }
    
    // 存款
    void deposit(double amount) {
        if (amount <= 0) {
            throw std::invalid_argument("存款金額必須為正");
        }
        
        std::lock_guard<std::mutex> lock(mtx);
        balance += amount;
        std::cout << "[" << accountId << "] 存款 " << amount 
                  << "，餘額：" << balance << std::endl;
    }
    
    // 提款
    bool withdraw(double amount) {
        if (amount <= 0) {
            throw std::invalid_argument("提款金額必須為正");
        }
        
        std::lock_guard<std::mutex> lock(mtx);
        
        if (balance >= amount) {
            balance -= amount;
            std::cout << "[" << accountId << "] 提款 " << amount 
                      << "，餘額：" << balance << std::endl;
            return true;
        }
        
        std::cout << "[" << accountId << "] 提款失敗：餘額不足" << std::endl;
        return false;
    }
    
    // 查詢餘額
    double getBalance() const {
        std::lock_guard<std::mutex> lock(mtx);
        return balance;
    }
    
    // 獲取帳戶 ID
    std::string getId() const {
        std::lock_guard<std::mutex> lock(mtx);
        return accountId;
    }
    
    // 轉帳（靜態方法，需要鎖定兩個帳戶）
    static bool transfer(BankAccount& from, BankAccount& to, double amount) {
        if (&from == &to) {
            return false;  // 不能轉給自己
        }
        
        if (amount <= 0) {
            throw std::invalid_argument("轉帳金額必須為正");
        }
        
        // 使用 std::scoped_lock 同時鎖定兩個帳戶，避免死結
        std::scoped_lock lock(from.mtx, to.mtx);
        
        if (from.balance >= amount) {
            from.balance -= amount;
            to.balance += amount;
            std::cout << "[轉帳] " << from.accountId << " → " << to.accountId 
                      << "：" << amount << std::endl;
            return true;
        }
        
        std::cout << "[轉帳失敗] " << from.accountId << " 餘額不足" << std::endl;
        return false;
    }
};

// 模擬交易
void simulateTransactions(BankAccount& account, int id) {
    for (int i = 0; i < 5; ++i) {
        account.deposit(100);
        account.withdraw(50);
    }
}

int main() {
    BankAccount alice("Alice", 1000);
    BankAccount bob("Bob", 500);
    
    std::cout << "=== 初始狀態 ===" << std::endl;
    std::cout << "Alice: " << alice.getBalance() << std::endl;
    std::cout << "Bob: " << bob.getBalance() << std::endl;
    
    std::cout << "\n=== 並行交易 ===" << std::endl;
    
    std::thread t1([&]() {
        for (int i = 0; i < 3; ++i) {
            BankAccount::transfer(alice, bob, 100);
        }
    });
    
    std::thread t2([&]() {
        for (int i = 0; i < 3; ++i) {
            BankAccount::transfer(bob, alice, 50);
        }
    });
    
    std::thread t3(simulateTransactions, std::ref(alice), 1);
    std::thread t4(simulateTransactions, std::ref(bob), 2);
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    std::cout << "\n=== 最終狀態 ===" << std::endl;
    std::cout << "Alice: " << alice.getBalance() << std::endl;
    std::cout << "Bob: " << bob.getBalance() << std::endl;
    std::cout << "總額: " << (alice.getBalance() + bob.getBalance()) << std::endl;
    std::cout << "（初始總額為 1500，應保持不變）" << std::endl;
    
    return 0;
}
