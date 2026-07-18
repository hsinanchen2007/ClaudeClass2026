#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

int main() {
    // 泛型 Lambda：參數用 auto
    auto print = [](const auto& x) {
        std::cout << x << " ";
    };
    
    std::cout << "=== 泛型 Lambda ===" << std::endl;
    
    // 用在 int
    std::vector<int> ints = {1, 2, 3, 4, 5};
    std::cout << "ints: ";
    std::for_each(ints.begin(), ints.end(), print);
    std::cout << std::endl;
    
    // 用在 double
    std::vector<double> doubles = {1.1, 2.2, 3.3};
    std::cout << "doubles: ";
    std::for_each(doubles.begin(), doubles.end(), print);
    std::cout << std::endl;
    
    // 用在 string
    std::vector<std::string> strings = {"Hello", "World"};
    std::cout << "strings: ";
    std::for_each(strings.begin(), strings.end(), print);
    std::cout << std::endl;
    
    // 泛型比較
    auto compare = [](const auto& a, const auto& b) {
        return a < b;
    };
    
    std::cout << "\n=== 泛型比較 ===" << std::endl;
    std::cout << "compare(3, 5): " << compare(3, 5) << std::endl;
    std::cout << "compare(3.14, 2.72): " << compare(3.14, 2.72) << std::endl;
    std::cout << "compare(\"apple\", \"banana\"): " 
              << compare(std::string("apple"), std::string("banana")) << std::endl;
    
    return 0;
}
