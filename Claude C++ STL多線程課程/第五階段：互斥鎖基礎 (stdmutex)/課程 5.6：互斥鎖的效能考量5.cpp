// 檔案：lesson_5_6_avoid_io_in_lock.cpp
// 說明：避免在臨界區段內進行 I/O

#include <iostream>
#include <thread>
#include <mutex>
#include <sstream>
#include <vector>

std::mutex mtx;
std::vector<std::string> logs;

// 💀 差的做法：在鎖內進行 I/O
void badLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << message << std::endl;  // 💀 I/O 在鎖內！
}

// ✓ 好的做法：只保護共享資料
void goodLog(const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        logs.push_back(message);  // ✓ 只保護共享容器
    }
    // I/O 在鎖外（或由專門的執行緒處理）
}

// ✓ 更好的做法：批次輸出
void flushLogs() {
    std::vector<std::string> localLogs;
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        localLogs.swap(logs);  // 快速交換
    }
    
    // 在鎖外進行 I/O
    for (const auto& log : localLogs) {
        std::cout << log << std::endl;
    }
}
int main() {
    const int numThreads = 4;
    const int logsPerThread = 5;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([t, logsPerThread]() {
            for (int i = 0; i < logsPerThread; ++i) {
                std::ostringstream oss;
                oss << "Thread " << t << " - Log " << i;
                
                //badLog(oss.str());  // 差的做法
                goodLog(oss.str());   // 好的做法
            }
        });
    }
    
    for (auto& th : threads) {
        th.join();
    }
    
    flushLogs();  // 輸出所有日誌
    
    return 0;
}