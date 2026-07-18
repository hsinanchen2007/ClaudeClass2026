#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {
        std::cout << "Hello from lambda!" << std::endl;
    });
    
    t.join();
    return 0;
}
