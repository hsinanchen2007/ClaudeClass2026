#include <iostream>
#include <thread>

int counter = 0;

void increment() {
    // 這三步不是原子的！
    // 1. 讀取 counter
    // 2. +1
    // 3. 寫回
    for (int i = 0; i < 10000; ++i) {
        counter++;  // 非原子操作
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    // 預期 20000，實際可能更小
    std::cout << "counter = " << counter << std::endl;
    
    return 0;
}
