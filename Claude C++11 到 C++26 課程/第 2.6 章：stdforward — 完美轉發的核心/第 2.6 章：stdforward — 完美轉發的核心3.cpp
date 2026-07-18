#include <iostream>
#include <string>
#include <utility>

void check(const std::string& s) { std::cout << "  左值\n"; }
void check(std::string&& s)      { std::cout << "  右值\n"; }

template<typename T>
void demo(T&& arg) {
    std::cout << "原樣傳遞 arg:       ";
    check(arg);                         // 永遠是左值

    std::cout << "std::move(arg):     ";
    check(std::move(arg));              // 永遠是右值

    std::cout << "std::forward<T>(arg):";
    check(std::forward<T>(arg));        // 保持原始值類別
}

int main() {
    std::string s = "Hello";

    std::cout << "=== 傳入左值 ===\n";
    demo(s);

    std::cout << "\n=== 傳入右值 ===\n";
    demo(std::string("tmp"));

    return 0;
}
