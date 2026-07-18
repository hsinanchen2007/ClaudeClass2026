#include <iostream>
#include <unordered_map>
#include <string>

int main() {
    std::cout << "=== std::unordered_map ===" << std::endl;
    
    std::unordered_map<std::string, int> ages;
    
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 35;
    ages["Diana"] = 28;
    
    // 順序不固定
    std::cout << "所有人:" << std::endl;
    for (const auto& pair : ages) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    
    // 查找：平均 O(1)
    auto it = ages.find("Bob");
    if (it != ages.end()) {
        std::cout << "Bob 的年齡: " << it->second << std::endl;
    }
    
    return 0;
}
