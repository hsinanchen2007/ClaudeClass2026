#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // for_each：對每個元素執行操作, 使用 lambda 表達式輸出元素
    std::cout << "=== for_each（輸出）===" << std::endl;
    std::cout << "元素: ";
    std::for_each(vec.begin(), vec.end(), [](int n) {
        std::cout << n << " ";
    });
    std::cout << std::endl;
    
    // for_each 也可以修改元素（透過參考）, 使用 lambda 表達式將每個元素乘以 2
    std::cout << "\n=== for_each（修改）===" << std::endl;
    std::for_each(vec.begin(), vec.end(), [](int& n) {
        n *= 2;
    });
    std::cout << "乘以 2 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // for_each 會回傳函數物件（可以累積狀態）, 使用 struct 定義一個累積總和的函數物件
    std::cout << "\n=== for_each（累積）===" << std::endl;
    struct Sum {
        int total = 0;
        void operator()(int n) { total += n; }
    };
    
    Sum result = std::for_each(vec.begin(), vec.end(), Sum());
    std::cout << "總和: " << result.total << std::endl;
    
    return 0;
}
