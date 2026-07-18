// 檔案：lesson_5_4_scope_too_large.cpp
// 說明：保護範圍太大，降低效能

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;
int result = 0;

void inefficientWork(int input) {
    std::lock_guard<std::mutex> lock(mtx);  // 💀 整個函式都被鎖住
    
    // 這部分不需要鎖
    int temp = input * input;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 模擬耗時計算
    temp += input;
    
    // 只有這裡需要鎖
    result += temp;
}

void efficientWork(int input) {
    // 不需要鎖的部分
    int temp = input * input;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    temp += input;
    
    // 只鎖必要的部分
    {
        std::lock_guard<std::mutex> lock(mtx);
        result += temp;  // ✓ 最小化臨界區段
    }
}

int main() {
    auto start = std::chrono::steady_clock::now();
    
    std::thread t1(inefficientWork, 10);
    std::thread t2(inefficientWork, 20);
    t1.join();
    t2.join();
    
    auto mid = std::chrono::steady_clock::now();
    
    result = 0;
    
    std::thread t3(efficientWork, 10);
    std::thread t4(efficientWork, 20);
    t3.join();
    t4.join();
    
    auto end = std::chrono::steady_clock::now();
    
    auto inefficient_time = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    auto efficient_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid);
    
    std::cout << "低效版本：" << inefficient_time.count() << " ms" << std::endl;
    std::cout << "高效版本：" << efficient_time.count() << " ms" << std::endl;
    
    return 0;
}
