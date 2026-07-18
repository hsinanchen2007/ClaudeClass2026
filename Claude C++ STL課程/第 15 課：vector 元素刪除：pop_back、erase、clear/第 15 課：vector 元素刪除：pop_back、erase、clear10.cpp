#include <vector>
#include <iostream>
#include <algorithm>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // Erase-Remove 慣用法（一行）
    v.erase(std::remove_if(v.begin(), v.end(), 
                           [](int x) { return x % 2 == 0; }),
            v.end());
    
    for (int x : v) std::cout << x << " ";  // 1 3 5 7 9
    std::cout << std::endl;
    
    return 0;
}
