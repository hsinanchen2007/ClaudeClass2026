#include <vector>
#include <iostream>
#include <stdexcept>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 正常存取
    std::cout << "v.at(0) = " << v.at(0) << std::endl;  // 10
    std::cout << "v.at(2) = " << v.at(2) << std::endl;  // 30
    
    // 越界存取會拋出例外
    try {
        std::cout << v.at(10) << std::endl;
    }
    catch (const std::out_of_range& e) {
        std::cout << "捕捉到例外: " << e.what() << std::endl;
    }
    
    return 0;
}
