#include <iostream>
#include <thread>

class Singleton {
    Singleton() { std::cout << "Singleton 建立" << std::endl; }
    
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
    static Singleton& getInstance() {
        static Singleton instance;  // C++11 保證執行緒安全
        return instance;
    }
    
    void doSomething() { std::cout << "工作中" << std::endl; }
};

int main() {
    std::thread t1([]() { Singleton::getInstance().doSomething(); });
    std::thread t2([]() { Singleton::getInstance().doSomething(); });
    
    t1.join();
    t2.join();
    
    return 0;
}
