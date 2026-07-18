#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

std::once_flag flag;
std::atomic<int> attempt{0};

void mayFail() {
    int current = ++attempt;
    std::cout << "嘗試 #" << current << std::endl;
    
    if (current < 3) {
        throw std::runtime_error("初始化失敗");
    }
    
    std::cout << "初始化成功！" << std::endl;
}

void worker() {
    try {
        std::call_once(flag, mayFail);
        std::cout << "繼續執行" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕獲例外: " << e.what() << std::endl;
    }
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    std::thread t4(worker);
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    return 0;
}
