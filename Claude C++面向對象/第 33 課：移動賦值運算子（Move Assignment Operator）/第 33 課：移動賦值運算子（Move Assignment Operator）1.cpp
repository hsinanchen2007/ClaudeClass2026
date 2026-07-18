// lesson33_move_assignment.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson33 lesson33_move_assignment.cpp
// 驗證：valgrind --leak-check=full ./lesson33

#include <iostream>
#include <cstring>
#include <utility>

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

    // ──────── 解構函數 ────────
    ~SimpleString() {
        if (m_data)
            std::cout << "  [解構] \"" << m_data << "\"\n";
        else
            std::cout << "  [解構] (nullptr)\n";
        delete[] m_data;
    }

    // ──────── 拷貝建構函數 ────────
    SimpleString(const SimpleString& other)
        : m_len(other.m_len)
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // ──────── 移動建構函數 ────────
    SimpleString(SimpleString&& other) noexcept
        : m_data(other.m_data)
        , m_len(other.m_len)
    {
        other.m_data = nullptr;
        other.m_len = 0;
        std::cout << "  [移動建構] \"" << m_data << "\"\n";
    }

    // ──────── 拷貝賦值運算子 ────────
    SimpleString& operator=(const SimpleString& other) {
        std::cout << "  [拷貝賦值] ";
        if (this == &other) {
            std::cout << "自我賦值，跳過\n";
            return *this;
        }

        // 先配置再釋放（例外安全）
        char* newData = new char[other.m_len + 1];
        std::strcpy(newData, other.m_data);

        delete[] m_data;
        m_data = newData;
        m_len = other.m_len;

        std::cout << "\"" << m_data << "\"\n";
        return *this;
    }

    // ════════════════════════════════════════════════
    // ★ 移動賦值運算子 ★
    // ════════════════════════════════════════════════
    SimpleString& operator=(SimpleString&& other) noexcept {
        std::cout << "  [移動賦值] ";
        if (this == &other) {
            std::cout << "自我賦值，跳過\n";
            return *this;
        }

        // 釋放舊資源
        delete[] m_data;

        // 偷走 other 的資源
        m_data = other.m_data;
        m_len  = other.m_len;

        // 置空 other
        other.m_data = nullptr;
        other.m_len  = 0;

        std::cout << "\"" << m_data << "\"\n";
        return *this;
    }

    // ──────── swap ────────
    void swap(SimpleString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ──────── 存取 ────────
    const char* c_str() const { return m_data ? m_data : "(null)"; }
    std::size_t length() const { return m_len; }
};

int main() {
    std::cout << "===== 測試 1：拷貝賦值（左值）=====\n";
    {
        SimpleString a("Dragon");
        SimpleString b("Knight");
        std::cout << "  執行 b = a:\n";
        b = a;   // a 是左值 → 拷貝賦值
        std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 2：移動賦值（右值）=====\n";
    {
        SimpleString a("Wizard");
        SimpleString b("Archer");
        std::cout << "  執行 b = std::move(a):\n";
        b = std::move(a);   // std::move(a) 是右值 → 移動賦值
        std::cout << "  a=\"" << a.c_str() << "\" (已被移動)\n";
        std::cout << "  b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 3：從暫時物件移動賦值 =====\n";
    {
        SimpleString a("Rogue");
        std::cout << "  執行 a = SimpleString(\"Phoenix\"):\n";
        a = SimpleString("Phoenix");  // 暫時物件是右值 → 移動賦值
        std::cout << "  a=\"" << a.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 4：自我移動賦值 =====\n";
    {
        SimpleString a("Paladin");
        std::cout << "  執行 a = std::move(a):\n";
        a = std::move(a);   // 自我賦值檢查保護
        std::cout << "  a=\"" << a.c_str() << "\" (安全)\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 5：鏈式移動賦值 =====\n";
    {
        SimpleString a("Alpha");
        SimpleString b("Beta");
        SimpleString c("Gamma");
        std::cout << "  執行 c = b = std::move(a):\n";
        c = b = std::move(a);
        // 分解：b = std::move(a) → 移動賦值，回傳 b（左值）
        //       c = b           → 拷貝賦值（因為 b 是左值！）
        std::cout << "  a=\"" << a.c_str() << "\"\n";
        std::cout << "  b=\"" << b.c_str() << "\"\n";
        std::cout << "  c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 6：移動賦值給被移動過的物件 =====\n";
    {
        SimpleString a("First");
        SimpleString b("Second");
        SimpleString c("Third");

        std::cout << "  步驟 1：b = std::move(a)\n";
        b = std::move(a);      // a 被掏空
        std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";

        std::cout << "  步驟 2：a = std::move(c)  (對被移動過的 a 重新移動賦值)\n";
        a = std::move(c);      // a 重新獲得資源
        std::cout << "  a=\"" << a.c_str() << "\"  c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 所有測試完成 =====\n";
    return 0;
}
