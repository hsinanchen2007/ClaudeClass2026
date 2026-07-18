// ============================================================
// 第 30 課 總結：三法則（Rule of Three）
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【三法則】如果類別需要自訂以下任何一個，通常三個都需要自訂：
//   1. 解構函數（Destructor）           ~ClassName()
//   2. 拷貝建構函數（Copy Constructor）  ClassName(const ClassName&)
//   3. 拷貝賦值運算子（Copy Assignment） ClassName& operator=(const ClassName&)
//
// 【為什麼？】
//   需要自訂解構函數 → 表示類別管理了動態資源（new 出來的記憶體）
//   → 管理動態資源 → 預設的淺拷貝不安全
//   → 所以拷貝建構和拷貝賦值也必須自訂（做深拷貝）
//
// 【判斷依據】
//   類別有 new/malloc/file handle → 需要三法則
//   類別只用 string/vector/unique_ptr → 不需要（Rule of Zero）
//
// 【推薦實作模式】
//   解構函數：delete 資源
//   拷貝建構：new + 複製
//   拷貝賦值：Copy-and-Swap 慣用法
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>

// ============================================================
// 完整的 Rule of Three 示範：ManagedString
// ============================================================
class ManagedString {
    char* m_data;
    std::size_t m_len;

public:
    // 一般建構函數
    ManagedString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\" @"
                  << static_cast<void*>(m_data) << "\n";
    }

    // ═══════════════════════════════════════
    // ★ Rule of Three：以下三個必須同時存在 ★
    // ═══════════════════════════════════════

    // ① 解構函數 — 釋放動態資源
    ~ManagedString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr")
                  << "\" @" << static_cast<void*>(m_data) << "\n";
        delete[] m_data;
    }

    // ② 拷貝建構函數 — 深拷貝
    ManagedString(const ManagedString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\" @"
                  << static_cast<void*>(m_data) << "\n";
    }

    // ③ 拷貝賦值運算子 — Copy-and-Swap
    void swap(ManagedString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    ManagedString& operator=(ManagedString other) {  // 傳值
        std::cout << "  [拷貝賦值] swap\n";
        swap(other);
        return *this;
    }

    // ═══════════════════════════════════════
    // ★ Rule of Three 結束 ★
    // ═══════════════════════════════════════

    const char* c_str() const { return m_data; }
    std::size_t length() const { return m_len; }
};

// 非成員 swap
inline void swap(ManagedString& a, ManagedString& b) noexcept { a.swap(b); }

// 按值傳參 → 觸發拷貝建構
void processByValue(ManagedString s) {
    std::cout << "    processByValue: \"" << s.c_str() << "\"\n";
}

// 按值回傳
ManagedString createString(const char* text) {
    ManagedString local(text);
    return local;
}

int main() {
    // ============================================================
    // 測試 1：基本生命週期
    // ============================================================
    std::cout << "===== 測試 1：基本生命週期 =====\n";
    {
        ManagedString s("Hello");
        std::cout << "  s = \"" << s.c_str() << "\"\n";
    } // s 離開作用域 → 解構
    std::cout << "\n";

    // ============================================================
    // 測試 2：拷貝建構（深拷貝）
    // ============================================================
    std::cout << "===== 測試 2：拷貝建構 =====\n";
    {
        ManagedString a("Dragon");
        ManagedString b = a;  // 拷貝建構
        std::cout << "  a @" << static_cast<const void*>(a.c_str())
                  << "  b @" << static_cast<const void*>(b.c_str()) << "\n";
        std::cout << "  （位址不同 → 深拷貝成功）\n";
    }
    std::cout << "\n";

    // ============================================================
    // 測試 3：拷貝賦值（Copy-and-Swap）
    // ============================================================
    std::cout << "===== 測試 3：拷貝賦值 =====\n";
    {
        ManagedString a("Knight");
        ManagedString b("Wizard");
        std::cout << "  賦值前：a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";
        b = a;
        std::cout << "  賦值後：a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 測試 4：自我賦值安全性
    // ============================================================
    std::cout << "===== 測試 4：自我賦值 =====\n";
    {
        ManagedString a("Phoenix");
        a = a;  // Copy-and-Swap 自動處理
        std::cout << "  a=\"" << a.c_str() << "\"（安全）\n";
    }
    std::cout << "\n";

    // ============================================================
    // 測試 5：按值傳參 & 回傳
    // ============================================================
    std::cout << "===== 測試 5：按值傳參 =====\n";
    {
        ManagedString a("Rogue");
        processByValue(a);
        std::cout << "  返回後 a=\"" << a.c_str() << "\"（不受影響）\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 6：按值回傳 =====\n";
    {
        ManagedString result = createString("Summoned");
        std::cout << "  result=\"" << result.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 測試 7：鏈式賦值
    // ============================================================
    std::cout << "===== 測試 7：鏈式賦值 =====\n";
    {
        ManagedString a("A"), b("B"), c("C");
        c = b = a;
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str()
                  << "\" c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 重點整理
    // ============================================================
    std::cout << "=== 三法則速查 ===\n";
    std::cout << "  需要自訂解構函數？ → 也需要拷貝建構 + 拷貝賦值\n";
    std::cout << "  判斷：類別是否管理動態資源？\n";
    std::cout << "    是 → 實作三法則（解構 + 拷貝建構 + 拷貝賦值）\n";
    std::cout << "    否（只用 string/vector/unique_ptr）→ Rule of Zero\n";

    return 0;
}
