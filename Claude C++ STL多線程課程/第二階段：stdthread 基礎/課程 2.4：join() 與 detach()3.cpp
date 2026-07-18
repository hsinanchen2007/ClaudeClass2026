#include <iostream>
#include <thread>
#include <chrono>

void dangerous() {
    int localVar = 42;
    
    std::thread t([&localVar]() {  // 捕獲區域變數的引用
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << localVar << std::endl;  // 危險！localVar 可能已被銷毀
    });
    
    t.detach();
}  // localVar 在這裡被銷毀，但執行緒還在運行

int main() {
    dangerous();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
