#include <iostream>
#include <array>

int main() {
    // array：大小在編譯期固定
    std::array<int, 5> arr = {10, 20, 30, 40, 50};
    
    std::cout << "=== std::array ===" << std::endl;
    std::cout << "大小: " << arr.size() << std::endl;
    std::cout << "第一個: " << arr.front() << std::endl;
    std::cout << "最後一個: " << arr.back() << std::endl;
    std::cout << "arr[2]: " << arr[2] << std::endl;
    std::cout << "arr.at(2): " << arr.at(2) << std::endl;  // 有邊界檢查
    
    // 遍歷
    std::cout << "所有元素: ";
    for (int n : arr) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
