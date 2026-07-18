// ============================================================
// 第 29 課 總結：拷貝賦值運算子（Copy Assignment Operator）
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【簽名】ClassName& operator=(const ClassName& other)
//
// 【拷貝建構 vs 拷貝賦值】
//   MyClass b = a;   → 拷貝「建構」（b 尚未存在，初始化時呼叫）
//   b = a;           → 拷貝「賦值」（b 已存在，賦值時呼叫）
//
// 【傳統寫法步驟】
//   1. 自我賦值檢查：if (this == &other) return *this;
//   2. 釋放舊資源：  delete[] m_data;
//   3. 配置新記憶體
//   4. 複製資料
//   5. 回傳 *this（支援鏈式賦值 a = b = c）
//
// 【Copy-and-Swap 慣用法（推薦）】
//   ClassName& operator=(ClassName other) {  // 傳值！
//       swap(*this, other);
//       return *this;
//   }
//   優點：異常安全 + 自動處理自我賦值 + 程式碼簡潔
//
// 【隱式刪除的情況】
//   類別含 const 成員 → 不能重新賦值 → 編譯器不生成 operator=
//   類別含引用成員   → 不能重新綁定 → 編譯器不生成 operator=
//   解法：改用指標/去 const，或用 placement new（進階）
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>   // std::swap
#include <algorithm> // std::copy
#include <string>
#include <new>       // placement new

// ============================================================
// 方法一：傳統拷貝賦值寫法
// ============================================================
class TraditionalString {
    char* m_data;
    std::size_t m_len;
public:
    TraditionalString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    TraditionalString(const TraditionalString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // ★ 傳統拷貝賦值運算子
    TraditionalString& operator=(const TraditionalString& other) {
        std::cout << "  [傳統拷貝賦值] ";
        if (this == &other) {                       // ① 自我賦值檢查
            std::cout << "自我賦值，跳過\n";
            return *this;
        }
        delete[] m_data;                            // ② 釋放舊資源
        m_len = other.m_len;                        // ③ 複製大小
        m_data = new char[m_len + 1];               // ④ 配置新記憶體
        std::strcpy(m_data, other.m_data);          // ⑤ 複製資料
        std::cout << "\"" << m_data << "\"\n";
        return *this;                               // ⑥ 回傳 *this
    }

    ~TraditionalString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
};

// ============================================================
// 方法二：Copy-and-Swap 慣用法（推薦）
// ============================================================
class SwapString {
    char* m_data;
    std::size_t m_len;
public:
    SwapString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    SwapString(const SwapString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // swap 函數
    void swap(SwapString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ★ Copy-and-Swap：參數按值傳入（自動觸發拷貝建構）
    SwapString& operator=(SwapString other) {  // ← 注意：傳值，不是 const&
        std::cout << "  [Copy-and-Swap 賦值]\n";
        swap(other);        // 交換 this 和 other 的內容
        return *this;       // other 帶著舊資料離開作用域，自動解構
    }
    // 優點：
    //   1. 異常安全：如果 new 在拷貝建構階段失敗，原物件不受影響
    //   2. 自動處理自我賦值：a = a 時，other 是 a 的拷貝，交換後仍正確
    //   3. 程式碼簡潔：不需要手動 delete/new

    ~SwapString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
};

// ============================================================
// 隱式刪除示範：含 const 成員和引用成員
// ============================================================
class Config {
    const int m_id;           // const 成員 → operator= 被隱式刪除
    std::string& m_nameRef;   // 引用成員 → operator= 被隱式刪除
public:
    Config(int id, std::string& name) : m_id(id), m_nameRef(name) {}

    // ★ 解法：使用 placement new（進階技巧）
    // 原理：既然「賦值」行不通，改用「銷毀 + 重建」
    Config& operator=(const Config& other) {
        if (this != &other) {
            this->~Config();              // 手動銷毀當前物件
            new (this) Config(other);     // 在同一塊記憶體上重新建構
        }
        return *this;
    }
    // ⚠️ placement new 風險：若建構拋例外，物件處於已銷毀但未重建狀態
    // 一般建議：優先考慮將 const 改為 non-const + getter，引用改為指標

    void print() const {
        std::cout << "  Config{ id=" << m_id << ", name=\"" << m_nameRef << "\" }\n";
    }
};

// ============================================================
// DynamicBuffer：完整的拷貝賦值示範
// ============================================================
class DynamicBuffer {
    unsigned char* m_buffer;
    std::size_t m_capacity;
public:
    explicit DynamicBuffer(std::size_t cap)
        : m_capacity(cap), m_buffer(new unsigned char[cap]{}) {}

    DynamicBuffer(const DynamicBuffer& other)
        : m_capacity(other.m_capacity), m_buffer(new unsigned char[other.m_capacity]) {
        std::copy(other.m_buffer, other.m_buffer + m_capacity, m_buffer);
    }

    void swap(DynamicBuffer& other) noexcept {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_capacity, other.m_capacity);
    }

    DynamicBuffer& operator=(DynamicBuffer other) {  // Copy-and-Swap
        swap(other);
        return *this;
    }

    ~DynamicBuffer() { delete[] m_buffer; }

    unsigned char& operator[](std::size_t i) { return m_buffer[i]; }
    std::size_t capacity() const { return m_capacity; }
};

int main() {
    // ============================================================
    // 示範 1：傳統拷貝賦值
    // ============================================================
    std::cout << "=== 傳統拷貝賦值 ===\n";
    {
        TraditionalString a("Dragon");
        TraditionalString b("Knight");
        std::cout << "  b = a:\n";
        b = a;
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        std::cout << "  自我賦值：\n";
        a = a;  // 安全
    }
    std::cout << "\n";

    // ============================================================
    // 示範 2：Copy-and-Swap 慣用法
    // ============================================================
    std::cout << "=== Copy-and-Swap 賦值 ===\n";
    {
        SwapString a("Wizard");
        SwapString b("Archer");
        std::cout << "  b = a:\n";
        b = a;
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        // 用暫時物件賦值
        std::cout << "  b = SwapString(\"Phoenix\"):\n";
        b = SwapString("Phoenix");
        std::cout << "  b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 示範 3：鏈式賦值
    // ============================================================
    std::cout << "=== 鏈式賦值 ===\n";
    {
        SwapString a("A"), b("B"), c("C");
        std::cout << "  c = b = a:\n";
        c = b = a;
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str()
                  << "\" c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 示範 4：隱式刪除 + placement new 解法
    // ============================================================
    std::cout << "=== 隱式刪除（const/引用成員）===\n";
    {
        std::string name1 = "Alice", name2 = "Bob";
        Config c1(1, name1);
        Config c2(2, name2);
        std::cout << "  賦值前:\n";
        c1.print(); c2.print();

        c1 = c2;  // 使用 placement new 實現
        std::cout << "  賦值後 (c1 = c2):\n";
        c1.print(); c2.print();
    }
    std::cout << "\n";

    // ============================================================
    // 重點整理
    // ============================================================
    std::cout << "=== 重點整理 ===\n";
    std::cout << "  1. 拷貝建構：物件不存在時初始化（MyClass b = a）\n";
    std::cout << "  2. 拷貝賦值：物件已存在時賦值（b = a）\n";
    std::cout << "  3. 傳統寫法：自我檢查 → delete → new → copy → return *this\n";
    std::cout << "  4. Copy-and-Swap（推薦）：異常安全、自動處理自我賦值\n";
    std::cout << "  5. const/引用成員 → operator= 被隱式刪除\n";
    std::cout << "     解法：去 const + 改指標，或 placement new\n";

    return 0;
}
