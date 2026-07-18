#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 危險！越界存取
    // 編譯器不會報錯，但這是未定義行為
    std::cout << v[10] << std::endl;   // 可能印出垃圾值，可能當機
    
    v[10] = 999;  // 可能破壞其他記憶體，導致難以追蹤的 bug
    
    return 0;
}
