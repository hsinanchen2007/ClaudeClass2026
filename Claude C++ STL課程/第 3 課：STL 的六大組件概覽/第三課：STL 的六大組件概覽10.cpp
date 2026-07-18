#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // reverse_iterator：反向迭代
    std::cout << "反向迭代: ";
    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;
    
    // back_inserter：自動在尾端插入
    std::vector<int> src = {10, 20, 30};
    std::vector<int> dest;
    
    std::copy(src.begin(), src.end(), std::back_inserter(dest));
    std::cout << "back_inserter 結果: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;
    
    // ostream_iterator：輸出到串流
    std::cout << "ostream_iterator: ";
    std::copy(vec.begin(), vec.end(), 
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    return 0;
}
