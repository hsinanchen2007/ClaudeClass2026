#include <iostream>
#include <string>

int main() {
    std::string name;
    int age;
    
    std::cout << "請輸入你的名字: ";
    std::cin >> name;
    
    std::cout << "請輸入你的年齡: ";
    std::cin >> age;
    
    std::cout << "你好 " << name << "，你今年 " << age << " 歲" << std::endl;
    
    return 0;
}
