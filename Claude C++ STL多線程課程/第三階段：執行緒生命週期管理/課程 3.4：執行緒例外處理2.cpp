#include <iostream>
#include <thread>
#include <stdexcept>

void worker() {
    try {
        throw std::runtime_error("執行緒內的錯誤！");
    } catch (const std::exception& e) {
        std::cout << "執行緒內捕獲: " << e.what() << std::endl;
    }
}

int main() {
    std::thread t(worker);
    t.join();
    
    std::cout << "程式正常結束" << std::endl;
    return 0;
}
