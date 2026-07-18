#include <iostream>

// 匿名命名空間
namespace {
    int internalCounter = 0;  // 只在這個檔案內可見
    
    void internalFunction() {
        std::cout << "這是內部函數" << std::endl;
    }
}

int main() {
    internalCounter = 10;  // 可以直接使用，不需前綴
    internalFunction();
    
    return 0;
}
