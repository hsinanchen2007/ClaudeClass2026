// 檔案：lesson_5_6_lock_granularity.cpp
// 說明：比較粗粒度鎖與細粒度鎖的效能

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <array>
#include <iomanip>

const int NUM_BUCKETS = 16;
const int ITERATIONS = 100000;

// 粗粒度鎖：一個鎖保護所有資料
class CoarseGrainedCounters {
private:
    std::mutex mtx;
    std::array<long long, NUM_BUCKETS> counters{};
    
public:
    void increment(int bucket) {
        std::lock_guard<std::mutex> lock(mtx);  // 鎖整個結構
        ++counters[bucket % NUM_BUCKETS];
    }
    
    long long get(int bucket) const {
        return counters[bucket % NUM_BUCKETS];
    }
    
    long long total() const {
        long long sum = 0;
        for (auto c : counters) sum += c;
        return sum;
    }
};

// 細粒度鎖：每個桶一個鎖
class FineGrainedCounters {
private:
    std::array<std::mutex, NUM_BUCKETS> mutexes;
    std::array<long long, NUM_BUCKETS> counters{};
    
public:
    void increment(int bucket) {
        int idx = bucket % NUM_BUCKETS;
        std::lock_guard<std::mutex> lock(mutexes[idx]);  // 只鎖一個桶
        ++counters[idx];
    }
    
    long long get(int bucket) const {
        return counters[bucket % NUM_BUCKETS];
    }
    
    long long total() const {
        long long sum = 0;
        for (auto c : counters) sum += c;
        return sum;
    }
};

template<typename Counter>
void worker(Counter& counter, int threadId, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        // 每個執行緒存取不同的桶（模擬真實場景）
        counter.increment(threadId * 1000 + i);
    }
}

template<typename Counter>
double runTest(int numThreads) {
    Counter counter;
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker<Counter>, std::ref(counter), i, ITERATIONS);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    std::cout << "=== 鎖粒度效能比較 ===" << std::endl;
    std::cout << "桶數量：" << NUM_BUCKETS << std::endl;
    std::cout << "每執行緒迭代：" << ITERATIONS << std::endl << std::endl;
    
    std::cout << std::setw(10) << "執行緒數"
              << std::setw(18) << "粗粒度 (ms)"
              << std::setw(18) << "細粒度 (ms)"
              << std::setw(12) << "加速比"
              << std::endl;
    std::cout << std::string(58, '-') << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    
    for (int numThreads : {1, 2, 4, 8, 16}) {
        double coarseTime = runTest<CoarseGrainedCounters>(numThreads);
        double fineTime = runTest<FineGrainedCounters>(numThreads);
        double speedup = coarseTime / fineTime;
        
        std::cout << std::setw(10) << numThreads
                  << std::setw(18) << coarseTime
                  << std::setw(18) << fineTime
                  << std::setw(11) << speedup << "x"
                  << std::endl;
    }
    
    return 0;
}
