#include <iostream>
#include <vector>
#include <list>
#include <forward_list>
#include <algorithm>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    std::list<int> lst = {5, 2, 8, 1, 9};
    std::forward_list<int> flst = {5, 2, 8, 1, 9};
    
    // find：只需要 Input Iterator（所有容器都能用）, 但效率可能不同
    std::cout << "=== find（Input Iterator）===" << std::endl;
    std::cout << "vector: " << (*std::find(vec.begin(), vec.end(), 8)) << std::endl;
    std::cout << "list: " << (*std::find(lst.begin(), lst.end(), 8)) << std::endl;
    std::cout << "forward_list: " << (*std::find(flst.begin(), flst.end(), 8)) << std::endl;
    
    // reverse：需要 Bidirectional Iterator, list 和 vector 支援，但 forward_list 不支援
    std::cout << "\n=== reverse（Bidirectional Iterator）===" << std::endl;
    std::reverse(vec.begin(), vec.end());
    std::reverse(lst.begin(), lst.end());
    // std::reverse(flst.begin(), flst.end());  // 錯誤！forward_list 不支援
    
    std::cout << "vector reversed: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // sort：需要 Random Access Iterator, vector 支援，但 list 和 forward_list 不支援
    std::cout << "\n=== sort（Random Access Iterator）===" << std::endl;
    std::sort(vec.begin(), vec.end());
    // std::sort(lst.begin(), lst.end());  // 錯誤！list 不支援
    
    // list 和 forward_list 有自己的 sort, 但效率較低，因為它們沒有隨機存取能力
    lst.sort();
    flst.sort();
    
    std::cout << "All sorted successfully!" << std::endl;
    
    return 0;
}
