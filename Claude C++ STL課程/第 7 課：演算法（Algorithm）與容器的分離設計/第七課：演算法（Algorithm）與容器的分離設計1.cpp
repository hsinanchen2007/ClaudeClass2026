#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <algorithm>

int main() {
    std::cout << "=== 同一個 find 用在不同容器 ===" << std::endl;
    
    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::list<int> lst = {10, 20, 30, 40, 50};
    std::set<int> s = {10, 20, 30, 40, 50};
    
    // std::find 只認識迭代器，不認識容器, 所以同一個 find 可以用在 vector、list、set 等不同容器上
    auto v_it = std::find(vec.begin(), vec.end(), 30);
    auto l_it = std::find(lst.begin(), lst.end(), 30);
    auto s_it = std::find(s.begin(), s.end(), 30);
    
    std::cout << "vector: " << (v_it != vec.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "list: " << (l_it != lst.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "set: " << (s_it != s.end() ? "找到" : "沒找到") << std::endl;
    
    // 甚至可以用在原生陣列（指標就是迭代器）, 需要提供陣列的開始和結束位置
    int arr[] = {10, 20, 30, 40, 50};
    auto a_it = std::find(arr, arr + 5, 30);
    std::cout << "array: " << (a_it != arr + 5 ? "找到" : "沒找到") << std::endl;
    
    return 0;
}
