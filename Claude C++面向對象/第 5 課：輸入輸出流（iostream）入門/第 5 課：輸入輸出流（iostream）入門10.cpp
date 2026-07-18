#include <iostream>
#include <string>

int main() {
    int age;
    std::string name;
    
    std::cout << "請輸入年齡: ";
    std::cin >> age;
    std::cout << "請輸入姓名: ";
    std::getline(std::cin, name);  // 問題！會讀到空字串, 
                                   // 因為前一次輸入年齡後，留下了一個換行符在輸入緩衝區中，getline() 會讀取這個換行符，導致 name 變成空字串。

    // 解決方法：在讀取年齡後，清除輸入緩衝區中的換行符
    //std::cin.ignore();  // 忽略前一次輸入留下的換行符
    //std::getline(std::cin, name);
    
    std::cout << "\n年齡: " << age << std::endl;
    std::cout << "姓名: [" << name << "]" << std::endl;
    
    // 另一種解決方法：先讀取年齡，然後再讀取姓名
    std::cout << "\n請輸入姓名: ";
    std::getline(std::cin, name);

    std::cout << "請輸入年齡: ";
    std::cin >> age;

    std::cout << "年齡: " << age << std::endl;
    std::cout << "姓名: [" << name << "]" << std::endl;

    return 0;
}
    