#include <vector>
#include <iostream>
#include <cstring>  // for memcpy

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 取得指向內部陣列的指標
    int* ptr = v.data();
    
    std::cout << "ptr[0] = " << ptr[0] << std::endl;  // 10
    std::cout << "ptr[2] = " << ptr[2] << std::endl;  // 30
    
    // 用指標修改
    ptr[1] = 200;
    std::cout << "v[1] = " << v[1] << std::endl;  // 200
    
    // 實用場景：與 C 函數互動
    // 例如 memcpy、C 風格 API 等
    int dest[5];
    std::memcpy(dest, v.data(), v.size() * sizeof(int));
    
    return 0;
}
