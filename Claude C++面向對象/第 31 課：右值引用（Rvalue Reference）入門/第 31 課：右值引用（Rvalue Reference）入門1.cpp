// lesson31_binding_rules.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson31a lesson31_binding_rules.cpp

#include <iostream>
#include <string>

int main() {
    int x = 42;

    // ──── 左值引用 ────
    int& lref = x;          // ✅ 左值引用 → 左值
    // int& lref2 = 42;     // ❌ 左值引用 → 右值（不行！）

    // ──── const 左值引用 ────
    const int& clref1 = x;  // ✅ const 左值引用 → 左值
    const int& clref2 = 42; // ✅ const 左值引用 → 右值（特殊！）

    // ──── 右值引用 ────
    int&& rref = 42;        // ✅ 右值引用 → 右值
    // int&& rref2 = x;     // ❌ 右值引用 → 左值（不行！）

    // 修改右值引用
    rref = 100;             // ✅ 右值引用本身是可修改的
    std::cout << "rref = " << rref << "\n";  // 100

    // ──── 字串的例子 ────
    std::string str = "Hello";

    std::string& sref = str;                     // ✅ 左值引用 → 左值
    const std::string& csref = std::string("X"); // ✅ const 左值引用 → 右值
    std::string&& srref = std::string("Temp");   // ✅ 右值引用 → 右值
    // std::string&& srref2 = str;               // ❌ 右值引用 → 左值

    std::cout << "srref = \"" << srref << "\"\n";

    // 可以修改右值引用綁定的物件
    srref += " Modified";
    std::cout << "srref = \"" << srref << "\"\n";

    return 0;
}
