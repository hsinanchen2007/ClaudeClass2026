#include <iostream>

int main() {
    int arr[] = {10, 20, 30, 40, 50};
    
    // 用指標遍歷陣列
    for (int* ptr = arr; ptr < arr + 5; ++ptr) {
        std::cout << *ptr << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
