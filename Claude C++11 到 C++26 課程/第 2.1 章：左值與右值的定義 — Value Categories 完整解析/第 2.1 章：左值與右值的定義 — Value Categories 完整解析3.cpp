#include <iostream>
#include <string>
#include <utility>  // std::move

int main() {
    std::string s = "Hello, World!";

    // std::move(s) 是 xvalue
    // 它仍然指向 s 這個物件（有身份），
    // 但告訴編譯器「可以移動」
    std::string s2 = std::move(s);  // s 的資源被移動到 s2

    std::cout << "s  = \"" << s  << "\"\n";  // s 現在是空的（或未指定狀態）
    std::cout << "s2 = \"" << s2 << "\"\n";  // "Hello, World!"

    return 0;
}
