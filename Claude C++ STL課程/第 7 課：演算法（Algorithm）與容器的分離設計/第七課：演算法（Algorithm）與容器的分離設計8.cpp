#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // fill：用固定值填充, 這裡將 vec 的所有元素填充為 42
    std::cout << "=== fill ===" << std::endl;
    std::vector<int> vec(5);
    std::fill(vec.begin(), vec.end(), 42);
    std::cout << "fill 42: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // fill_n：填充前 n 個, 這裡將 vec 的前 3 個元素填充為 100
    std::cout << "\n=== fill_n ===" << std::endl;
    std::fill_n(vec.begin(), 3, 100);
    std::cout << "fill_n 前 3 個為 100: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // generate：用函數生成值, 這裡使用 lambda 表達式生成遞增的數字乘以 10
    std::cout << "\n=== generate ===" << std::endl;
    int counter = 0;
    std::generate(vec.begin(), vec.end(), [&counter]() {
        return ++counter * 10;
    });
    std::cout << "generate: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // generate_n：生成前 n 個, 這裡使用 lambda 表達式生成 Fibonacci 數列的前 10 個數字
    std::cout << "\n=== generate_n ===" << std::endl;
    std::vector<int> fibs(10);
    int a = 0, b = 1;
    std::generate_n(fibs.begin(), 10, [&a, &b]() {
        int result = a;
        int temp = a + b;
        a = b;
        b = temp;
        return result;
    });
    std::cout << "Fibonacci: ";
    for (int n : fibs) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
