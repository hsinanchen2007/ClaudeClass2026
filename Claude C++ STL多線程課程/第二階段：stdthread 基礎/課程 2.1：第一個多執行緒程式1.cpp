// 檔案：lesson_2_1_first_thread.cpp

#include <iostream>
#include <thread>

void sayHello() {
    std::cout << "Hello from thread!" << std::endl;
}

int main() {
    std::thread t(sayHello);  // 建立並啟動執行緒
    t.join();                 // 等待執行緒結束
    
    std::cout << "Back in main" << std::endl;
    return 0;
}
