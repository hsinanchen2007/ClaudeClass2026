// 檔案：lesson_5_3_try_lock_raii.cpp
// 說明：使用 unique_lock 搭配 try_lock

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;
int counter = 0;

void safeIncrement(int id) {
    // std::try_to_lock 讓 unique_lock 使用 try_lock() 而非 lock()
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    
    if (lock.owns_lock()) {
        // 成功獲取鎖
        ++counter;
        std::cout << "執行緒 " << id << "：成功遞增，counter = " 
                  << counter << std::endl;
        // lock 離開作用域時自動 unlock
    } else {
        // 獲取失敗
        std::cout << "執行緒 " << id << "：無法獲取鎖" << std::endl;
    }
    // 無論成功與否，都不需要手動 unlock
}

int main() {
    std::thread t1(safeIncrement, 1);
    std::thread t2(safeIncrement, 2);
    std::thread t3(safeIncrement, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "最終 counter = " << counter << std::endl;
    
    return 0;
}
