#include <iostream>
#include <thread>
#include <mutex>

class Singleton {
    static Singleton* instance;
    static std::once_flag initFlag;
    
    Singleton() { std::cout << "Singleton 建立" << std::endl; }
    
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
    static Singleton& getInstance() {
        std::call_once(initFlag, []() {
            instance = new Singleton();
        });
        return *instance;
    }
    
    void doSomething() { std::cout << "工作中" << std::endl; }
};

Singleton* Singleton::instance = nullptr;
std::once_flag Singleton::initFlag;

int main() {
    std::thread t1([]() { Singleton::getInstance().doSomething(); });
    std::thread t2([]() { Singleton::getInstance().doSomething(); });
    
    t1.join();
    t2.join();
    
    return 0;
}
