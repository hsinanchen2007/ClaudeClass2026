#include <iostream>
#include <string>

int main() {
    std::string name;
    int age;
    double height;
    
    std::cout << "請輸入姓名: ";
    std::cin >> name;
    
    std::cout << "請輸入年齡: ";
    std::cin >> age;
    
    std::cout << "請輸入身高(cm): ";
    std::cin >> height;
    
    std::cout << "\n=== 個人資料 ===" << std::endl;
    std::cout << "姓名: " << name << std::endl;
    std::cout << "年齡: " << age << " 歲" << std::endl;
    std::cout << "身高: " << height << " cm" << std::endl;
    
    return 0;
}
