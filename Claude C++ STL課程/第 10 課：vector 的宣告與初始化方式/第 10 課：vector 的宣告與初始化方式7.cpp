#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3};
    
    // 方法一：指定數量和值
    v.assign(5, 100);
    std::cout << "assign(5, 100): ";
    for (int x : v) std::cout << x << " ";  // 輸出：100 100 100 100 100
    std::cout << std::endl;
    
    // 方法二：從初始化串列
    v.assign({10, 20, 30});
    std::cout << "assign({10, 20, 30}): ";
    for (int x : v) std::cout << x << " ";  // 輸出：10 20 30
    std::cout << std::endl;
    
    // 方法三：從迭代器範圍
    std::vector<int> other = {7, 8, 9, 10, 11};
    v.assign(other.begin() + 1, other.end() - 1);
    std::cout << "從迭代器範圍: ";
    for (int x : v) std::cout << x << " ";  // 輸出：8 9 10
    std::cout << std::endl;
    
    return 0;
}
