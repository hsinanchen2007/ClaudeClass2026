// ============================================================
// 第 33 課 總結：移動賦值運算子（Move Assignment Operator）
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【簽名】ClassName& operator=(ClassName&& other) noexcept
//
// 【移動賦值步驟（分開寫法）】
//   1. 自我賦值檢查：if (this == &other) return *this;
//   2. 釋放自己的舊資源：delete[] m_data;
//   3. 偷取 other 的資源：m_data = other.m_data;
//   4. 歸零 other：other.m_data = nullptr;
//   5. 回傳 *this
//
// 【統一賦值運算子（推薦）】
//   ClassName& operator=(ClassName other) {  // 傳值！
//       swap(*this, other);
//       return *this;
//   }
//   一個函數同時處理拷貝賦值和移動賦值！
//   傳左值 → 觸發拷貝建構 → swap（拷貝賦值路徑）
//   傳右值 → 觸發移動建構 → swap（移動賦值路徑）
//
// 【拷貝賦值 vs 移動賦值】
//   b = a;            // a 是左值 → 拷貝賦值
//   b = std::move(a); // std::move(a) 是右值 → 移動賦值
//   b = func();       // func() 回傳臨時物件 → 移動賦值
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>

// ============================================================
// 方法一：分開寫拷貝賦值和移動賦值
// ============================================================
class SeparateString {
    char* m_data;
    std::size_t m_len;
public:
    SeparateString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    ~SeparateString() {
        if (m_data) std::cout << "  [解構] \"" << m_data << "\"\n";
        else        std::cout << "  [解構] (nullptr)\n";
        delete[] m_data;
    }

    // 拷貝建構
    SeparateString(const SeparateString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // 移動建構
    SeparateString(SeparateString&& other) noexcept
        : m_data(other.m_data), m_len(other.m_len) {
        other.m_data = nullptr; other.m_len = 0;
        std::cout << "  [移動建構] \"" << m_data << "\"\n";
    }

    // ★ 拷貝賦值運算子
    SeparateString& operator=(const SeparateString& other) {
        std::cout << "  [拷貝賦值] ";
        if (this == &other) { std::cout << "自我賦值\n"; return *this; }
        char* newData = new char[other.m_len + 1];  // 先配置再釋放（異常安全）
        std::strcpy(newData, other.m_data);
        delete[] m_data;
        m_data = newData;
        m_len = other.m_len;
        std::cout << "\"" << m_data << "\"\n";
        return *this;
    }

    // ★ 移動賦值運算子
    SeparateString& operator=(SeparateString&& other) noexcept {
        std::cout << "  [移動賦值] ";
        if (this == &other) { std::cout << "自我賦值\n"; return *this; }
        delete[] m_data;             // 釋放舊資源
        m_data = other.m_data;       // 偷取
        m_len = other.m_len;
        other.m_data = nullptr;      // 歸零
        other.m_len = 0;
        std::cout << "\"" << m_data << "\"\n";
        return *this;
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

// ============================================================
// 方法二：統一賦值運算子（推薦）
// ============================================================
class UnifiedString {
    char* m_data;
    std::size_t m_len;
public:
    UnifiedString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    ~UnifiedString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "null") << "\"\n";
        delete[] m_data;
    }

    UnifiedString(const UnifiedString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    UnifiedString(UnifiedString&& other) noexcept
        : m_data(other.m_data), m_len(other.m_len) {
        other.m_data = nullptr; other.m_len = 0;
        std::cout << "  [移動建構] \"" << m_data << "\"\n";
    }

    void swap(UnifiedString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ★ 統一賦值：一個函數處理拷貝和移動
    UnifiedString& operator=(UnifiedString other) {  // 傳值！
        std::cout << "  [統一賦值] swap\n";
        swap(other);        // 交換內容
        return *this;       // other 解構時釋放舊資料
    }
    // 傳左值 → other 由拷貝建構 → swap（拷貝路徑）
    // 傳右值 → other 由移動建構 → swap（移動路徑）

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

int main() {
    // ============================================================
    // 1. 分開寫法：拷貝賦值 vs 移動賦值
    // ============================================================
    std::cout << "===== 1. 分開寫法 =====\n";
    {
        SeparateString a("Dragon");
        SeparateString b("Knight");

        std::cout << "\n  拷貝賦值（左值）：b = a\n";
        b = a;

        std::cout << "\n  移動賦值（右值）：b = std::move(a)\n";
        b = std::move(a);
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        std::cout << "\n  移動賦值（臨時物件）：b = SeparateString(\"Phoenix\")\n";
        b = SeparateString("Phoenix");
        std::cout << "  b=\"" << b.c_str() << "\"\n";

        std::cout << "\n  自我移動賦值：\n";
        b = std::move(b);  // 自我賦值檢查保護
    }
    std::cout << "\n";

    // ============================================================
    // 2. 鏈式移動賦值
    // ============================================================
    std::cout << "===== 2. 鏈式移動賦值 =====\n";
    {
        SeparateString a("Alpha"), b("Beta"), c("Gamma");
        std::cout << "  c = b = std::move(a):\n";
        c = b = std::move(a);
        // 分解：b = std::move(a) → 移動賦值，回傳 b（左值！）
        //       c = b           → 拷貝賦值（因為 b 是左值）
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str()
                  << "\" c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 3. 統一賦值運算子
    // ============================================================
    std::cout << "===== 3. 統一賦值（推薦）=====\n";
    {
        UnifiedString a("Dragon");
        UnifiedString b("Knight");

        std::cout << "\n  拷貝路徑：b = a（左值 → 拷貝建構 other → swap）\n";
        b = a;

        std::cout << "\n  移動路徑：b = std::move(a)（右值 → 移動建構 other → swap）\n";
        b = std::move(a);

        std::cout << "\n  臨時物件路徑：\n";
        b = UnifiedString("Phoenix");
    }
    std::cout << "\n";

    // ============================================================
    // 4. 對被移動物件重新賦值
    // ============================================================
    std::cout << "===== 4. 被移動物件可以重新賦值 =====\n";
    {
        SeparateString a("First"), b("Second"), c("Third");
        b = std::move(a);
        std::cout << "  a 被移動後：\"" << a.c_str() << "\"\n";
        a = std::move(c);  // 對被移動的 a 重新移動賦值
        std::cout << "  a 重新賦值：\"" << a.c_str() << "\"\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  分開寫：operator=(const T&) + operator=(T&&)\n";
    std::cout << "  統一寫（推薦）：operator=(T other) + swap\n";
    std::cout << "  統一寫自動根據傳入左/右值選擇拷貝/移動路徑\n";
    std::cout << "  鏈式賦值中 b = std::move(a) 回傳的 b 是左值\n";

    return 0;
}
