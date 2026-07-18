#include <iostream>
#include <vector>
#include <algorithm>

// 普通函數
bool is_even(int n) {
    return n % 2 == 0;
}

bool greater_than(int a, int b) {
    return a > b;
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 用普通函數作為謂詞
    int even_count = std::count_if(vec.begin(), vec.end(), is_even);
    std::cout << "偶數個數: " << even_count << std::endl;
    
    // 用普通函數作為比較器
    std::sort(vec.begin(), vec.end(), greater_than);
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
