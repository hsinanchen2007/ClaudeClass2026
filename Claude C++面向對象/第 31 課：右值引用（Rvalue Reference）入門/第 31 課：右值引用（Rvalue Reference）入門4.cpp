// lesson31_move_preview.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson31d lesson31_move_preview.cpp

#include <iostream>
#include <string>
#include <utility>

void process(const std::string& s) {
    std::cout << "  [const T&] \"" << s << "\"\n";
}

void process(std::string&& s) {
    std::cout << "  [T&&]      \"" << s << "\"\n";
}

int main() {
    std::string a = "Alpha";
    std::string b = "Beta";

    std::cout << "1. 傳入左值：\n";
    process(a);                  // const T& 版本

    std::cout << "\n2. 傳入 std::move(a)：\n";
    process(std::move(a));       // T&& 版本

    std::cout << "\n3. move 後 a 的狀態：\n";
    std::cout << "   a = \"" << a << "\"\n";          // 通常是空字串，但不保證
    std::cout << "   a.size() = " << a.size() << "\n"; // 通常是 0
    std::cout << "   （有效但未指定的狀態）\n";

    std::cout << "\n4. 可以對 a 重新賦值：\n";
    a = "Gamma";                 // ✅ 重新賦值是安全的
    std::cout << "   a = \"" << a << "\"\n";

    std::cout << "\n5. 右值引用變數本身是左值：\n";
    std::string&& rref = std::string("Omega");
    process(rref);               // const T& 版本！因為 rref 是左值
    process(std::move(rref));    // T&& 版本

    return 0;
}
