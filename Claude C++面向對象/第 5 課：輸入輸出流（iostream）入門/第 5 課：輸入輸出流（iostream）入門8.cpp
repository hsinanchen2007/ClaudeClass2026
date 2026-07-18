#include <iostream>
#include <string>

int main() {
    std::string word;
    
    std::cout << "請輸入一段文字: ";
    std::cin >> word;
    
    std::cout << "你輸入的是: [" << word << "]" << std::endl;
    
    return 0;
}
