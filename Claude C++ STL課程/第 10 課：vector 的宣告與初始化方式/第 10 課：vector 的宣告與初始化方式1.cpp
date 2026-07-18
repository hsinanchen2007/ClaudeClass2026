#include <vector>
#include <string>

int main() {
    std::vector<int> v1;           // 空的 vector，size=0，capacity=0
    std::vector<double> v2{};      // 同上，C++11 統一初始化語法
    std::vector<std::string> v3 = {};  // 同上
    
    return 0;
}
