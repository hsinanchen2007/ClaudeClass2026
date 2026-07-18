#include <iostream>

int main() {
    // 正常輸出
    std::cout << "這是正常訊息" << std::endl;
    
    // 錯誤輸出（無緩衝，立即顯示）
    std::cerr << "這是錯誤訊息" << std::endl;
    
    // 日誌輸出（有緩衝）
    std::clog << "這是日誌訊息" << std::endl;
    
    return 0;
}
