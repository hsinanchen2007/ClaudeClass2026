// lesson27_deep_copy.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o deep lesson27_deep_copy.cpp
// 驗證無記憶體問題：valgrind ./deep

#include <iostream>
#include <cstring>

class SimpleString {
private:
    char* m_data;
    std::size_t m_len;

public:
    // ──────── 建構函數 ────────
    SimpleString(const char* str = "") {
        m_len = std::strlen(str);
        m_data = new char[m_len + 1];
        std::strcpy(m_data, str);
        std::cout << "  [建構] @ " << static_cast<void*>(m_data)
                  << " \"" << m_data << "\"\n";
    }

    // ──────── 拷貝建構函數（深拷貝）────────
    // 當用一個已存在的物件來初始化新物件時呼叫
    SimpleString(const SimpleString& other) {
        m_len = other.m_len;
        m_data = new char[m_len + 1];      // ★ 配置新的記憶體
        std::strcpy(m_data, other.m_data);  // ★ 複製內容，而非位址
        std::cout << "  [深拷貝建構] 新記憶體 @ " << static_cast<void*>(m_data)
                  << " 從 @ " << static_cast<const void*>(other.m_data)
                  << " 複製 \"" << m_data << "\"\n";
    }

    // ──────── 拷貝賦值運算子（深拷貝）────────
    // 當用 = 把一個已存在的物件賦值給另一個已存在的物件時呼叫
    SimpleString& operator=(const SimpleString& other) {
        std::cout << "  [深拷貝賦值]\n";

        // ★ 步驟 1：檢查自我賦值
        if (this == &other) {
            std::cout << "    自我賦值，跳過\n";
            return *this;
        }

        // ★ 步驟 2：釋放舊資源
        delete[] m_data;

        // ★ 步驟 3：配置新記憶體並複製
        m_len = other.m_len;
        m_data = new char[m_len + 1];
        std::strcpy(m_data, other.m_data);

        std::cout << "    新記憶體 @ " << static_cast<void*>(m_data)
                  << " 內容 \"" << m_data << "\"\n";

        // ★ 步驟 4：回傳 *this 以支援鏈式賦值（a = b = c）
        return *this;
    }

    // ──────── 解構函數 ────────
    ~SimpleString() {
        std::cout << "  [解構] 釋放 @ " << static_cast<void*>(m_data)
                  << " \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    // ──────── 存取函數 ────────
    const char* c_str() const { return m_data; }
    std::size_t length() const { return m_len; }

    void setFirstChar(char ch) {
        if (m_len > 0) m_data[0] = ch;
    }
};

int main() {
    std::cout << "=== 深拷貝示範 ===\n\n";

    // ---- 測試 1：拷貝建構 ----
    std::cout << "【測試 1】拷貝建構\n";
    SimpleString s1("Dragon");

    std::cout << "  建立 s2 = s1（拷貝建構）\n";
    SimpleString s2 = s1;   // 呼叫拷貝建構函數

    std::cout << "  s1 指向: " << static_cast<const void*>(s1.c_str()) << "\n";
    std::cout << "  s2 指向: " << static_cast<const void*>(s2.c_str()) << "\n";
    std::cout << "  （位址不同 → 各自獨立！）\n";

    s2.setFirstChar('X');
    std::cout << "  修改 s2 後：s1=\"" << s1.c_str()
              << "\"  s2=\"" << s2.c_str() << "\"\n";
    std::cout << "  （s1 不受影響）\n\n";

    // ---- 測試 2：拷貝賦值 ----
    std::cout << "【測試 2】拷貝賦值\n";
    SimpleString s3("Knight");
    std::cout << "  執行 s3 = s1\n";
    s3 = s1;   // 呼叫拷貝賦值運算子

    std::cout << "  s1=\"" << s1.c_str()
              << "\"  s3=\"" << s3.c_str() << "\"\n\n";

    // ---- 測試 3：自我賦值安全性 ----
    std::cout << "【測試 3】自我賦值\n";
    s1 = s1;   // 應安全處理
    std::cout << "  s1=\"" << s1.c_str() << "\"（安全！）\n\n";

    // ---- 測試 4：鏈式賦值 ----
    std::cout << "【測試 4】鏈式賦值\n";
    SimpleString s4("Wizard");
    SimpleString s5("Archer");
    std::cout << "  執行 s5 = s4 = s1\n";
    s5 = s4 = s1;
    std::cout << "  s1=\"" << s1.c_str() << "\"  s4=\"" << s4.c_str()
              << "\"  s5=\"" << s5.c_str() << "\"\n\n";

    std::cout << "=== 離開 main，開始解構 ===\n";
    return 0;
}
