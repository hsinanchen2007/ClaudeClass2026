#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>

int main() {
    std::cout << "=== 算術類函數物件 ===" << std::endl;
    
    // plus<T>：加法
    std::plus<int> add;
    std::cout << "plus: 3 + 5 = " << add(3, 5) << std::endl;
    
    // minus<T>：減法
    std::minus<int> subtract;
    std::cout << "minus: 10 - 3 = " << subtract(10, 3) << std::endl;
    
    // multiplies<T>：乘法
    std::multiplies<int> multiply;
    std::cout << "multiplies: 4 * 6 = " << multiply(4, 6) << std::endl;
    
    // divides<T>：除法
    std::divides<int> divide;
    std::cout << "divides: 20 / 4 = " << divide(20, 4) << std::endl;
    
    // modulus<T>：取餘數
    std::modulus<int> mod;
    std::cout << "modulus: 17 % 5 = " << mod(17, 5) << std::endl;
    
    // negate<T>：取負值
    std::negate<int> neg;
    std::cout << "negate: -7 = " << neg(7) << std::endl;
    
    // 實際應用：計算乘積
    std::cout << "\n=== 實際應用 ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    int product = std::accumulate(vec.begin(), vec.end(), 1, 
                                   std::multiplies<int>());
    std::cout << "1 * 2 * 3 * 4 * 5 = " << product << std::endl;
    
    return 0;
}
