#include <iostream>
#include <thread>

struct Bank {
    int accountA = 1000;
    int accountB = 1000;
    // 不變量：accountA + accountB == 2000
};

Bank bank;

void transfer(int amount) {
    // 不變量暫時破壞
    bank.accountA -= amount;
    // ← 此刻總額不是 2000！
    bank.accountB += amount;
    // 不變量恢復
}

void audit() {
    int total = bank.accountA + bank.accountB;
    if (total != 2000) {
        std::cout << "警告！總額異常: " << total << std::endl;
    }
}

int main() {
    std::thread t1([]() {
        for (int i = 0; i < 1000; ++i) transfer(1);
    });
    
    std::thread t2([]() {
        for (int i = 0; i < 1000; ++i) audit();
    });
    
    t1.join();
    t2.join();
    
    return 0;
}
