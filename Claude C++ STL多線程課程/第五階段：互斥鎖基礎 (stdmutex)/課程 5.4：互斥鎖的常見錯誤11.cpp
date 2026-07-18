// 檔案：lesson_5_4_lock_order.cpp
// 說明：鎖定順序不一致導致死結

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mutexA;
std::mutex mutexB;

void thread1() {
    mutexA.lock();
    std::cout << "Thread 1: locked A" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    mutexB.lock();  // 💀 等待 Thread 2 釋放 B
    std::cout << "Thread 1: locked B" << std::endl;
    
    mutexB.unlock();
    mutexA.unlock();
}

void thread2() {
    mutexB.lock();  // ← 順序相反！
    std::cout << "Thread 2: locked B" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    mutexA.lock();  // 💀 等待 Thread 1 釋放 A
    std::cout << "Thread 2: locked A" << std::endl;
    
    mutexA.unlock();
    mutexB.unlock();
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);
    
    t1.join();
    t2.join();
    
    // 💀 程式永遠不會到達這裡
    std::cout << "完成" << std::endl;
    
    return 0;
}
