// ============================================================
// 第 27 課 總結：淺拷貝與深拷貝（Shallow Copy vs Deep Copy）
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【核心觀念】
// 淺拷貝（Shallow Copy）：只複製指標本身 → 兩個物件共享同一塊記憶體
//   → 問題1：修改一方會影響另一方
//   → 問題2：兩個物件解構時 double free → 未定義行為（crash）
//
// 深拷貝（Deep Copy）：分配新的記憶體 + 複製資料內容
//   → 兩個物件各自獨立，互不影響，各自安全解構
//
// 【何時需要深拷貝？】
//   類別內有指標成員指向動態分配的記憶體（new/malloc）
//   → 編譯器預設的拷貝建構/賦值只做淺拷貝（成員逐一複製）
//   → 必須自訂拷貝建構函數和拷貝賦值運算子來實現深拷貝
// ============================================================

#include <iostream>
#include <cstring>

// ============================================================
// 淺拷貝問題示範
// ============================================================
class ShallowString {
    char* m_data;
    std::size_t m_len;
public:
    ShallowString(const char* str = "") {
        m_len = std::strlen(str);
        m_data = new char[m_len + 1];
        std::strcpy(m_data, str);
        std::cout << "  [建構] 配置 @" << static_cast<void*>(m_data)
                  << " \"" << m_data << "\"\n";
    }

    ~ShallowString() {
        std::cout << "  [解構] 釋放 @" << static_cast<void*>(m_data)
                  << " \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    // ⚠️ 沒有自訂拷貝建構 → 編譯器生成淺拷貝版本：
    //   ShallowString(const ShallowString& other)
    //       : m_data(other.m_data), m_len(other.m_len) {}
    //   ↑ 只複製指標值，不配置新記憶體！

    const char* c_str() const { return m_data; }
    void setFirstChar(char ch) { if (m_len > 0) m_data[0] = ch; }
};

// ============================================================
// 深拷貝解決方案
// ============================================================
class DeepString {
    char* m_data;
    std::size_t m_len;
public:
    // 一般建構函數
    DeepString(const char* str = "") {
        m_len = std::strlen(str);
        m_data = new char[m_len + 1];
        std::strcpy(m_data, str);
        std::cout << "  [建構] @" << static_cast<void*>(m_data)
                  << " \"" << m_data << "\"\n";
    }

    // ★ 拷貝建構函數（深拷貝）
    DeepString(const DeepString& other) {
        m_len = other.m_len;
        m_data = new char[m_len + 1];          // 配置新的記憶體
        std::strcpy(m_data, other.m_data);     // 複製資料內容
        std::cout << "  [深拷貝建構] 新 @" << static_cast<void*>(m_data)
                  << " 從 @" << static_cast<const void*>(other.m_data)
                  << " \"" << m_data << "\"\n";
    }

    // ★ 拷貝賦值運算子（深拷貝）
    DeepString& operator=(const DeepString& other) {
        std::cout << "  [深拷貝賦值]\n";
        if (this == &other) {                  // ① 自我賦值檢查
            std::cout << "    自我賦值，跳過\n";
            return *this;
        }
        delete[] m_data;                       // ② 釋放舊資源
        m_len = other.m_len;                   // ③ 複製大小
        m_data = new char[m_len + 1];          // ④ 配置新記憶體
        std::strcpy(m_data, other.m_data);     // ⑤ 複製資料
        return *this;                          // ⑥ 回傳 *this（支援鏈式賦值）
    }

    // 解構函數
    ~DeepString() {
        std::cout << "  [解構] @" << static_cast<void*>(m_data)
                  << " \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
    void setFirstChar(char ch) { if (m_len > 0) m_data[0] = ch; }
};

int main() {
    // ============================================================
    // 示範 1：淺拷貝的兩大問題
    // ============================================================
    std::cout << "=== 淺拷貝問題（概念說明，不實際執行避免 crash）===\n";
    std::cout << "  ShallowString a(\"hello\");\n";
    std::cout << "  ShallowString b = a;  // 淺拷貝\n";
    std::cout << "  問題 1：b.setFirstChar('X') → a 也被改了（共享記憶體）\n";
    std::cout << "  問題 2：a 解構 delete → b 解構 delete 同一塊 → 💥 double free\n\n";

    // ============================================================
    // 示範 2：深拷貝 — 拷貝建構
    // ============================================================
    std::cout << "=== 深拷貝：拷貝建構 ===\n";
    {
        DeepString s1("Dragon");
        DeepString s2 = s1;  // 呼叫拷貝建構函數（深拷貝）

        std::cout << "  s1 @" << static_cast<const void*>(s1.c_str()) << "\n";
        std::cout << "  s2 @" << static_cast<const void*>(s2.c_str()) << "\n";
        std::cout << "  （位址不同 → 各自獨立！）\n";

        // 修改 s2 不影響 s1
        s2.setFirstChar('X');
        std::cout << "  修改 s2 後：s1=\"" << s1.c_str()
                  << "\"  s2=\"" << s2.c_str() << "\"\n";
        std::cout << "  （s1 不受影響 ✅）\n";
    }
    std::cout << "  （各自安全解構 ✅）\n\n";

    // ============================================================
    // 示範 3：深拷貝 — 拷貝賦值
    // ============================================================
    std::cout << "=== 深拷貝：拷貝賦值 ===\n";
    {
        DeepString s1("Knight");
        DeepString s2("Wizard");
        std::cout << "  賦值前：s1=\"" << s1.c_str() << "\" s2=\"" << s2.c_str() << "\"\n";
        s2 = s1;  // 呼叫拷貝賦值運算子
        std::cout << "  賦值後：s1=\"" << s1.c_str() << "\" s2=\"" << s2.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 示範 4：自我賦值安全性
    // ============================================================
    std::cout << "=== 自我賦值安全性 ===\n";
    {
        DeepString s1("Phoenix");
        s1 = s1;  // 自我賦值 → if (this == &other) 攔截
        std::cout << "  s1=\"" << s1.c_str() << "\"（安全！）\n";
    }
    std::cout << "\n";

    // ============================================================
    // 示範 5：鏈式賦值
    // ============================================================
    std::cout << "=== 鏈式賦值 ===\n";
    {
        DeepString a("A"), b("B"), c("C");
        c = b = a;  // b=a 回傳 b 的引用 → c=b
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str()
                  << "\" c=\"" << c.c_str() << "\"\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  1. 類別有指標成員 → 編譯器預設淺拷貝 → 危險！\n";
    std::cout << "  2. 深拷貝：new 新記憶體 + 複製資料內容\n";
    std::cout << "  3. 需要自訂：拷貝建構函數 + 拷貝賦值運算子（+解構函數）\n";
    std::cout << "  4. 拷貝賦值要注意：自我賦值檢查 + 先釋放舊資源\n";
    std::cout << "  5. 回傳 *this 支援鏈式賦值 a = b = c\n";

    return 0;
}
