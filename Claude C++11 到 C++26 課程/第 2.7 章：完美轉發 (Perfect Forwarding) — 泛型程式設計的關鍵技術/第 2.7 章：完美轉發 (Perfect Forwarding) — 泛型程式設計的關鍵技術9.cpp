#include <iostream>

class Config {
public:
    static const int MAX_SIZE = 100;  // 僅宣告，未定義
};

void target(const int& n) {
    std::cout << "n = " << n << "\n";
}

template<typename T>
void wrapper(T&& arg) {
    target(std::forward<T>(arg));
}

int main() {
    target(Config::MAX_SIZE);   // OK：const int& 綁定到值

    // wrapper(Config::MAX_SIZE);
    // 可能出現連結錯誤！
    // T&& 推導為 const int&，需要取址
    // 但 MAX_SIZE 只有宣告沒有定義，沒有地址

    // 解決方法：提供定義
    // const int Config::MAX_SIZE;  // 在 .cpp 檔中

    // 或用 C++17 的 inline variable：
    // static inline const int MAX_SIZE = 100;

    // 或用 constexpr（C++17 起隱含 inline）：
    // static constexpr int MAX_SIZE = 100;

    return 0;
}
