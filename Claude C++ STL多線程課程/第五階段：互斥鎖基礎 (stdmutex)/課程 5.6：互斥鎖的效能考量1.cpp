// 檔案：lesson_5_6_lock_overhead.cpp
// 說明：測量互斥鎖的基本開銷

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <iomanip>

std::mutex mtx;

// 無鎖操作
void noLockOperation(long long& counter, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        ++counter;
    }
}

// 有鎖操作
void withLockOperation(long long& counter, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        ++counter;
    }
}

// 計時函式
template<typename Func>
double measureTime(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count();
}

int main() {
    const int iterations = 1000000;
    long long counter = 0;
    
    std::cout << "=== 互斥鎖開銷測量 ===" << std::endl;
    std::cout << "迭代次數：" << iterations << std::endl << std::endl;
    
    // 測量無鎖操作
    counter = 0;
    double noLockTime = measureTime([&]() {
        noLockOperation(counter, iterations);
    });
    
    // 測量有鎖操作（單執行緒，無競爭）
    counter = 0;
    double withLockTime = measureTime([&]() {
        withLockOperation(counter, iterations);
    });
    
    double lockOverhead = (withLockTime - noLockTime) / iterations;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "無鎖操作總時間：" << noLockTime / 1e6 << " ms" << std::endl;
    std::cout << "有鎖操作總時間：" << withLockTime / 1e6 << " ms" << std::endl;
    std::cout << "每次鎖操作開銷：" << lockOverhead << " ns" << std::endl;
    std::cout << "效能比率：" << withLockTime / noLockTime << "x 慢" << std::endl;
    
    return 0;
}
