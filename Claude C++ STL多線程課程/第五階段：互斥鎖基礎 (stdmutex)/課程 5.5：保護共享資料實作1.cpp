// 檔案：lesson_5_5_safe_counter.cpp
// 說明：執行緒安全的計數器類別

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

class ThreadSafeCounter {
private:
    mutable std::mutex mtx;  // mutable：允許在 const 方法中使用
    int count = 0;
    
public:
    // 建構函式
    ThreadSafeCounter() = default;
    
    // 禁止複製（互斥鎖不可複製）
    ThreadSafeCounter(const ThreadSafeCounter&) = delete;
    ThreadSafeCounter& operator=(const ThreadSafeCounter&) = delete;
    
    // 遞增
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        ++count;
    }
    
    // 遞減
    void decrement() {
        std::lock_guard<std::mutex> lock(mtx);
        --count;
    }
    
    // 增加指定值
    void add(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        count += value;
    }
    
    // 獲取當前值
    int get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return count;
    }
    
    // 重設並返回舊值（原子操作）
    int reset() {
        std::lock_guard<std::mutex> lock(mtx);
        int oldValue = count;
        count = 0;
        return oldValue;
    }
    
    // 條件遞增：只有當前值小於 max 時才遞增
    bool incrementIfLessThan(int max) {
        std::lock_guard<std::mutex> lock(mtx);
        if (count < max) {
            ++count;
            return true;
        }
        return false;
    }
};

// 測試
void workerIncrement(ThreadSafeCounter& counter, int times) {
    for (int i = 0; i < times; ++i) {
        counter.increment();
    }
}

int main() {
    ThreadSafeCounter counter;
    
    const int numThreads = 4;
    const int incrementsPerThread = 100000;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(workerIncrement, std::ref(counter), incrementsPerThread);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int expected = numThreads * incrementsPerThread;
    int actual = counter.get();
    
    std::cout << "預期值：" << expected << std::endl;
    std::cout << "實際值：" << actual << std::endl;
    std::cout << "結果：" << (expected == actual ? "✓ 正確" : "✗ 錯誤") << std::endl;
    
    return 0;
}
