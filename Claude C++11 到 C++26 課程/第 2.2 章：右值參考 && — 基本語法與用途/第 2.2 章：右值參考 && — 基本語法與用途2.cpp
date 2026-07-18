#include <iostream>

void show_binding() {
    int x = 10;

    // T&：只能綁定左值
    int& a = x;            // OK
    // int& b = 42;        // 錯誤

    // const T&：左值右值都能綁定
    const int& c = x;      // OK：綁定左值
    const int& d = 42;     // OK：綁定右值（延長臨時物件生命週期）

    // T&&：只能綁定右值
    // int&& e = x;        // 錯誤：x 是左值
    int&& f = 42;          // OK：綁定右值
    int&& g = x + 1;       // OK：x + 1 的結果是右值

    // 如果硬要讓右值參考綁定左值，用 std::move
    int&& h = std::move(x);  // OK：std::move(x) 是 xvalue（右值）

    std::cout << "所有綁定成功\n";
}

int main() {
    show_binding();
    return 0;
}
