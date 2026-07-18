// lesson28_copy_constructor.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson28 lesson28_copy_constructor.cpp

#include <iostream>
#include <cstring>

class SimpleString {
private:
    char* m_data;
    std::size_t m_len;

public:
    // ──────── 一般建構函數 ────────
    SimpleString(const char* str = "") {
        m_len = std::strlen(str);
        m_data = new char[m_len + 1];
        std::strcpy(m_data, str);
        std::cout << "  [一般建構] \"" << m_data << "\"\n";
    }

    // ──────── 拷貝建構函數 ────────
    SimpleString(const SimpleString& other)
        : m_len(other.m_len)                     // 用初始化列表複製長度
        , m_data(new char[other.m_len + 1])      // 配置新記憶體
    {
        std::strcpy(m_data, other.m_data);       // 複製內容
        std::cout << "  [拷貝建構] \"" << m_data
                  << "\" (新 @ " << static_cast<void*>(m_data)
                  << ", 源 @ " << static_cast<const void*>(other.m_data) << ")\n";
    }

    // ──────── 拷貝賦值運算子 ────────
    SimpleString& operator=(const SimpleString& other) {
        std::cout << "  [拷貝賦值] ";
        if (this == &other) {
            std::cout << "自我賦值，跳過\n";
            return *this;
        }
        delete[] m_data;
        m_len = other.m_len;
        m_data = new char[m_len + 1];
        std::strcpy(m_data, other.m_data);
        std::cout << "\"" << m_data << "\"\n";
        return *this;
    }

    // ──────── 解構函數 ────────
    ~SimpleString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
};

// ──────── 測試用的函數 ────────

// 按值傳參 → 觸發拷貝建構
void passByValue(SimpleString s) {
    std::cout << "    函數內：s = \"" << s.c_str() << "\"\n";
}
// 函數結束時 s 被解構

// 按 const 引用傳參 → 不觸發拷貝建構
void passByConstRef(const SimpleString& s) {
    std::cout << "    函數內：s = \"" << s.c_str() << "\"\n";
}

// 按值返回 → 可能觸發拷貝建構（也可能被優化掉）
SimpleString createString() {
    std::cout << "  createString() 內部：\n";
    SimpleString local("Created Inside");
    std::cout << "  即將 return local\n";
    return local;
}

int main() {
    std::cout << "===== 情境 1：直接初始化 =====\n";
    SimpleString a("Alpha");
    SimpleString b(a);         // 拷貝建構
    std::cout << "\n";

    std::cout << "===== 情境 2：拷貝初始化（= 語法）=====\n";
    SimpleString c = a;        // 拷貝建構（不是賦值！）
    std::cout << "\n";

    std::cout << "===== 情境 3：按值傳參 =====\n";
    std::cout << "  呼叫 passByValue(a):\n";
    passByValue(a);            // 拷貝建構 → 函數結束時解構副本
    std::cout << "\n";

    std::cout << "===== 情境 4：按 const 引用傳參 =====\n";
    std::cout << "  呼叫 passByConstRef(a):\n";
    passByConstRef(a);         // 沒有拷貝建構！
    std::cout << "\n";

    std::cout << "===== 情境 5：按值返回 =====\n";
    SimpleString d = createString();
    std::cout << "  d = \"" << d.c_str() << "\"\n";
    std::cout << "\n";

    std::cout << "===== 情境 6：賦值（不是拷貝建構！）=====\n";
    SimpleString e("Echo");
    std::cout << "  e = a（拷貝賦值，不是拷貝建構）:\n";
    e = a;                     // 拷貝賦值運算子，因為 e 已經存在
    std::cout << "\n";

    std::cout << "===== 開始解構 =====\n";
    return 0;
}
