#include <iostream>
#include <vector>
#include <list>
#include <forward_list>
#include <algorithm>
 
int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    std::list<int> lst = {5, 2, 8, 1, 9};
    std::forward_list<int> flst = {5, 2, 8, 1, 9};
    
    // find：只需要 Input Iterator
    // 所有容器都能用
    std::cout << "=== find（需要 Input Iterator）===" << std::endl;
    
    auto v_it = std::find(vec.begin(), vec.end(), 8);
    auto l_it = std::find(lst.begin(), lst.end(), 8);
    auto f_it = std::find(flst.begin(), flst.end(), 8);
    
    std::cout << "vector: " << (v_it != vec.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "list: " << (l_it != lst.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "forward_list: " << (f_it != flst.end() ? "找到" : "沒找到") << std::endl;
    
    // reverse：需要 Bidirectional Iterator
    // forward_list 不能用！
    std::cout << "\n=== reverse（需要 Bidirectional Iterator）===" << std::endl;
    
    std::reverse(vec.begin(), vec.end());
    std::reverse(lst.begin(), lst.end());
    // std::reverse(flst.begin(), flst.end());  // 編譯錯誤！
    
    std::cout << "vector 反轉: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "list 反轉: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // sort：需要 Random Access Iterator
    // 只有 vector 能用 std::sort！
    std::cout << "\n=== sort（需要 Random Access Iterator）===" << std::endl;
    
    std::sort(vec.begin(), vec.end());
    // std::sort(lst.begin(), lst.end());  // 編譯錯誤！
    // std::sort(flst.begin(), flst.end());  // 編譯錯誤！
    
    // list 和 forward_list 有自己的 sort 成員函數, 但效率不如 std::sort
    lst.sort();
    flst.sort();
    
    std::cout << "vector 排序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "list 排序: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "forward_list 排序: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
 