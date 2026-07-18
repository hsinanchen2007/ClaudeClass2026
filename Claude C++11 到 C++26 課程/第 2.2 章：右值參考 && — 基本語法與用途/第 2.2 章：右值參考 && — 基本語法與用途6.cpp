#include <iostream>
#include <string>
#include <utility>

void inner(const std::string& s) { std::cout << "  inner: const T&\n"; }
void inner(std::string&& s)      { std::cout << "  inner: T&&\n"; }

void outer(std::string&& s) {
    // s 的「型別」是 std::string&&（右值參考）
    // 但 s 這個「表達式」是左值（有名字）

    std::cout << "直接傳 s:\n";
    inner(s);              // 呼叫 const T& 版本！因為 s 是左值

    std::cout << "傳 std::move(s):\n";
    inner(std::move(s));   // 呼叫 T&& 版本，因為 std::move(s) 是右值
}

int main() {
    outer(std::string("Hello"));
    return 0;
}
