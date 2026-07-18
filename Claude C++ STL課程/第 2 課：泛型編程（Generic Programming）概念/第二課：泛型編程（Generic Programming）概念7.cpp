#include <iostream>
#include <vector>
#include <string>

int main() {
    // vector<int> 是一個型別
    // vector<double> 是另一個型別
    // 它們是從同一個 template 實例化出來的不同類別
    
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::vector<std::string> words = {"Hello", "World"};
    
    std::cout << "numbers[0] = " << numbers[0] << std::endl;
    std::cout << "words[0] = " << words[0] << std::endl;
    
    return 0;
}
