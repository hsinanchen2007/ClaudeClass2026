#include <iostream>

namespace very_long_namespace_name {
    void greet() {
        std::cout << "Hello from long namespace!" << std::endl;
    }
}

int main() {
    // 建立別名
    namespace vln = very_long_namespace_name;
    
    // 使用別名
    vln::greet();
    
    // 原名稱仍然有效
    very_long_namespace_name::greet();
    
    return 0;
}
