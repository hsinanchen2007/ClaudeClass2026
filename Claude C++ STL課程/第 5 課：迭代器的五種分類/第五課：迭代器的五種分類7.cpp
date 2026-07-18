#include <iostream>
#include <vector>
#include <deque>
 
int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    std::cout << "=== Random Access Iterator 完整功能 ===" << std::endl;
    
    auto it = vec.begin();
    
    // 基本操作
    std::cout << "*it = " << *it << std::endl;
    
    // 前進後退
    ++it;
    std::cout << "++it: *it = " << *it << std::endl;
    --it;
    std::cout << "--it: *it = " << *it << std::endl;
    
    // 跳躍！
    it = it + 3;
    std::cout << "it + 3: *it = " << *it << std::endl;
    
    it = it - 2;
    std::cout << "it - 2: *it = " << *it << std::endl;
    
    // 下標存取
    it = vec.begin();
    std::cout << "it[2] = " << it[2] << std::endl;
    std::cout << "it[4] = " << it[4] << std::endl;
    
    // 計算距離
    auto begin = vec.begin();
    auto end = vec.end();
    std::cout << "end - begin = " << (end - begin) << std::endl;
    
    // 比較大小
    auto mid = vec.begin() + 2;
    std::cout << "begin < mid? " << (begin < mid ? "是" : "否") << std::endl;
    std::cout << "end > mid? " << (end > mid ? "是" : "否") << std::endl;
    
    return 0;
}
 