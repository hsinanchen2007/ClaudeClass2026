#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1 = {1, 2, 3, 4, 5};    // 明確的初始化串列
    std::vector<int> v2{1, 2, 3, 4, 5};       // 同上，省略等號
    std::vector<int> v3({1, 2, 3, 4, 5});     // 同上，但較少人這樣寫
    
    for (int x : v1) {
        std::cout << x << " ";  // 輸出：1 2 3 4 5
    }
    std::cout << std::endl;
    
    return 0;
}
