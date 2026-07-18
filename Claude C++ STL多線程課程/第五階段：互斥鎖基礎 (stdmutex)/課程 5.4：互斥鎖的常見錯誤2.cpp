// 檔案：lesson_5_4_early_return.cpp
// 說明：早期返回導致忘記解鎖

#include <iostream>
#include <mutex>

std::mutex mtx;

int getValue(int input) {
    mtx.lock();
    
    if (input < 0) {
        std::cout << "無效輸入" << std::endl;
        return -1;  // 💀 提前返回，沒有 unlock！
    }
    
    if (input == 0) {
        std::cout << "零值" << std::endl;
        return 0;   // 💀 又一個提前返回！
    }
    
    int result = input * 2;
    mtx.unlock();
    return result;
}

int main() {
    std::cout << getValue(10) << std::endl;   // OK
    std::cout << getValue(-5) << std::endl;   // 💀 鎖沒釋放
    std::cout << getValue(20) << std::endl;   // 💀 永遠卡住
    
    return 0;
}
