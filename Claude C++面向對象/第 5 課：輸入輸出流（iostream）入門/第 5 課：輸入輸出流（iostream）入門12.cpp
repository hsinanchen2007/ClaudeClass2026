#include <iostream>
#include <limits>

int main() {
    int number;
    
    std::cout << "請輸入一個整數: ";
    std::cin >> number;
    
    // 檢查輸入是否成功, 如果輸入失敗，則清除錯誤狀態並忽略錯誤的輸入
    if (std::cin.fail()) {
        std::cout << "輸入錯誤！不是有效的整數。" << std::endl;
        
        // 清除錯誤狀態, 讓 cin 可以繼續使用
        std::cin.clear();
        
        // 忽略錯誤的輸入, 直到換行符為止，確保輸入緩衝區被清空
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    } else {
        std::cout << "你輸入的是: " << number << std::endl;
    }
    
    return 0;
}
