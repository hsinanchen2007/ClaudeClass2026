#include <iostream>
#include <string>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    std::cout << "max(10, 20) = " << find_max(10, 20) << std::endl;
    std::cout << "max(3.14, 2.72) = " << find_max(3.14, 2.72) << std::endl;
    std::cout << "max('a', 'z') = " << find_max('a', 'z') << std::endl;
    
    // 字串比較（字典序）
    std::string s1 = "apple", s2 = "banana";
    std::cout << "max(\"apple\", \"banana\") = " << find_max(s1, s2) << std::endl;
    
    return 0;
}
