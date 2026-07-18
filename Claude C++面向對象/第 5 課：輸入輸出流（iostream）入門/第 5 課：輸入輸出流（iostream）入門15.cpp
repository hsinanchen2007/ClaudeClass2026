#include <iostream>
#include <iomanip>  // 格式化控制器

int main() {
    double pi = 3.14159265358979;
    int num = 42;
    
    // 設定小數精度
    std::cout << "預設: " << pi << std::endl;
    std::cout << "精度3: " << std::setprecision(3) << pi << std::endl;
    std::cout << "精度8: " << std::setprecision(8) << pi << std::endl;
    
    // fixed 固定小數點表示法
    std::cout << "fixed精度2: " << std::fixed << std::setprecision(2) 
              << pi << std::endl;
    
    return 0;
}
