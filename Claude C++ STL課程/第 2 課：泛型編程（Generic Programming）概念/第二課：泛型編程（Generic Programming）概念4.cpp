#include <iostream>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    // 問題：10 是 int，3.14 是 double，型別不一致
    // std::cout << find_max(10, 3.14);  // 編譯錯誤！
    
    // 解法一：明確指定型別, 讓編譯器知道我們想用 double 版本的 find_max
    std::cout << find_max<double>(10, 3.14) << std::endl;
    
    // 解法二：讓兩個參數型別一致
    std::cout << find_max(10.0, 3.14) << std::endl;
    
    return 0;
}
