#include <iostream>

void process(int x)    { std::cout << "int: " << x << "\n"; }
void process(double x) { std::cout << "double: " << x << "\n"; }

template<typename T>
void wrapper(T&& arg) {
    // ...
}

int main() {
    // wrapper(process);
    // 錯誤！process 是重載函式，編譯器不知道要選哪一個
    // T 無法推導

    // 解決方法 1：明確指定型別
    wrapper(static_cast<void(*)(int)>(process));

    // 解決方法 2：用 Lambda 包裝
    wrapper([](int x) { process(x); });

    return 0;
}
