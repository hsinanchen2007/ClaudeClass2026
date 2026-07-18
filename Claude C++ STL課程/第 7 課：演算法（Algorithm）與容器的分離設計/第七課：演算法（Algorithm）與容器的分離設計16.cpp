#include <iostream>
#include <list>
#include <set>
#include <algorithm>

int main() {
    // list 的成員函數 vs STL 演算法
    std::cout << "=== list 的成員函數 ===" << std::endl;
    
    std::list<int> lst = {5, 2, 8, 1, 9, 2, 7, 2};
    
    // list::sort() 比 std::sort() 更適合 list
    // 因為 list::sort() 只調整指標，不需要移動元素, 而 std::sort() 需要隨機存取，效率較低
    lst.sort();
    std::cout << "sort 後: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // list::remove() 比 erase-remove 慣用法更高效, 因為 list::remove() 只調整指標，不需要移動元素
    lst.remove(2);
    std::cout << "remove(2) 後: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // list::unique() 移除連續重複, 但不會移除非連續的重複元素
    std::list<int> lst2 = {1, 1, 2, 2, 2, 3, 3};
    lst2.unique();
    std::cout << "unique 後: ";
    for (int n : lst2) std::cout << n << " ";
    std::cout << std::endl;
    
    // set 的成員函數
    std::cout << "\n=== set 的成員函數 ===" << std::endl;
    std::set<int> s = {5, 2, 8, 1, 9};
    
    // set::find() 是 O(log n), 因為 set 是用紅黑樹實作的，而 std::find() 是 O(n)，因為它用線性搜尋
    // std::find() 在 set 上是 O(n)（因為它用線性搜尋）, 所以 set::find() 更適合 set
    auto it = s.find(8);  // O(log n) - 使用紅黑樹
    // auto it2 = std::find(s.begin(), s.end(), 8);  // O(n) - 線性搜尋
    
    if (it != s.end()) {
        std::cout << "set::find(8): 找到" << std::endl;
    }
    
    return 0;
}
