// lesson30_rule_of_three.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson30 lesson30_rule_of_three.cpp
// 驗證：valgrind --leak-check=full ./lesson30

#include <iostream>
#include <cstring>
#include <utility>

class ManagedString {
private:
    char* m_data;
    std::size_t m_len;

public:
    // ──────── 建構函數 ────────
    ManagedString(const char* str = "")
        : m_len(std::strlen(str))
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\" @ "
                  << static_cast<void*>(m_data) << "\n";
    }

    // ════════════════════════════════════════
    // ★ Rule of Three：以下三個必須同時存在 ★
    // ════════════════════════════════════════

    // ──────── 1. 解構函數 ────────
    ~ManagedString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr")
                  << "\" @ " << static_cast<void*>(m_data) << "\n";
        delete[] m_data;
    }

    // ──────── 2. 拷貝建構函數 ────────
    ManagedString(const ManagedString& other)
        : m_len(other.m_len)
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\" @ "
                  << static_cast<void*>(m_data) << "\n";
    }

    // ──────── 3. 拷貝賦值運算子（Copy-and-Swap）────────
    ManagedString& operator=(ManagedString other) {
        std::cout << "  [拷貝賦值] swap\n";
        swap(other);
        return *this;
    }

    // ──────── swap（配合 Copy-and-Swap）────────
    void swap(ManagedString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ──────── 存取 ────────
    const char* c_str() const { return m_data; }
    std::size_t length() const { return m_len; }
};

// 非成員 swap
inline void swap(ManagedString& a, ManagedString& b) noexcept {
    a.swap(b);
}

// ──────── 測試函數：按值傳入（觸發拷貝建構）────────
void processByValue(ManagedString s) {
    std::cout << "    processByValue: \"" << s.c_str() << "\"\n";
}

// ──────── 測試函數：按值返回 ────────
ManagedString createString(const char* text) {
    ManagedString local(text);
    return local;
}

int main() {
    std::cout << "===== 測試 1：基本生命週期 =====\n";
    {
        ManagedString s("Hello");
        std::cout << "  s = \"" << s.c_str() << "\"\n";
    }  // s 離開作用域，解構
    std::cout << "\n";

    std::cout << "===== 測試 2：拷貝建構 =====\n";
    {
        ManagedString a("Dragon");
        ManagedString b = a;   // 拷貝建構
        std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";
        std::cout << "  a @ " << static_cast<const void*>(a.c_str())
                  << "  b @ " << static_cast<const void*>(b.c_str()) << "\n";
        std::cout << "  （位址不同 → 深拷貝成功）\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 3：拷貝賦值 =====\n";
    {
        ManagedString a("Knight");
        ManagedString b("Wizard");
        std::cout << "  賦值前：a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";
        b = a;
        std::cout << "  賦值後：a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 4：自我賦值 =====\n";
    {
        ManagedString a("Phoenix");
        a = a;  // Copy-and-Swap 自動處理
        std::cout << "  a=\"" << a.c_str() << "\"（安全）\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 5：按值傳參（觸發拷貝建構）=====\n";
    {
        ManagedString a("Rogue");
        processByValue(a);  // 拷貝建構 → 函數結束時解構副本
        std::cout << "  函數返回後 a=\"" << a.c_str() << "\"（不受影響）\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 6：按值返回 =====\n";
    {
        ManagedString result = createString("Summoned");
        std::cout << "  result=\"" << result.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 7：鏈式賦值 =====\n";
    {
        ManagedString a("A"), b("B"), c("C");
        c = b = a;
        std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str()
                  << "\"  c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 所有測試通過 =====\n";
    return 0;
}
