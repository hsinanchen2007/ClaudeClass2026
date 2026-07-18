#include <vector>
#include <iostream>

int main() {
    std::vector<int> empty_vec;
    
    // 危險！未定義行為
    // std::cout << empty_vec.front() << std::endl;
    // std::cout << empty_vec.back() << std::endl;
    
    // 安全的做法：先檢查
    if (!empty_vec.empty()) {
        std::cout << empty_vec.front() << std::endl;
    }
    
    return 0;
}
