#include <iostream>
#include <vector>
#include <list>
#include <set>

template <typename Container>
void show_container_info(const std::string& name, const Container& c) {
    std::cout << name << ":" << std::endl;
    std::cout << "  size(): " << c.size() << std::endl;
    std::cout << "  empty(): " << (c.empty() ? "true" : "false") << std::endl;
    // max_size(): 容器理論上能容納的最大元素數
    std::cout << "  max_size(): " << c.max_size() << std::endl;
    std::cout << std::endl;
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::list<int> lst = {1, 2, 3};
    std::set<int> s = {1, 2};
    std::vector<int> empty_vec;
    
    show_container_info("vector", vec);
    show_container_info("list", lst);
    show_container_info("set", s);
    show_container_info("empty_vector", empty_vec);
    
    // 其他共同操作
    std::cout << "=== 其他共同操作 ===" << std::endl;
    
    // swap
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {10, 20};
    v1.swap(v2);
    
    std::cout << "swap 後 v1: ";
    for (int n : v1) std::cout << n << " ";
    std::cout << std::endl;
    
    // clear
    v1.clear();
    std::cout << "clear 後 v1.size(): " << v1.size() << std::endl;
    
    return 0;
}
