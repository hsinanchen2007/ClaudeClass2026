#include <iostream>
#include <vector>
#include <list>
#include <iterator>
 
int main() {
    // std::advance：移動迭代器
    std::cout << "=== std::advance ===" << std::endl;
    
    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::list<int> lst = {10, 20, 30, 40, 50};
    
    auto vit = vec.begin();
    auto lit = lst.begin();
    
    // 對於 Random Access Iterator，advance 內部用 it + n
    std::advance(vit, 3);
    std::cout << "vector advance(it, 3): " << *vit << std::endl;
    
    // 對於 Bidirectional Iterator，advance 內部用 ++it 三次
    std::advance(lit, 3);
    std::cout << "list advance(it, 3): " << *lit << std::endl;
    
    // std::distance：計算距離
    std::cout << "\n=== std::distance ===" << std::endl;
    
    auto vbegin = vec.begin();
    auto vend = vec.end();
    std::cout << "vector distance: " << std::distance(vbegin, vend) << std::endl;
    
    auto lbegin = lst.begin();
    auto lend = lst.end();
    std::cout << "list distance: " << std::distance(lbegin, lend) << std::endl;
    
    // std::next 和 std::prev（C++11）：更方便的移動
    std::cout << "\n=== std::next / std::prev ===" << std::endl;
    
    auto it = vec.begin();
    auto next_it = std::next(it, 2);  // 前進 2 步，不修改 it
    auto prev_it = std::prev(vec.end(), 1);  // 後退 1 步
    
    std::cout << "*it = " << *it << std::endl;
    std::cout << "*next(it, 2) = " << *next_it << std::endl;
    std::cout << "*prev(end, 1) = " << *prev_it << std::endl;
    
    return 0;
}
 