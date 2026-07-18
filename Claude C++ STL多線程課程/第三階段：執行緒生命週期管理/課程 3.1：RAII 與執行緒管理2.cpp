#include <iostream>
#include <thread>

class ThreadGuard {
    std::thread& t;
public:
    explicit ThreadGuard(std::thread& thread) : t(thread) {}
    
    ~ThreadGuard() {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // 禁止複製
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

int main() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });
    
    ThreadGuard guard(t);  // 保證 t 會被 join
    
    // 即使這裡拋出例外，guard 的解構函式仍會執行
    // throw std::runtime_error("測試");
    
    return 0;
}  // guard 解構 → 自動 join
