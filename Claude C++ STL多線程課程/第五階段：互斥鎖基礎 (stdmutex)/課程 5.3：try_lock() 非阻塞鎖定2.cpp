// 檔案：lesson_5_3_retry_pattern.cpp
// 說明：使用 try_lock() 實現有限次數的重試

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;
int shared_data = 0;

bool tryUpdateWithRetry(int id, int maxRetries) {
    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        if (mtx.try_lock()) {
            // 成功獲取鎖
            std::cout << "執行緒 " << id << "：第 " << attempt 
                      << " 次嘗試成功" << std::endl;
            
            ++shared_data;
            
            // 模擬一些工作
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            mtx.unlock();
            return true;
        }
        
        std::cout << "執行緒 " << id << "：第 " << attempt 
                  << " 次嘗試失敗，稍後重試..." << std::endl;
        
        // 等待一小段時間再重試
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "執行緒 " << id << "：達到最大重試次數，放棄" << std::endl;
    return false;
}

int main() {
    std::thread t1(tryUpdateWithRetry, 1, 5);
    std::thread t2(tryUpdateWithRetry, 2, 5);
    
    t1.join();
    t2.join();
    
    std::cout << "最終 shared_data = " << shared_data << std::endl;
    
    return 0;
}
