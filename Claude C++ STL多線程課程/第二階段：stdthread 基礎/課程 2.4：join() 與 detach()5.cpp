#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {
        std::cout << "Task" << std::endl;
    });
    
    // 安全的寫法
    if (t.joinable()) {
        t.join();
    }
    
    // 再次呼叫是安全的（因為有檢查）
    if (t.joinable()) {
        t.join();  // 不會執行
    }
    
    return 0;
}
