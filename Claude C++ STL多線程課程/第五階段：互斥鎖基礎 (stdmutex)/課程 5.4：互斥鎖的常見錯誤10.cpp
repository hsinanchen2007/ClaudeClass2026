// 檔案：lesson_5_4_exception_safe.cpp
// 說明：使用 RAII 確保例外安全

#include <iostream>
#include <mutex>
#include <stdexcept>

std::mutex mtx;

void safeOperation(int value) {
    std::lock_guard<std::mutex> lock(mtx);  // ✓ RAII
    
    std::cout << "開始處理 " << value << std::endl;
    
    if (value < 0) {
        throw std::invalid_argument("值不能為負數");
        // ✓ 例外拋出時，lock_guard 解構，自動 unlock
    }
    
    std::cout << "處理完成" << std::endl;
}  // ✓ 正常返回時也會自動 unlock

int main() {
    try {
        safeOperation(10);
        safeOperation(-5);  // 拋出例外
    } catch (const std::exception& e) {
        std::cout << "捕獲例外：" << e.what() << std::endl;
    }
    
    // ✓ 鎖已被正確釋放
    std::cout << "嘗試再次操作..." << std::endl;
    safeOperation(20);  // ✓ 正常執行
    
    return 0;
}
