#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 1. get_id() - 取得當前執行緒 ID
    std::cout << "ID: " << std::this_thread::get_id() << std::endl;
    
    // 2. sleep_for() - 休眠指定時間
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 3. sleep_until() - 休眠到指定時間點
    auto wakeTime = std::chrono::steady_clock::now() 
                  + std::chrono::milliseconds(100);
    std::this_thread::sleep_until(wakeTime);
    
    // 4. yield() - 讓出 CPU 時間給其他執行緒
    std::this_thread::yield();
    
    return 0;
}
