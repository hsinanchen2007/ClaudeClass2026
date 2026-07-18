#include <iostream>
#include <list>

int main() {
    std::cout << "=== std::list ===" << std::endl;
    
    std::list<int> lst = {20, 40};
    
    // 任意位置插入都是 O(1)（前提是已有迭代器）
    lst.push_front(10);
    lst.push_back(50);
    
    // 在中間插入
    auto it = lst.begin();
    ++it;  // 指向 20
    ++it;  // 指向 40
    lst.insert(it, 30);  // 在 40 前面插入 30
    
    std::cout << "元素: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // list 特有的操作
    std::list<int> lst2 = {100, 200, 300};
    
    // splice：將 lst2 的元素移動到 lst
    it = lst.begin();
    std::advance(it, 2);  // 指向 30

    // 將 lst2 的所有元素插入到 lst 的 it 位置（30 前面）
    // splice 是 O(1) 操作，因為它只是改變指標，不需要複製元素
    // 注意：splice 後 lst2 會變空，因為元素被移動了
    lst.splice(it, lst2);  // 在 30 前面插入 lst2 的所有元素
    
    std::cout << "splice 後 lst: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "splice 後 lst2 大小: " << lst2.size() << std::endl;
    
    return 0;
}
