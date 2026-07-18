#include <iostream>
#include <thread>

void takeOwnership(std::thread t) {
    std::cout << "取得執行緒所有權" << std::endl;
    t.join();
}

int main() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });
    
    takeOwnership(std::move(t));  // 必須 move
    
    // t 現在是空的
    std::cout << "t joinable: " << t.joinable() << std::endl;
    
    return 0;
}
