#include <iostream>
#include <limits>

int main() {
    int number;
    
    while (true) {
        std::cout << "請輸入一個正整數: ";
        std::cin >> number;
        
        // 檢查輸入是否成功, 如果輸入失敗，則清除錯誤狀態並忽略錯誤的輸入
        if (std::cin.fail()) {
            // 輸入類型錯誤
            std::cout << "錯誤：請輸入數字！" << std::endl;
            std::cin.clear();
            // 忽略錯誤的輸入, 直到換行符為止，確保輸入緩衝區被清空
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else if (number <= 0) {
            // 數值範圍錯誤
            std::cout << "錯誤：必須是正整數！" << std::endl;
        } else {
            // 輸入正確
            break;
        }
    }
    
    std::cout << "你輸入的正整數是: " << number << std::endl;
    
    return 0;
}
