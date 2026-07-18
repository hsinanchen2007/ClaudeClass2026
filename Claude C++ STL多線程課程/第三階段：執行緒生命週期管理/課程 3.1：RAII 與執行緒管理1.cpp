#include <iostream>
#include <thread>

void riskyFunction() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });
    
    // 如果這裡拋出例外...
    throw std::runtime_error("發生錯誤！");
    
    t.join();  // 永遠不會執行！
}  // t 解構時仍是 joinable → std::terminate()

int main() {
    try {
        riskyFunction();
    } catch (...) {
        std::cout << "捕獲例外" << std::endl;
    }
    return 0;
}
