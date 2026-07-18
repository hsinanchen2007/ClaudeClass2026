// 檔案：lesson_5_4_recursive_mutex.cpp
// 說明：使用遞迴互斥鎖解決重複鎖定

#include <iostream>
#include <mutex>

std::recursive_mutex rmtx;  // ✓ 遞迴互斥鎖

void innerFunction() {
    rmtx.lock();  // ✓ 同一執行緒可以再次鎖定
    std::cout << "Inner function" << std::endl;
    rmtx.unlock();
}

void outerFunction() {
    rmtx.lock();
    std::cout << "Outer function" << std::endl;
    
    innerFunction();  // ✓ 正常運作
    
    rmtx.unlock();
}

int main() {
    outerFunction();  // ✓ 正常執行
    std::cout << "完成" << std::endl;
    return 0;
}
