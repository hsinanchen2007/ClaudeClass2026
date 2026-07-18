#include <iostream>
#include <thread>
#include <string>

void printStr(const std::string& s) {
    std::cout << s << std::endl;
}

void danger() {
    char buffer[] = "Hello";
    
    // 危險！buffer 可能在轉換前就被銷毀
    std::thread t(printStr, buffer);
    t.detach();
    
}  // buffer 在這裡被銷毀

void safe() {
    char buffer[] = "Hello";
    
    // 安全：明確轉換為 std::string
    std::thread t(printStr, std::string(buffer));
    t.detach();
}

int main() {
    danger();
    
    // 等待一會兒以確保分離的執行緒有時間執行
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    safe();
    
    // 再次等待
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    return 0;
}