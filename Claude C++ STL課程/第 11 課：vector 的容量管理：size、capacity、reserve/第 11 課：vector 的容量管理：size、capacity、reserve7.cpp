#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    
    std::cout << "max_size: " << v.max_size() << std::endl;
    
    // 在 64 位元系統上，這個數字通常非常大
    // 例如：4611686018427387903（約 4.6 × 10^18）
    
    return 0;
}
