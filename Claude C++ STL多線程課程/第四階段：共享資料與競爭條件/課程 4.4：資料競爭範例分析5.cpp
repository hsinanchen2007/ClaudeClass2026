#include <iostream>
#include <thread>

class Singleton {
    static Singleton* instance;
    
public:
    // 危險！雙重檢查鎖定的錯誤實作
    static Singleton* getInstance() {
        if (instance == nullptr) {          // 第一次檢查
            // ← 多執行緒可能同時通過這裡！
            instance = new Singleton();     // 可能建立多個實例
        }
        return instance;
    }
};

Singleton* Singleton::instance = nullptr;

int main() {
    std::thread t1([]{ Singleton::getInstance(); });
    std::thread t2([]{ Singleton::getInstance(); });
    
    t1.join();
    t2.join();
    
    return 0;
}