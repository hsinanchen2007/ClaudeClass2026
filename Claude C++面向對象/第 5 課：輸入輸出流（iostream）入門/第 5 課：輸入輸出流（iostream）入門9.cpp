#include <iostream>
#include <string>

int main() {
    std::string fullLine;
    
    std::cout << "請輸入一段文字: ";
    std::getline(std::cin, fullLine);
    
    std::cout << "你輸入的是: [" << fullLine << "]" << std::endl;
    
    return 0;
}
