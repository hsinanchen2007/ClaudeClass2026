#include <iostream>
#include <string>
#include <utility>

void final_target(const std::string& s) { std::cout << "  最終: 左值\n"; }
void final_target(std::string&& s)      { std::cout << "  最終: 右值\n"; }

// 第三層
template<typename T>
void layer3(T&& arg) {
    std::cout << "  layer3 → final_target\n";
    final_target(std::forward<T>(arg));
}

// 第二層
template<typename T>
void layer2(T&& arg) {
    std::cout << "  layer2 → layer3\n";
    layer3(std::forward<T>(arg));
}

// 第一層
template<typename T>
void layer1(T&& arg) {
    std::cout << "  layer1 → layer2\n";
    layer2(std::forward<T>(arg));
}

int main() {
    std::string s = "Hello";

    std::cout << "傳入左值，穿越三層:\n";
    layer1(s);

    std::cout << "\n傳入右值，穿越三層:\n";
    layer1(std::string("tmp"));

    return 0;
}
