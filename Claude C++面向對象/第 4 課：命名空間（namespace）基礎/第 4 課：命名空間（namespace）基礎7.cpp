#include <iostream>

// 第一部分
namespace mylib {
    void funcA() {
        std::cout << "Function A" << std::endl;
    }
}

// 第二部分（擴展同一個命名空間）
namespace mylib {
    void funcB() {
        std::cout << "Function B" << std::endl;
    }
}

// 這在多檔案專案中很常見：
// math.h 定義 namespace mylib { 數學函數 }
// string.h 定義 namespace mylib { 字串函數 }
// 它們都屬於同一個 mylib 命名空間

int main() {
    mylib::funcA();
    mylib::funcB();
    
    return 0;
}
