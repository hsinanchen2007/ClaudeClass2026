// 檔案：lesson_1_4_race_condition.cpp

#include <iostream>
#include <thread>

int counter = 0;

void incrementManyTimes() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;  // 這不是原子操作！
    }
}

int main() {
    std::thread t1(incrementManyTimes);
    std::thread t2(incrementManyTimes);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;
    
    return 0;
}
