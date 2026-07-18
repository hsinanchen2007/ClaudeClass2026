#include <iostream>
#include <cstddef>

void target(int* ptr) {
    std::cout << "收到指標: " << ptr << "\n";
}

template<typename T>
void wrapper(T&& arg) {
    target(std::forward<T>(arg));
}

int main() {
    target(0);            // OK：0 隱式轉換為 int*
    target(NULL);         // OK：NULL 通常是 0 或 0L
    target(nullptr);      // OK

    // wrapper(0);         // 錯誤！T 推導為 int，int 不能轉為 int*
    // wrapper(NULL);      // 可能錯誤，取決於 NULL 的定義
    wrapper(nullptr);      // OK：T 推導為 std::nullptr_t

    return 0;
}
