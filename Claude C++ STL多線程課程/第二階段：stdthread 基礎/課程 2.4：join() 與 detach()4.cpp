#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {
        std::cout << "Working" << std::endl;
    });
    
    std::cout << "joinable: " << t.joinable() << std::endl;  // true
    
    t.join();
    
    std::cout << "joinable: " << t.joinable() << std::endl;  // false
    
    return 0;
}
