#include <iostream>
#include <iomanip>

int main() {
    std::cout << "=== 欄位寬度與對齊 ===" << std::endl;
    
    // setw 只對下一個輸出有效
    std::cout << "[" << std::setw(10) << 42 << "]" << std::endl;
    std::cout << "[" << std::setw(10) << "Hello" << "]" << std::endl;
    
    // 靠左對齊
    std::cout << "[" << std::left << std::setw(10) << 42 << "]" << std::endl;
    std::cout << "[" << std::left << std::setw(10) << "Hello" << "]" << std::endl;
    
    // 使用填充字元
    std::cout << "[" << std::right << std::setfill('0') << std::setw(6) 
              << 42 << "]" << std::endl;
    
    // 恢復預設填充字元
    std::cout << std::setfill(' ');
    
    return 0;
}
