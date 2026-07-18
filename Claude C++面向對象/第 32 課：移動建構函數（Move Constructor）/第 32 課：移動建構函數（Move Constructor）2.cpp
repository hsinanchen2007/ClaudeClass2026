// lesson32_noexcept.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson32b lesson32_noexcept.cpp

#include <iostream>
#include <vector>
#include <cstring>

class Widget {
private:
    char* m_name;

public:
    Widget(const char* name) : m_name(new char[std::strlen(name) + 1]) {
        std::strcpy(m_name, name);
    }

    // 拷貝建構
    Widget(const Widget& other) : m_name(new char[std::strlen(other.m_name) + 1]) {
        std::strcpy(m_name, other.m_name);
        std::cout << "    [拷貝] \"" << m_name << "\"\n";
    }

    // 移動建構 —— 有 noexcept！
    Widget(Widget&& other) noexcept : m_name(other.m_name) {
        other.m_name = nullptr;
        std::cout << "    [移動] \"" << m_name << "\"\n";
    }

    ~Widget() { delete[] m_name; }

    Widget& operator=(Widget other) {
        std::swap(m_name, other.m_name);
        return *this;
    }
};

int main() {
    std::cout << "=== vector 擴容時的行為（有 noexcept）===\n\n";

    std::vector<Widget> vec;
    vec.reserve(2);  // 先預留 2 個空間

    std::cout << "push_back #1:\n";
    vec.push_back(Widget("Alpha"));

    std::cout << "\npush_back #2:\n";
    vec.push_back(Widget("Beta"));

    std::cout << "\npush_back #3（觸發擴容，需要搬移已有元素）:\n";
    vec.push_back(Widget("Gamma"));
    // 因為移動建構有 noexcept → vector 會用移動來搬 Alpha 和 Beta

    std::cout << "\n完成！\n";
    return 0;
}
