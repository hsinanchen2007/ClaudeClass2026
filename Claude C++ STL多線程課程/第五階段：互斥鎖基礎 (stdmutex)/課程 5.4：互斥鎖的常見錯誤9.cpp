// 檔案：lesson_5_4_exception_unsafe.cpp
// 說明：例外導致 unlock 未執行

#include <iostream>
#include <mutex>
#include <stdexcept>

std::mutex mtx;

void riskyOperation(int value) {
    mtx.lock();
    
    std::cout << "開始處理 " << value << std::endl;
    
    if (value < 0) {
        throw std::invalid_argument("值不能為負數");  // 💀 例外拋出
        // unlock() 永遠不會執行！
    }
    
    std::cout << "處理完成" << std::endl;
    mtx.unlock();
}

int main() {
    try {
        riskyOperation(10);   // OK
        riskyOperation(-5);   // 💀 拋出例外，鎖沒釋放
    } catch (const std::exception& e) {
        std::cout << "捕獲例外：" << e.what() << std::endl;
    }
    
    // 此時 mtx 仍處於鎖定狀態！
    std::cout << "嘗試再次操作..." << std::endl;
    riskyOperation(20);  // 💀 死結！
    
    return 0;
}
