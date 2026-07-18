// lesson29_copy_assignment.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson29 lesson29_copy_assignment.cpp

#include <iostream>
#include <cstring>
#include <utility>  // std::swap

class SimpleString {
private:
    char* m_data;
    std::size_t m_len;

public:
    // ──────── 建構函數 ────────
    SimpleString(const char* str = "")
        : m_len(std::strlen(str))
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    // ──────── 拷貝建構函數 ────────
    SimpleString(const SimpleString& other)
        : m_len(other.m_len)
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // ──────── 拷貝賦值運算子（Copy-and-Swap）────────
    SimpleString& operator=(SimpleString other) {  // 按值傳入！
        std::cout << "  [拷貝賦值] swap with \"" << other.m_data << "\"\n";
        swap(other);       // 交換內容
        return *this;      // other 帶著舊資料離開並解構
    }

    // ──────── 解構函數 ────────
    ~SimpleString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    // ──────── swap ────────
    void swap(SimpleString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ──────── 存取 ────────
    const char* c_str() const { return m_data; }
    std::size_t length() const { return m_len; }
};

int main() {
    std::cout << "===== 測試 1：基本拷貝賦值 =====\n";
    SimpleString a("Dragon");
    SimpleString b("Knight");
    std::cout << "  執行 b = a:\n";
    b = a;
    std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n\n";

    std::cout << "===== 測試 2：自我賦值 =====\n";
    std::cout << "  執行 a = a:\n";
    a = a;
    std::cout << "  a=\"" << a.c_str() << "\"（安全！）\n\n";

    std::cout << "===== 測試 3：鏈式賦值 =====\n";
    SimpleString c("Wizard");
    SimpleString d("Archer");
    std::cout << "  執行 d = c = a:\n";
    d = c = a;
    std::cout << "  a=\"" << a.c_str() << "\"  c=\"" << c.c_str()
              << "\"  d=\"" << d.c_str() << "\"\n\n";

    std::cout << "===== 測試 4：用暫時物件賦值 =====\n";
    SimpleString e("Rogue");
    std::cout << "  執行 e = SimpleString(\"Phoenix\"):\n";
    e = SimpleString("Phoenix");
    std::cout << "  e=\"" << e.c_str() << "\"\n\n";

    std::cout << "===== 開始解構 =====\n";
    return 0;
}
