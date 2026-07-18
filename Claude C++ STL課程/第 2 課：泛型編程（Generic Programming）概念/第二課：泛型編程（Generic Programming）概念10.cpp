#include <iostream>
#include <concepts>

// 定義一個 Concept：可比較的型別
template <typename T>
concept Comparable = requires(T a, T b) {
    { a > b } -> std::convertible_to<bool>;
    { a < b } -> std::convertible_to<bool>;
};

// 使用 Concept 約束模板
template <Comparable T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    std::cout << find_max(10, 20) << std::endl;      // OK
    std::cout << find_max(3.14, 2.72) << std::endl;  // OK
    
    return 0;
}
