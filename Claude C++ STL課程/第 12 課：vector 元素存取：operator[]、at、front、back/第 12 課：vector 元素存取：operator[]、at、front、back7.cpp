#include <vector>
#include <iostream>

void print_first(const std::vector<int>& v) {
    if (!v.empty()) {
        std::cout << v[0] << std::endl;      // OK，讀取
        std::cout << v.at(0) << std::endl;   // OK，讀取
        std::cout << v.front() << std::endl; // OK，讀取
        
        // v[0] = 100;      // 編譯錯誤！不能修改
        // v.front() = 100; // 編譯錯誤！不能修改
    }
    
    const int* ptr = v.data();  // 回傳 const int*
    // ptr[0] = 100;  // 編譯錯誤！
}

int main() {
    std::vector<int> v = {1, 2, 3};
    print_first(v);
    return 0;
}
