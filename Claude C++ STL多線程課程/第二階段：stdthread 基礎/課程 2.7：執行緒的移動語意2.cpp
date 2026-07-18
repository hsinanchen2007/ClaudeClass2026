#include <iostream>
#include <thread>

int main() {
    std::thread t1([]() {
        std::cout << "執行中" << std::endl;
    });
    
    std::cout << "t1 joinable: " << t1.joinable() << std::endl;
    
    // 移動所有權
    std::thread t2 = std::move(t1);
    
    std::cout << "移動後:" << std::endl;
    std::cout << "t1 joinable: " << t1.joinable() << std::endl;
    std::cout << "t2 joinable: " << t2.joinable() << std::endl;
    
    t2.join();  // 由 t2 負責 join
    return 0;
}
