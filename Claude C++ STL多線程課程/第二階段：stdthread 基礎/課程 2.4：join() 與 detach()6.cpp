#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {});
    
    t.join();
    t.join();  // 崩潰！std::system_error
    
    return 0;
}
