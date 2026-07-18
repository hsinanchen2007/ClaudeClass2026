#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // Lambda 表達式
    int count = std::count_if(vec.begin(), vec.end(), 
        [](int n) { return n % 2 == 0; }  // Lambda
    );
    std::cout << "偶數個數: " << count << std::endl;
    
    // 帶捕獲的 Lambda
    int divisor = 3;
    int count3 = std::count_if(vec.begin(), vec.end(),
        [divisor](int n) { return n % divisor == 0; }  // 捕獲外部變數
    );
    std::cout << "3的倍數個數: " << count3 << std::endl;
    
    // 用 Lambda 做 transform
    std::vector<int> squared;
    squared.resize(vec.size());
    std::transform(vec.begin(), vec.end(), squared.begin(),
        [](int n) { return n * n; }
    );
    std::cout << "平方: ";
    for (int n : squared) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
