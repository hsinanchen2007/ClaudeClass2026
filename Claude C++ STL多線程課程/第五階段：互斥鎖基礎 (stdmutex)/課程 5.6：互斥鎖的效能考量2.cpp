// 檔案：lesson_5_6_contention.cpp
// 說明：測量鎖競爭對效能的影響

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <iomanip>

std::mutex mtx;
long long sharedCounter = 0;

void incrementWithLock(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        ++sharedCounter;
    }
}

double runTest(int numThreads, int totalIterations) {
    sharedCounter = 0;
    int iterationsPerThread = totalIterations / numThreads;
    
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(incrementWithLock, iterationsPerThread);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    const int totalIterations = 1000000;
    
    std::cout << "=== 鎖競爭效能測試 ===" << std::endl;
    std::cout << "總迭代次數：" << totalIterations << std::endl << std::endl;
    
    std::cout << std::setw(10) << "執行緒數" 
              << std::setw(15) << "時間 (ms)"
              << std::setw(15) << "相對效能"
              << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    double baseTime = runTest(1, totalIterations);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(10) << 1 
              << std::setw(15) << baseTime
              << std::setw(15) << "1.00x"
              << std::endl;
    
    for (int numThreads : {2, 4, 8, 16}) {
        double time = runTest(numThreads, totalIterations);
        double ratio = time / baseTime;
        
        std::cout << std::setw(10) << numThreads 
                  << std::setw(15) << time
                  << std::setw(14) << ratio << "x"
                  << std::endl;
    }
    
    return 0;
}
