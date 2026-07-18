#include <iostream>
#include <set>

int main() {
    std::cout << "=== std::set ===" << std::endl;
    
    // 元素自動排序，不允許重複
    std::set<int> s;
    
    s.insert(30);
    s.insert(10);
    s.insert(50);
    s.insert(20);
    s.insert(40);
    s.insert(30);  // 重複，會被忽略
    
    std::cout << "元素（自動排序）: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "大小: " << s.size() << std::endl;
    
    // 查找
    auto it = s.find(30);
    if (it != s.end()) {
        std::cout << "找到 30" << std::endl;
    }
    
    // count：因為不重複，只會回傳 0 或 1
    std::cout << "30 的數量: " << s.count(30) << std::endl;
    std::cout << "100 的數量: " << s.count(100) << std::endl;
    
    // 刪除
    s.erase(30);
    std::cout << "刪除 30 後: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
