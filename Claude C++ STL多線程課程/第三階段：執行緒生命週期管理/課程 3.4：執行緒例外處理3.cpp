#include <iostream>
#include <thread>
#include <stdexcept>
#include <exception>

std::exception_ptr threadException = nullptr;

void worker() {
    try {
        throw std::runtime_error("執行緒內的錯誤！");
    } catch (...) {
        // 捕獲並儲存例外
        threadException = std::current_exception();
    }
}

int main() {
    std::thread t(worker);
    t.join();
    
    // 檢查是否有例外
    if (threadException) {
        try {
            std::rethrow_exception(threadException);
        } catch (const std::exception& e) {
            std::cout << "主執行緒捕獲: " << e.what() << std::endl;
        }
    }
    
    return 0;
}
