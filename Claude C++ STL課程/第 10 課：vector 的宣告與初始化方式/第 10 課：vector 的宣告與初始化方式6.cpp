#include <vector>
#include <iostream>
#include <utility>  // for std::move

int main() {
    std::vector<int> source = {1, 2, 3, 4, 5};
    
    std::cout << "移動前 source size: " << source.size() << std::endl;
    
    std::vector<int> dest = std::move(source);  // 移動建構
    
    std::cout << "移動後 source size: " << source.size() << std::endl;  // 通常是 0
    std::cout << "移動後 dest size: " << dest.size() << std::endl;      // 5
    
    return 0;
}
