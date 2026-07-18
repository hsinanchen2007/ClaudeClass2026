#include <iostream>
#include <thread>
#include <stdexcept>

void worker() {
    throw std::runtime_error("執行緒內的錯誤！");
}

int main() {
    std::thread t(worker);
    
    try {
        t.join();
    } catch (const std::exception& e) {
        // 這裡捕獲不到！
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
