#include <iostream>
#include <iomanip>

int main() {
    int num = 255;
    
    std::cout << "十進位: " << std::dec << num << std::endl;
    std::cout << "十六進位: " << std::hex << num << std::endl;
    std::cout << "八進位: " << std::oct << num << std::endl;
    
    // 顯示進位前綴
    std::cout << std::showbase;
    std::cout << "十進位: " << std::dec << num << std::endl;
    std::cout << "十六進位: " << std::hex << num << std::endl;
    std::cout << "八進位: " << std::oct << num << std::endl;
    
    // 恢復預設
    std::cout << std::dec << std::noshowbase;
    
    return 0;
}
