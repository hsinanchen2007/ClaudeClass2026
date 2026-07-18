#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    std::cout << "=== 迭代器基本操作 ===" << std::endl;
    
    // 取得起始迭代器
    std::vector<int>::iterator it = vec.begin();
    
    // *it：解參考
    std::cout << "*it = " << *it << std::endl;
    
    // ++it：前進
    ++it;
    std::cout << "++it 後，*it = " << *it << std::endl;
    
    // it++：後置前進
    std::cout << "*it++ = " << *it++ << std::endl;  // 先取值，再前進
    std::cout << "現在 *it = " << *it << std::endl;
    
    // 比較
    std::cout << "\n=== 迭代器比較 ===" << std::endl;
    auto begin = vec.begin();
    auto end = vec.end();
    
    std::cout << "begin == end? " << (begin == end ? "是" : "否") << std::endl;
    std::cout << "begin != end? " << (begin != end ? "是" : "否") << std::endl;
    
    return 0;
}
