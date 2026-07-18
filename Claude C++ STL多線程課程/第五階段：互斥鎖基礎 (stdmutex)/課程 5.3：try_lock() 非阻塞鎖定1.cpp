// 檔案：lesson_5_3_try_lock_basic.cpp
// 說明：try_lock() 的基本使用方式

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;

void worker(int id) {
    // 嘗試獲取鎖
    if (mtx.try_lock()) {
        // 成功獲取鎖
        std::cout << "執行緒 " << id << "：成功獲取鎖，開始工作..." << std::endl;
        
        // 模擬工作
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "執行緒 " << id << "：工作完成，釋放鎖" << std::endl;
        mtx.unlock();  // 記得解鎖！
    } else {
        // 獲取失敗，立即返回
        std::cout << "執行緒 " << id << "：無法獲取鎖，執行其他任務" << std::endl;
    }
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}
