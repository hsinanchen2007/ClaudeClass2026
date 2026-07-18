#include <iostream>
#include <thread>

class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread) 
        : t(std::move(thread)) 
    {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }
    
    ~ScopedThread() {
        t.join();  // 一定是 joinable，不用檢查
    }
    
    // 禁止複製
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
};

int main() {
    ScopedThread st(std::thread([]() {
        std::cout << "安全的執行緒" << std::endl;
    }));
    
    // 自動管理，不需要手動 join
    return 0;
}
