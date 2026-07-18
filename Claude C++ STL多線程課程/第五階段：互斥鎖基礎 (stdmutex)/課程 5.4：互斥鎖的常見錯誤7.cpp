// 檔案：lesson_5_4_refactor.cpp
// 說明：透過重構避免重複鎖定

#include <iostream>
#include <mutex>

std::mutex mtx;

// 內部實作（假設鎖已被持有，不加鎖）
void innerFunctionImpl() {
    std::cout << "Inner function" << std::endl;
}

// 公開介面（需要鎖）
void innerFunction() {
    std::lock_guard<std::mutex> lock(mtx);
    innerFunctionImpl();
}

void outerFunction() {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "Outer function" << std::endl;
    
    innerFunctionImpl();  // ✓ 呼叫不加鎖的版本
}

int main() {
    outerFunction();
    std::cout << "完成" << std::endl;
    return 0;
}
