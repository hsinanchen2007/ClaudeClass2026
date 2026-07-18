// lesson32_move_constructor.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson32 lesson32_move_constructor.cpp

#include <iostream>
#include <cstring>
#include <utility>  // std::move, std::swap

class SimpleString {
private:
    char* m_data;
    std::size_t m_len;

public:
    // ──────── 一般建構函數 ────────
    SimpleString(const char* str = "")
        : m_len(std::strlen(str))
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\" @ "
                  << static_cast<void*>(m_data) << "\n";
    }

    // ──────── 拷貝建構函數（深拷貝）────────
    SimpleString(const SimpleString& other)
        : m_len(other.m_len)
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\" 新 @ "
                  << static_cast<void*>(m_data) << "\n";
    }

    // ════════════════════════════════════════════
    // ★ 移動建構函數 ★
    // ════════════════════════════════════════════
    SimpleString(SimpleString&& other) noexcept
        : m_data(other.m_data)     // 步驟 1：偷走指標
        , m_len(other.m_len)       // 步驟 3：複製基本型別
    {
        other.m_data = nullptr;    // 步驟 2：置空來源
        other.m_len = 0;

        std::cout << "  [移動建構] \"" << m_data << "\" 偷自 @ "
                  << static_cast<void*>(m_data)
                  << " (other 現在是 nullptr)\n";
    }

    // ──────── 拷貝賦值運算子（Copy-and-Swap）────────
    SimpleString& operator=(SimpleString other) {
        std::cout << "  [賦值] swap\n";
        swap(other);
        return *this;
    }

    // ──────── 解構函數 ────────
    ~SimpleString() {
        if (m_data) {
            std::cout << "  [解構] \"" << m_data << "\" @ "
                      << static_cast<void*>(m_data) << "\n";
        } else {
            std::cout << "  [解構] (已被移動，nullptr)\n";
        }
        delete[] m_data;  // delete nullptr 是安全的
    }

    // ──────── swap ────────
    void swap(SimpleString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ──────── 存取 ────────
    const char* c_str() const { return m_data ? m_data : "(null)"; }
    std::size_t length() const { return m_len; }
    bool empty() const { return m_data == nullptr || m_len == 0; }
};

int main() {
    std::cout << "===== 測試 1：從暫時物件移動建構 =====\n";
    SimpleString a = SimpleString("Dragon Sword");
    // SimpleString("Dragon Sword") 是右值 → 呼叫移動建構函數
    std::cout << "  a = \"" << a.c_str() << "\"\n\n";

    std::cout << "===== 測試 2：拷貝建構（左值）=====\n";
    SimpleString b = a;  // a 是左值 → 呼叫拷貝建構函數
    std::cout << "  a = \"" << a.c_str() << "\"\n";
    std::cout << "  b = \"" << b.c_str() << "\"\n\n";

    std::cout << "===== 測試 3：用 std::move 強制移動 =====\n";
    SimpleString c = std::move(a);  // std::move(a) 是右值 → 移動建構
    std::cout << "  a = \"" << a.c_str() << "\" (已被移動)\n";
    std::cout << "  c = \"" << c.c_str() << "\"\n\n";

    std::cout << "===== 測試 4：move 後的物件可以重新賦值 =====\n";
    a = SimpleString("Phoenix Staff");  // 對被移動的物件重新賦值
    std::cout << "  a = \"" << a.c_str() << "\" (重生了！)\n\n";

    std::cout << "===== 開始解構 =====\n";
    return 0;
}
