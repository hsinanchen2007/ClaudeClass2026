#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>

int main() {
    // 序列容器：vector
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::cout << "vector: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 序列容器：list
    std::list<int> lst = {10, 20, 30};
    lst.push_front(5);  // list 可以高效地在前端插入
    std::cout << "list: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // 關聯容器：set（自動排序、不重複）
    std::set<int> s = {30, 10, 20, 10, 30};  // 重複的會被忽略
    std::cout << "set: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;
    
    // 關聯容器：map（鍵值對）
    std::map<std::string, int> ages;
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 35;
    std::cout << "map: ";
    for (const auto& pair : ages) {
        std::cout << pair.first << "=" << pair.second << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
