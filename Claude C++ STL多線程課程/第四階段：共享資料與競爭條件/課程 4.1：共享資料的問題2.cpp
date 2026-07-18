#include <iostream>
#include <thread>

struct Account {
    int balance = 1000;
};

Account accountA, accountB;

void transfer(Account& from, Account& to, int amount) {
    if (from.balance >= amount) {
        from.balance -= amount;
        to.balance += amount;
    }
}

int main() {
    std::thread t1([&]() {
        for (int i = 0; i < 1000; ++i) {
            transfer(accountA, accountB, 1);
        }
    });
    
    std::thread t2([&]() {
        for (int i = 0; i < 1000; ++i) {
            transfer(accountB, accountA, 1);
        }
    });
    
    t1.join();
    t2.join();
    
    int total = accountA.balance + accountB.balance;
    std::cout << "A: " << accountA.balance << std::endl;
    std::cout << "B: " << accountB.balance << std::endl;
    std::cout << "總額: " << total << " (應為 2000)" << std::endl;
    
    return 0;
}
