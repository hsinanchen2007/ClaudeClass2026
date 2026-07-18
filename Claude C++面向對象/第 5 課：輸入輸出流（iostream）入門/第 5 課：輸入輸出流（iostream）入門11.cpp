#include <iostream>
#include <string>
#include <limits>

int main() {
    int age;
    std::string name;
    
    std::cout << "請輸入年齡: ";
    std::cin >> age;
    
    // 解決方案一：忽略緩衝區中剩餘的字元直到換行
    // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略直到換行符的所有字元
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    // 解決方案二（簡化版）：忽略一個字元
    // std::cin.ignore();  // 忽略前一次輸入留下的換行符
    // std::cin.ignore();
    
    std::cout << "請輸入姓名: ";
    std::getline(std::cin, name);
    
    std::cout << "年齡: " << age << std::endl;
    std::cout << "姓名: [" << name << "]" << std::endl;
    
    return 0;
}
