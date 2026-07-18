#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

int main() {
    std::vector<int> src = {1, 2, 3, 4, 5};
    
    // copy：複製到另一個容器, 需要確保目標容器有足夠空間
    std::cout << "=== copy ===" << std::endl;
    std::vector<int> dest1(src.size());
    std::copy(src.begin(), src.end(), dest1.begin());
    std::cout << "copy 結果: ";
    for (int n : dest1) std::cout << n << " ";
    std::cout << std::endl;
    
    // copy_if：條件複製, 只複製滿足條件的元素
    std::cout << "\n=== copy_if ===" << std::endl;
    std::vector<int> dest2;
    std::copy_if(src.begin(), src.end(), std::back_inserter(dest2),
        [](int n) { return n % 2 == 0; });
    std::cout << "只複製偶數: ";
    for (int n : dest2) std::cout << n << " ";
    std::cout << std::endl;
    
    // copy_n：複製前 n 個, 需要確保目標容器有足夠空間
    std::cout << "\n=== copy_n ===" << std::endl;
    std::vector<int> dest3;
    std::copy_n(src.begin(), 3, std::back_inserter(dest3));
    std::cout << "複製前 3 個: ";
    for (int n : dest3) std::cout << n << " ";
    std::cout << std::endl;
    
    // copy_backward：從後往前複製, 需要確保目標容器有足夠空間
    std::cout << "\n=== copy_backward ===" << std::endl;
    std::vector<int> dest4(7, 0);  // {0, 0, 0, 0, 0, 0, 0}
    std::copy_backward(src.begin(), src.end(), dest4.end());
    std::cout << "copy_backward: ";
    for (int n : dest4) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
