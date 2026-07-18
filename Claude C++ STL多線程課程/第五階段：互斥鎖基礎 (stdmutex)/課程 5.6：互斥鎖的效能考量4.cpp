// 檔案：lesson_5_6_minimize_critical_section.cpp
// 說明：減少臨界區段大小提升效能

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <cmath>
#include <iomanip>

std::mutex mtx;
double result = 0;

// 耗時計算（模擬複雜運算）
double expensiveComputation(int input) {
    double sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += std::sin(input + i) * std::cos(input - i);
    }
    return sum;
}

// 差的做法：在鎖內進行計算
void badApproach(int start, int count) {
    for (int i = start; i < start + count; ++i) {
        std::lock_guard<std::mutex> lock(mtx);  // 鎖定
        double computed = expensiveComputation(i);  // 💀 在鎖內計算！
        result += computed;
    }  // 解鎖
}

// 好的做法：在鎖外進行計算
void goodApproach(int start, int count) {
    for (int i = start; i < start + count; ++i) {
        double computed = expensiveComputation(i);  // ✓ 在鎖外計算
        
        std::lock_guard<std::mutex> lock(mtx);  // 鎖定
        result += computed;  // 只鎖定必要的部分
    }  // 解鎖
}

// 更好的做法：批次累加
void betterApproach(int start, int count) {
    double localSum = 0;  // 本地累加
    
    for (int i = start; i < start + count; ++i) {
        localSum += expensiveComputation(i);  // 完全不需要鎖
    }
    
    std::lock_guard<std::mutex> lock(mtx);  // 只鎖定一次
    result += localSum;
}

template<typename Func>
double runTest(Func&& func, int numThreads, int totalWork) {
    result = 0;
    int workPerThread = totalWork / numThreads;
    
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(func, i * workPerThread, workPerThread);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    const int totalWork = 1000;
    const int numThreads = 4;
    
    std::cout << "=== 臨界區段大小對效能的影響 ===" << std::endl;
    std::cout << "執行緒數：" << numThreads << std::endl;
    std::cout << "總工作量：" << totalWork << std::endl << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    
    double badTime = runTest(badApproach, numThreads, totalWork);
    std::cout << "差的做法（鎖內計算）：" << badTime << " ms" << std::endl;
    
    double goodTime = runTest(goodApproach, numThreads, totalWork);
    std::cout << "好的做法（鎖外計算）：" << goodTime << " ms" << std::endl;
    
    double betterTime = runTest(betterApproach, numThreads, totalWork);
    std::cout << "更好做法（批次累加）：" << betterTime << " ms" << std::endl;
    
    std::cout << std::endl;
    std::cout << "加速比（差 vs 好）：" << badTime / goodTime << "x" << std::endl;
    std::cout << "加速比（差 vs 更好）：" << badTime / betterTime << "x" << std::endl;
    
    return 0;
}
