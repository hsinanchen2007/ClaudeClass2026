// 檔案：lesson_5_2_spinlock_concept.cpp
// 說明：展示自旋鎖的基本概念（僅供理解，非生產環境使用）

#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

class SimpleSpinlock {
private:
    std::atomic<bool> locked{false};
    
public:
    void lock() {
        // exchange 是原子操作：設定新值並返回舊值
        while (locked.exchange(true)) {
            // 如果舊值是 true，表示已被鎖定，繼續自旋
            // 這裡可以加入 yield 或 pause 來減少 CPU 消耗
        }
        // 如果舊值是 false，表示成功獲取鎖
    }
    
    void unlock() {
        locked.store(false);
    }
};

// 測試
SimpleSpinlock spinlock;
int counter = 0;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        spinlock.lock();
        ++counter;
        spinlock.unlock();
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;
    
    return 0;
}
