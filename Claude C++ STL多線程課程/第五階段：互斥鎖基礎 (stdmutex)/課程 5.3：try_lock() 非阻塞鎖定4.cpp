// 檔案：lesson_5_3_avoid_livelock.cpp
// 說明：使用隨機退避避免活鎖

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>

std::mutex mutex1;
std::mutex mutex2;

void robustOperation(int id) {
    // 隨機數產生器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);
    
    int attempts = 0;
    const int maxAttempts = 100;
    
    while (attempts < maxAttempts) {
        ++attempts;
        
        mutex1.lock();
        
        if (mutex2.try_lock()) {
            // 成功獲得兩個鎖
            std::cout << "執行緒 " << id << "：成功（嘗試 " 
                      << attempts << " 次）" << std::endl;
            
            // 執行操作
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            mutex2.unlock();
            mutex1.unlock();
            return;
        }
        
        // 釋放並等待隨機時間（指數退避的簡化版）
        mutex1.unlock();
        
        int waitTime = dis(gen);  // 1-10 毫秒的隨機等待
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }
    
    std::cout << "執行緒 " << id << "：達到最大嘗試次數" << std::endl;
}

int main() {
    std::thread t1(robustOperation, 1);
    std::thread t2(robustOperation, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
