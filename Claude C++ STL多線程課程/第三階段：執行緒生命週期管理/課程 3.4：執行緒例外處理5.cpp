#include <iostream>
#include <future>
#include <stdexcept>

int worker() {
    throw std::runtime_error("非同步任務的錯誤！");
    return 42;
}

int main() {
    std::future<int> result = std::async(std::launch::async, worker);
    
    try {
        int value = result.get();  // 例外在這裡被重新拋出
        std::cout << "結果: " << value << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
