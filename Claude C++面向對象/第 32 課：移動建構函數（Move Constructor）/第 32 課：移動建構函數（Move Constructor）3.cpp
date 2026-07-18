// lesson32_auto_generation.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson32c lesson32_auto_generation.cpp

#include <iostream>
#include <string>
#include <type_traits>

// 情況 1：什麼都沒寫 → 編譯器自動生成移動建構
class AutoMove {
    std::string m_name;
    int m_value;
public:
    AutoMove(std::string name, int val) : m_name(std::move(name)), m_value(val) {}
};

// 情況 2：寫了解構函數 → 編譯器不自動生成移動建構
class NoAutoMove {
    std::string m_name;
    int m_value;
public:
    NoAutoMove(std::string name, int val) : m_name(std::move(name)), m_value(val) {}
    ~NoAutoMove() {}  // 自定義解構 → 移動建構不會自動生成
};

int main() {
    std::cout << std::boolalpha;

    std::cout << "AutoMove 可移動建構？ "
              << std::is_move_constructible_v<AutoMove> << "\n";
    std::cout << "AutoMove 有 nothrow 移動建構？ "
              << std::is_nothrow_move_constructible_v<AutoMove> << "\n";

    std::cout << "\nNoAutoMove 可移動建構？ "
              << std::is_move_constructible_v<NoAutoMove> << "\n";
    std::cout << "NoAutoMove 有 nothrow 移動建構？ "
              << std::is_nothrow_move_constructible_v<NoAutoMove> << "\n";
    // NoAutoMove 仍然「可移動建構」，但會退回使用拷貝建構（const T&）
    // 沒有真正的移動建構函數

    return 0;
}
