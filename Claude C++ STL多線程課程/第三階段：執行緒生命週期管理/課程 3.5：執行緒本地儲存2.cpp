#include <iostream>
#include <thread>

int globalCounter = 0;              // 共享
thread_local int localCounter = 0;  // 各自獨立

void work(const std::string& name) {
    ++globalCounter;
    ++localCounter;
    
    std::cout << name 
              << " global=" << globalCounter 
              << " local=" << localCounter << std::endl;
}

int main() {
    std::thread t1(work, "A");
    std::thread t2(work, "B");
    std::thread t3(work, "C");
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}
