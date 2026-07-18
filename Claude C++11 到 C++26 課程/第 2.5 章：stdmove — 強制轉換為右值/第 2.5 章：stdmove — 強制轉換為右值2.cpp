#include <iostream>
#include <string>
#include <type_traits>

// 我們自己的 move 實作
template<typename T>
typename std::remove_reference<T>::type&& my_move(T&& arg) noexcept {
    using ReturnType = typename std::remove_reference<T>::type&&;
    return static_cast<ReturnType>(arg);
}

// C++14 可以寫得更簡潔：
// template<typename T>
// decltype(auto) my_move_14(T&& arg) noexcept {
//     return static_cast<std::remove_reference_t<T>&&>(arg);
// }

void test(const std::string& s) { std::cout << "  左值版本\n"; }
void test(std::string&& s)      { std::cout << "  右值版本\n"; }

int main() {
    std::string s = "Hello";

    std::cout << "std::move:\n";
    test(std::move(s));

    s = "Hello";

    std::cout << "my_move:\n";
    test(my_move(s));         // 傳入左值：T = string&

    std::cout << "my_move (rvalue):\n";
    test(my_move(std::string("temp")));  // 傳入右值：T = string

    return 0;
}
