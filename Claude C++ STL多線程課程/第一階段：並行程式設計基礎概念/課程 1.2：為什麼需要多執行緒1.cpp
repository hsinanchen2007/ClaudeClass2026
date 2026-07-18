// 檔案名稱：lesson_1_2_performance.cpp
// 課程：1.2 - 為什麼需要多執行緒
// 說明：展示多執行緒在運算密集型任務中的效能提升

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <cmath>
#include <iomanip>

// 模擬一個運算密集型的任務：計算範圍內所有質數的數量
bool isPrime(long long n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (long long i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// 計算指定範圍內的質數數量（單執行緒版本）
long long countPrimesInRange(long long start, long long end) {
    long long count = 0;
    for (long long i = start; i <= end; ++i) {
        if (isPrime(i)) {
            ++count;
        }
    }
    return count;
}

// 工作執行緒函式：計算指定範圍內的質數數量，並將結果存入指定位置
void countPrimesWorker(long long start, long long end, long long& result) {
    result = countPrimesInRange(start, end);
}

int main() {
    const long long RANGE_START = 1;
    const long long RANGE_END = 1000000;  // 計算 1 到 100 萬之間的質數
    
    std::cout << "========================================" << std::endl;
    std::cout << "    多執行緒效能提升示範" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "計算範圍: " << RANGE_START << " 到 " << RANGE_END << std::endl;
    
    unsigned int numCores = std::thread::hardware_concurrency();
    std::cout << "可用 CPU 核心數: " << numCores << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // ========== 方式一：單執行緒計算 ==========
    std::cout << "\n【方式一】單執行緒計算：" << std::endl;
    
    auto singleStart = std::chrono::steady_clock::now();
    long long singleResult = countPrimesInRange(RANGE_START, RANGE_END);
    auto singleEnd = std::chrono::steady_clock::now();
    
    auto singleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        singleEnd - singleStart).count();
    
    std::cout << "找到 " << singleResult << " 個質數" << std::endl;
    std::cout << "執行時間: " << singleDuration << " 毫秒" << std::endl;

    // ========== 方式二：多執行緒計算 ==========
    std::cout << "\n【方式二】多執行緒計算（" << numCores << " 個執行緒）：" << std::endl;
    
    auto multiStart = std::chrono::steady_clock::now();
    
    // 準備儲存每個執行緒結果的容器
    std::vector<std::thread> threads;
    std::vector<long long> results(numCores, 0);
    
    // 計算每個執行緒負責的範圍
    long long rangeSize = (RANGE_END - RANGE_START + 1) / numCores;
    
    // 建立並啟動所有執行緒
    for (unsigned int i = 0; i < numCores; ++i) {
        long long threadStart = RANGE_START + i * rangeSize;
        long long threadEnd;
        
        // 最後一個執行緒處理剩餘的所有數字
        if (i == numCores - 1) {
            threadEnd = RANGE_END;
        } else {
            threadEnd = threadStart + rangeSize - 1;
        }
        
        std::cout << "  執行緒 " << i << ": 處理範圍 [" 
                  << threadStart << ", " << threadEnd << "]" << std::endl;
        
        // 建立執行緒，使用 std::ref 傳遞引用
        threads.emplace_back(countPrimesWorker, threadStart, threadEnd, 
                            std::ref(results[i]));
    }
    
    // 等待所有執行緒完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 彙總所有執行緒的結果
    long long multiResult = 0;
    for (unsigned int i = 0; i < numCores; ++i) {
        std::cout << "  執行緒 " << i << " 找到: " << results[i] << " 個質數" << std::endl;
        multiResult += results[i];
    }
    
    auto multiEnd = std::chrono::steady_clock::now();
    auto multiDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        multiEnd - multiStart).count();
    
    std::cout << "總共找到 " << multiResult << " 個質數" << std::endl;
    std::cout << "執行時間: " << multiDuration << " 毫秒" << std::endl;

    // ========== 效能比較 ==========
    std::cout << "\n========================================" << std::endl;
    std::cout << "效能比較結果：" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "單執行緒時間: " << singleDuration << " 毫秒" << std::endl;
    std::cout << "多執行緒時間: " << multiDuration << " 毫秒" << std::endl;
    
    double speedup = static_cast<double>(singleDuration) / multiDuration;
    double efficiency = speedup / numCores * 100;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "加速比 (Speedup): " << speedup << " 倍" << std::endl;
    std::cout << "並行效率: " << efficiency << "%" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 驗證結果一致性
    if (singleResult == multiResult) {
        std::cout << "✓ 結果驗證通過：兩種方式得到相同結果" << std::endl;
    } else {
        std::cout << "✗ 結果驗證失敗：結果不一致！" << std::endl;
    }

    return 0;
}
