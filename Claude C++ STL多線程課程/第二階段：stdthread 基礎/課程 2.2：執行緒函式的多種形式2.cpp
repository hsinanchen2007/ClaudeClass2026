#include <iostream>
#include <thread>

int main() {
    // 無捕獲的 Lambda
    std::thread t1([]() {
        std::cout << "Lambda 1" << std::endl;
    });
    
    // 有捕獲的 Lambda
    int value = 42;
    std::thread t2([value]() {
        std::cout << "Value: " << value << std::endl;
    });
    
    t1.join();
    t2.join();
    return 0;
}
