#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <string>

int main() {
    // 完整型別宣告（冗長）
    std::vector<int> vec = {1, 2, 3};
    std::vector<int>::iterator it1 = vec.begin();
    
    // 使用 auto（簡潔）
    auto it2 = vec.begin();
    
    std::cout << "兩者等價：*it1 = " << *it1 << ", *it2 = " << *it2 << std::endl;
    
    // 對於複雜型別，auto 更加重要
    std::map<std::string, int> scores;
    scores["Alice"] = 95;
    scores["Bob"] = 87;
    
    // 不用 auto，型別非常長
    std::map<std::string, int>::iterator map_it1 = scores.begin();
    
    // 用 auto，清爽多了
    auto map_it2 = scores.begin();
    
    std::cout << "map 第一個元素：" << map_it2->first 
              << " = " << map_it2->second << std::endl;
    
    return 0;
}
