#include <iostream>
#include <list>
#include <set>
 
int main() {
    // list 的迭代器是 Bidirectional Iterator
    std::cout << "=== list ===" << std::endl;
    std::list<int> lst = {10, 20, 30, 40, 50};
    
    auto it = lst.begin();
    std::cout << "起始: " << *it << std::endl;
    
    ++it;
    std::cout << "++it: " << *it << std::endl;
    
    ++it;
    std::cout << "++it: " << *it << std::endl;
    
    --it;  // Bidirectional 可以後退！
    std::cout << "--it: " << *it << std::endl;
    
    // 但不能跳躍
    // it + 2;  // 編譯錯誤！
    
    // set 也是 Bidirectional Iterator
    std::cout << "\n=== set ===" << std::endl;
    std::set<int> s = {50, 10, 30, 20, 40};  // 自動排序
    
    std::cout << "正向: ";
    for (auto it = s.begin(); it != s.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    std::cout << "反向: ";
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
 