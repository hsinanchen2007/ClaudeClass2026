#include <iostream>
#include <string>

void f(const std::string& s) { std::cout << "const T& 版本\n"; }
void f(std::string&& s)      { std::cout << "T&& 版本\n"; }

int main() {
    std::string s = "test";

    f(s);                // const T&（s 是左值，只有 const T& 能接）
    f(std::move(s));     // T&&（兩者都能接，但 T&& 更精確匹配）
    f(std::string("x")); // T&&（同上）

    return 0;
}
