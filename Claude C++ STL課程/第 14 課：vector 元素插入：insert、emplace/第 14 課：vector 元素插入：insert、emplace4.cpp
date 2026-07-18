#include <vector>
#include <iostream>
#include <array>

int main() {
    std::vector<int> v = {1, 2, 3};
    
    // 從另一個容器插入範圍
    std::vector<int> source = {10, 20, 30};
    v.insert(v.begin() + 1, source.begin(), source.end());
    
    std::cout << "插入 vector 範圍後: ";
    for (int x : v) {
        std::cout << x << " ";  // 1 10 20 30 2 3
    }
    std::cout << std::endl;
    
    // 從陣列插入
    int arr[] = {100, 200};
    v.insert(v.end(), std::begin(arr), std::end(arr));
    
    std::cout << "插入陣列後: ";
    for (int x : v) {
        std::cout << x << " ";  // 1 10 20 30 2 3 100 200
    }
    std::cout << std::endl;
    
    return 0;
}
