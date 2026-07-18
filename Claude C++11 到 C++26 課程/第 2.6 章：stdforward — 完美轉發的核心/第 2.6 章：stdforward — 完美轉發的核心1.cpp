#include <iostream>
#include <string>
#include <utility>

void target(const std::string& s) { std::cout << "  target(const T&) 左值版本\n"; }
void target(std::string&& s)      { std::cout << "  target(T&&) 右值版本\n"; }

// 嘗試 1：用 const T& 轉發
template<typename T>
void wrapper_v1(const T& arg) {
    target(arg);  // arg 永遠是左值 → 永遠呼叫左值版本
}

// 嘗試 2：用 T&& 轉發
template<typename T>
void wrapper_v2(T&& arg) {
    target(arg);  // arg 有名字 → 還是左值 → 還是呼叫左值版本！
}

int main() {
    std::string s = "Hello";

    std::cout << "=== wrapper_v1（const T&）===\n";
    wrapper_v1(s);                   // 左值 → 左值版本 ✅
    wrapper_v1(std::string("tmp"));  // 右值 → 左值版本 ❌ 應該是右值版本

    std::cout << "\n=== wrapper_v2（T&&）===\n";
    wrapper_v2(s);                   // 左值 → 左值版本 ✅
    wrapper_v2(std::string("tmp"));  // 右值 → 左值版本 ❌ 還是錯！

    return 0;
}
