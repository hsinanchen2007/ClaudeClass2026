// 檔案：lesson_5_3_avoid_deadlock.cpp
// 說明：使用 try_lock() 避免死結

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mutex1;
std::mutex mutex2;

void safeOperation(int id, std::mutex& first, std::mutex& second, 
                   const char* firstName, const char* secondName) {
    while (true) {
        // 先鎖定第一個互斥鎖
        first.lock();
        std::cout << "執行緒 " << id << "：獲得 " << firstName << std::endl;
        
        // 嘗試鎖定第二個互斥鎖
        if (second.try_lock()) {
            // 成功獲得兩個鎖
            std::cout << "執行緒 " << id << "：獲得 " << secondName 
                      << "，執行操作" << std::endl;
            
            // 執行需要兩個鎖的操作
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // 釋放鎖（注意順序：後獲取的先釋放）
            second.unlock();
            first.unlock();
            
            std::cout << "執行緒 " << id << "：操作完成" << std::endl;
            return;  // 成功完成
        }
        
        // 無法獲得第二個鎖，釋放第一個鎖，避免死結
        std::cout << "執行緒 " << id << "：無法獲得 " << secondName 
                  << "，釋放 " << firstName << " 並重試" << std::endl;
        first.unlock();
        
        // 稍等一下再重試（避免活鎖）
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main() {
    // 兩個執行緒以不同順序請求鎖
    std::thread t1([](){ 
        safeOperation(1, mutex1, mutex2, "mutex1", "mutex2"); 
    });
    std::thread t2([](){ 
        safeOperation(2, mutex2, mutex1, "mutex2", "mutex1"); 
    });
    
    t1.join();
    t2.join();
    
    std::cout << "兩個執行緒都成功完成，沒有死結！" << std::endl;
    
    return 0;
}
