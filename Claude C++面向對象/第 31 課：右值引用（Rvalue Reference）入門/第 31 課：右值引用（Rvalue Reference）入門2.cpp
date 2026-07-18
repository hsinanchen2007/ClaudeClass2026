// lesson31_rref_is_lvalue.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson31b lesson31_rref_is_lvalue.cpp

#include <iostream>

void takeRvalueRef(int&& val) {
    std::cout << "  takeRvalueRef: val = " << val << "\n";
    std::cout << "  &val = " << &val << " （可以取址 → val 是左值）\n";

    // val 是左值，所以不能傳給另一個接受右值引用的函數：
    // takeRvalueRef(val);  // ❌ 錯誤！val 是左值
}

void takeLvalueRef(int& val) {
    std::cout << "  takeLvalueRef: val = " << val << "\n";
}

void takeConstRef(const int& val) {
    std::cout << "  takeConstRef: val = " << val << "\n";
}

int main() {
    int x = 10;

    std::cout << "=== 傳入右值 ===\n";
    takeRvalueRef(42);           // ✅ 42 是右值
    takeRvalueRef(x + 1);       // ✅ x + 1 是右值

    std::cout << "\n=== 右值引用變數是左值 ===\n";
    int&& rref = 100;
    std::cout << "  rref = " << rref << "\n";
    std::cout << "  &rref = " << &rref << " （可以取址）\n";

    // rref 是左值，所以可以傳給接受左值引用的函數：
    takeLvalueRef(rref);    // ✅ rref 是左值
    takeConstRef(rref);     // ✅ rref 是左值

    // 但不能再傳給接受右值引用的函數：
    // takeRvalueRef(rref); // ❌ rref 是左值！

    return 0;
}
