// 檔案：lesson_5_4_forget_unlock.cpp
// 說明：忘記解鎖的錯誤示範

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;
int counter = 0;

void buggyIncrement() {
    mtx.lock();
    ++counter;
    std::cout << "Counter = " << counter << std::endl;
    
    // 💀 忘記 unlock()！
    // mtx.unlock();
}

int main() {
    std::thread t1(buggyIncrement);
    t1.join();
    
    std::cout << "執行緒 1 完成" << std::endl;
    
    std::thread t2(buggyIncrement);  // 💀 永遠卡住！
    
    // 設定超時檢測
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "程式卡住了！（這行可能不會執行）" << std::endl;
    
    t2.join();  // 永遠不會返回
    
    return 0;
}
