// 檔案：lesson_5_4_raii_fix.cpp
// 說明：使用 lock_guard 自動管理鎖

#include <iostream>
#include <mutex>

std::mutex mtx;

int getValueSafe(int input) {
    std::lock_guard<std::mutex> lock(mtx);  // ✓ RAII
    
    if (input < 0) {
        std::cout << "無效輸入" << std::endl;
        return -1;  // ✓ lock_guard 解構時自動 unlock
    }
    
    if (input == 0) {
        std::cout << "零值" << std::endl;
        return 0;   // ✓ 同樣會自動 unlock
    }
    
    return input * 2;
}  // ✓ 函式結束，lock_guard 解構，自動 unlock

int main() {
    std::cout << getValueSafe(10) << std::endl;
    std::cout << getValueSafe(-5) << std::endl;
    std::cout << getValueSafe(20) << std::endl;  // ✓ 正常執行
    
    return 0;
}
