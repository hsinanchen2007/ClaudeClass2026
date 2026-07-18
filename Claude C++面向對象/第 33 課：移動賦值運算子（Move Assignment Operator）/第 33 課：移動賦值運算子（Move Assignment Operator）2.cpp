// lesson33_unified.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson33b lesson33_unified.cpp

#include <iostream>
#include <cstring>
#include <utility>

class UnifiedString {
private:
    char* m_data;
    std::size_t m_len;

public:
    // 建構
    UnifiedString(const char* str = "")
        : m_len(std::strlen(str))
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    // 解構
    ~UnifiedString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "null") << "\"\n";
        delete[] m_data;
    }

    // 拷貝建構
    UnifiedString(const UnifiedString& other)
        : m_len(other.m_len)
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // 移動建構
    UnifiedString(UnifiedString&& other) noexcept
        : m_data(other.m_data), m_len(other.m_len)
    {
        other.m_data = nullptr;
        other.m_len = 0;
        std::cout << "  [移動建構] \"" << m_data << "\"\n";
    }

    // ★ 統一的賦值運算子（同時處理拷貝和移動）
    UnifiedString& operator=(UnifiedString other) {  // 按值傳入
        std::cout << "  [賦值] swap\n";
        swap(other);
        return *this;
    }

    // swap
    void swap(UnifiedString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

int main() {
    UnifiedString a("Dragon");
    UnifiedString b("Knight");

    std::cout << "\n--- 拷貝賦值路徑 ---\n";
    b = a;                          // a 是左值 → other 由拷貝建構 → swap

    std::cout << "\n--- 移動賦值路徑 ---\n";
    b = std::move(a);               // 右值 → other 由移動建構 → swap

    std::cout << "\n--- 暫時物件路徑 ---\n";
    b = UnifiedString("Phoenix");   // 暫時物件 → other 由移動建構 → swap

    std::cout << "\n--- 結束 ---\n";
    return 0;
}
