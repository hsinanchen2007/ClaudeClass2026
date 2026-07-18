#include <iostream>
#include <thread>

thread_local int counter = 0;  // 每個執行緒有獨立的 counter

void increment(const std::string& name) {
    for (int i = 0; i < 3; ++i) {
        ++counter;
        std::cout << name << ": counter = " << counter << std::endl;
    }
}

int main() {
    std::thread t1(increment, "Thread A");
    std::thread t2(increment, "Thread B");
    
    t1.join();
    t2.join();
    
    return 0;
}
