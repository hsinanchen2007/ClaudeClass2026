#include <iostream>
#include <string>
#include <utility>

int main() {
    std::string s = "Hello, World!";

    // 方式 1：std::move
    std::string a = std::move(s);
    std::cout << "s after std::move: \"" << s << "\"\n";

    s = "Hello, World!";  // 重新賦值

    // 方式 2：static_cast
    std::string b = static_cast<std::string&&>(s);
    std::cout << "s after static_cast: \"" << s << "\"\n";

    // 兩者行為完全相同
    std::cout << "a = \"" << a << "\"\n";
    std::cout << "b = \"" << b << "\"\n";

    return 0;
}
