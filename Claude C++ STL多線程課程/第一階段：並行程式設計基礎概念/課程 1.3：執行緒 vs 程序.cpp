// 檔案：lesson_1_3_shared_memory.cpp
// 說明：展示同一程序內的執行緒共享記憶體

#include <iostream>
#include <thread>

int sharedCounter = 0;  // 全域變數，所有執行緒共享

void increment() {
    for (int i = 0; i < 100000; ++i) {
        ++sharedCounter;  // 多個執行緒同時修改同一變數
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    // 預期 200000，但結果通常不是！
    std::cout << "預期結果: 200000" << std::endl;
    std::cout << "實際結果: " << sharedCounter << std::endl;
    
    return 0;
}
