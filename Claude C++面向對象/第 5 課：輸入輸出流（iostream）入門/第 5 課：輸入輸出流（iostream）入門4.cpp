#include <iostream>

int main() {
    // 方式一：std::endl（換行 + 清空緩衝區）
    std::cout << "Line 1" << std::endl;
    
    // 方式二：'\n'（只換行，不清空緩衝區）
    std::cout << "Line 2\n";
    
    // 方式三：std::flush（只清空緩衝區，不換行）
    std::cout << "Line 3" << std::flush;
    std::cout << " continued" << std::endl;
    
    return 0;
}
