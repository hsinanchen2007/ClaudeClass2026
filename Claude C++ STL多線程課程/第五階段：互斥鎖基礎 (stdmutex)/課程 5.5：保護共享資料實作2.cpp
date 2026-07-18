// 檔案：lesson_5_5_copyable_counter.cpp
// 說明：可複製的執行緒安全計數器（明確定義複製行為）

#include <iostream>
#include <mutex>

class CopyableCounter {
private:
    mutable std::mutex mtx;
    int count = 0;
    
public:
    CopyableCounter() = default;
    
    // 明確定義複製建構函式
    CopyableCounter(const CopyableCounter& other) {
        std::lock_guard<std::mutex> lock(other.mtx);  // 鎖定來源
        count = other.count;  // 複製值
        // 新物件有自己的 mutex（預設建構）
    }
    
    // 明確定義複製賦值運算子
    CopyableCounter& operator=(const CopyableCounter& other) {
        if (this != &other) {
            // 需要同時鎖定兩個物件，注意死結！
            std::scoped_lock lock(mtx, other.mtx);
            count = other.count;
        }
        return *this;
    }
    
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        ++count;
    }
    
    int get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return count;
    }
};

int main() {
    CopyableCounter c1;
    c1.increment();
    c1.increment();
    
    CopyableCounter c2 = c1;  // 複製
    
    std::cout << "c1 = " << c1.get() << std::endl;  // 2
    std::cout << "c2 = " << c2.get() << std::endl;  // 2
    
    c1.increment();
    
    std::cout << "c1 = " << c1.get() << std::endl;  // 3
    std::cout << "c2 = " << c2.get() << std::endl;  // 2（獨立）
    
    return 0;
}
