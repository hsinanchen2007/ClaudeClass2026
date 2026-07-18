#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "背景執行緒完成" << std::endl;  // 可能不會印出
    });
    
    t.detach();  // 放手，不等待
    
    std::cout << "主執行緒立即繼續" << std::endl;
    // 主程式結束時，背景執行緒會被強制終止
    return 0;
}
