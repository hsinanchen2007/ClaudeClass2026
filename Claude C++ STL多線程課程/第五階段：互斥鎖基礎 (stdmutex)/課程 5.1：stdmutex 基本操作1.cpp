// 檔案：lesson_5_1_no_mutex.cpp
// 說明：沒有互斥鎖保護的計數器（有競爭條件）

#include <iostream>
#include <thread>

int counter = 0;  // 共享資源

void increment() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;  // 危險！非原子操作
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;  // 通常小於 200000
    
    return 0;
}
