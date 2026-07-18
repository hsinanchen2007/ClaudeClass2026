#include <iostream>
#include <thread>
#include <stdexcept>

void riskyFunction() {
    std::jthread jt([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "執行緒完成" << std::endl;
    });
    
    throw std::runtime_error("發生錯誤！");
    
    // 不需要擔心！jt 解構時會自動 join
}

int main() {
    try {
        riskyFunction();
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
