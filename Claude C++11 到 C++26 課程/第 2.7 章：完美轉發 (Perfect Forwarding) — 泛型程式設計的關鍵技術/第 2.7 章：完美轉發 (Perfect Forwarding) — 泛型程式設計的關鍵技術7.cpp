#include <iostream>
#include <vector>
#include <utility>

void target(std::vector<int> v) {
    std::cout << "收到 " << v.size() << " 個元素\n";
}

template<typename T>
void wrapper(T&& arg) {
    target(std::forward<T>(arg));
}

int main() {
    target({1, 2, 3});      // OK：直接呼叫，編譯器推導為 initializer_list

    // wrapper({1, 2, 3});  // 錯誤！模板無法推導 {1, 2, 3} 的型別
                             // {1, 2, 3} 不是一個表達式，沒有型別

    // 解決方法 1：明確建構
    wrapper(std::vector<int>{1, 2, 3});  // OK

    // 解決方法 2：先存成變數
    auto init = {1, 2, 3};  // auto 可以推導 initializer_list
    // 但 wrapper(init) 傳的是 initializer_list，不是 vector

    return 0;
}
