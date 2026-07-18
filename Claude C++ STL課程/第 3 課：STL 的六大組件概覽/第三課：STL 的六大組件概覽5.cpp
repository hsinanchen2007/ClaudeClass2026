#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <algorithm>

template <typename Container>
void print_container(const std::string& name, const Container& c) {
    std::cout << name << ": ";
    for (const auto& elem : c) std::cout << elem << " ";
    std::cout << std::endl;
}

int main() {
    std::vector<int> vec = {3, 1, 4, 1, 5};
    std::list<int> lst = {9, 2, 6, 5, 3};
    std::deque<int> deq = {5, 8, 9, 7, 9};
    
    // 同一個 sort 演算法用在 vector 和 deque
    std::sort(vec.begin(), vec.end());
    std::sort(deq.begin(), deq.end());
    
    // list 有自己的 sort（因為 std::sort 需要 Random Access Iterator）
    lst.sort();
    
    print_container("vector", vec);
    print_container("list", lst);
    print_container("deque", deq);
    
    // 同一個 find 演算法用在所有容器
    std::cout << "\n尋找元素 5：" << std::endl;
    
    auto vit = std::find(vec.begin(), vec.end(), 5);
    std::cout << "  vector: " << (vit != vec.end() ? "找到" : "沒找到") << std::endl;
    
    auto lit = std::find(lst.begin(), lst.end(), 5);
    std::cout << "  list: " << (lit != lst.end() ? "找到" : "沒找到") << std::endl;
    
    auto dit = std::find(deq.begin(), deq.end(), 5);
    std::cout << "  deque: " << (dit != deq.end() ? "找到" : "沒找到") << std::endl;
    
    return 0;
}
