// 檔案：lesson_5_2_cas_lock.cpp
// 說明：使用 CAS 實現的自旋鎖

#include <iostream>
#include <thread>
#include <atomic>

class CASSpinlock {
private:
    std::atomic<bool> locked{false};
    
public:
    void lock() {
        bool expected = false;
        
        // compare_exchange_strong：
        // 如果 locked == expected(false)，則設為 true，返回 true
        // 如果 locked != expected，則 expected 被更新為 locked 的值，返回 false
        while (!locked.compare_exchange_strong(expected, true)) {
            expected = false;  // 重設 expected 為 false
            // 繼續嘗試
        }
    }
    
    void unlock() {
        locked.store(false);
    }
};

CASSpinlock cas_lock;
int shared_counter = 0;

void worker() {
    for (int i = 0; i < 100000; ++i) {
        cas_lock.lock();
        ++shared_counter;
        cas_lock.unlock();
    }
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    
    t1.join();
    t2.join();
    
    std::cout << "counter = " << shared_counter << std::endl;
    
    return 0;
}
