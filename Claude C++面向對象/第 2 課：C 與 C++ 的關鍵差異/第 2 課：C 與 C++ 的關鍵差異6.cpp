#include <iostream>

int main() {
    // 配置單一變數
    int* p = new int;      // 不需要 sizeof，不需要轉型
    *p = 42;
    std::cout << "值: " << *p << std::endl;
    delete p;              // 使用 delete 而非 free
    
    // 配置陣列
    int* arr = new int[5]; // 配置陣列使用 new[]
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }
    delete[] arr;          // 釋放陣列使用 delete[]
    
    return 0;
}
