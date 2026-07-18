#include <iostream>
#include <string>
#include <utility>

void target(const std::string& s) { std::cout << "  target(const T&) 左值版本\n"; }
void target(std::string&& s)      { std::cout << "  target(T&&) 右值版本\n"; }

template<typename T>
void wrapper(T&& arg) {
    std::cout << "  轉發中...\n";
    target(std::forward<T>(arg));  // 完美轉發
}

int main() {
    std::string s = "Hello";

    std::cout << "傳入左值:\n";
    wrapper(s);                     // 左值 → 轉發為左值 ✅

    std::cout << "\n傳入右值:\n";
    wrapper(std::string("tmp"));    // 右值 → 轉發為右值 ✅

    std::cout << "\n傳入 std::move:\n";
    wrapper(std::move(s));          // 右值 → 轉發為右值 ✅

    return 0;
}
