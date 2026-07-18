#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 印出每個元素的位址
    for (size_t i = 0; i < v.size(); ++i) {
        std::cout << "v[" << i << "] 位址: " << &v[i] << std::endl;
    }
    
    // 因為是連續記憶體，可以取得原始指標
    int* ptr = v.data();  // 或 &v[0]
    std::cout << "第三個元素: " << ptr[2] << std::endl;  // 輸出 30
    
    return 0;
}
