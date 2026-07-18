// ============================================================
// 第 32 課 總結：移動建構函數（Move Constructor）
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【簽名】ClassName(ClassName&& other) noexcept
//
// 【移動三步驟】
//   1. 偷取（steal）：m_data = other.m_data;      把指標複製過來
//   2. 歸零（nullify）：other.m_data = nullptr;    源物件指標置空
//   3. 複製基本型別：m_len = other.m_len;          int/size_t 直接複製
//
// 移動後的 other 處於「有效但未指定」狀態
//   → other 可以安全解構（delete nullptr 是安全的）
//   → other 可以被重新賦值
//
// 【noexcept 很重要！】
//   vector 擴容時：
//     有 noexcept → 使用移動（快）
//     無 noexcept → 退回使用拷貝（慢）
//   原因：vector 需要強異常安全保證，移動可能拋異常時只能用拷貝
//
// 【自動生成規則】
//   如果定義了以下任何一個 → 編譯器不自動生成移動建構/移動賦值：
//     - 自訂解構函數
//     - 自訂拷貝建構函數
//     - 自訂拷貝賦值運算子
//   → 這就是為什麼需要 Rule of Five
//
// 【拷貝 vs 移動】
//   拷貝：配置新記憶體 + 複製資料 → O(n) 昂貴
//   移動：偷指標 + 置空            → O(1) 便宜
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>
#include <vector>
#include <string>
#include <type_traits>
#include <chrono>

// ============================================================
// SimpleString：移動建構函數完整示範
// ============================================================
class SimpleString {
    char* m_data;
    std::size_t m_len;
public:
    // 一般建構
    SimpleString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    // 拷貝建構（深拷貝）— 昂貴 💰
    SimpleString(const SimpleString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構💰] \"" << m_data << "\"\n";
    }

    // ★ 移動建構 — 便宜 ⚡
    SimpleString(SimpleString&& other) noexcept   // ← noexcept！
        : m_data(other.m_data)                     // ① 偷取指標
        , m_len(other.m_len)                       // ③ 複製基本型別
    {
        other.m_data = nullptr;                    // ② 歸零源物件
        other.m_len = 0;
        std::cout << "  [移動建構⚡] \"" << m_data << "\"\n";
    }

    // 賦值（Copy-and-Swap）
    SimpleString& operator=(SimpleString other) {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
        return *this;
    }

    // 解構
    ~SimpleString() {
        if (m_data) std::cout << "  [解構] \"" << m_data << "\"\n";
        else        std::cout << "  [解構] (nullptr，已被移動)\n";
        delete[] m_data;  // delete nullptr 安全
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
    bool empty() const { return !m_data || m_len == 0; }
};

// ============================================================
// noexcept 對 vector 的影響
// ============================================================
class Widget {
    char* m_name;
public:
    Widget(const char* name) : m_name(new char[std::strlen(name) + 1]) {
        std::strcpy(m_name, name);
    }
    Widget(const Widget& other) : m_name(new char[std::strlen(other.m_name) + 1]) {
        std::strcpy(m_name, other.m_name);
        std::cout << "    [Widget 拷貝] \"" << m_name << "\"\n";
    }
    Widget(Widget&& other) noexcept : m_name(other.m_name) {
        other.m_name = nullptr;
        std::cout << "    [Widget 移動] \"" << m_name << "\"\n";
    }
    Widget& operator=(Widget other) { std::swap(m_name, other.m_name); return *this; }
    ~Widget() { delete[] m_name; }
};

// ============================================================
// 自動生成規則示範
// ============================================================
class AutoMove {
    std::string m_name;
public:
    AutoMove(std::string name) : m_name(std::move(name)) {}
    // 什麼都不寫 → 編譯器自動生成移動建構 ✅
};

class NoAutoMove {
    std::string m_name;
public:
    NoAutoMove(std::string name) : m_name(std::move(name)) {}
    ~NoAutoMove() {}  // 自訂解構 → 移動建構不自動生成 ❌
    // 仍然「可移動建構」但退回使用拷貝建構（const T&）
};

int main() {
    // ============================================================
    // 1. 基本移動建構
    // ============================================================
    std::cout << "===== 1. 基本移動建構 =====\n";
    {
        SimpleString a("Dragon Sword");
        std::cout << "  拷貝建構（左值）：\n";
        SimpleString b = a;           // a 是左值 → 拷貝建構

        std::cout << "  移動建構（std::move）：\n";
        SimpleString c = std::move(a); // 右值 → 移動建構
        std::cout << "  a = \"" << a.c_str() << "\" (已被移動)\n";
        std::cout << "  c = \"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 2. 從臨時物件移動
    // ============================================================
    std::cout << "===== 2. 從臨時物件移動 =====\n";
    {
        SimpleString a = SimpleString("Phoenix Staff");
        std::cout << "  a = \"" << a.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 3. 移動後可以重新賦值
    // ============================================================
    std::cout << "===== 3. 移動後重新賦值 =====\n";
    {
        SimpleString a("First");
        SimpleString b = std::move(a);  // a 被掏空
        std::cout << "  a = \"" << a.c_str() << "\" (已被移動)\n";
        a = SimpleString("Reborn");     // 重新賦值
        std::cout << "  a = \"" << a.c_str() << "\" (重生了)\n";
    }
    std::cout << "\n";

    // ============================================================
    // 4. noexcept 對 vector 擴容的影響
    // ============================================================
    std::cout << "===== 4. noexcept 對 vector 的影響 =====\n";
    {
        std::vector<Widget> vec;
        vec.reserve(2);  // 預留 2 個空間

        std::cout << "  push_back #1:\n";
        vec.push_back(Widget("Alpha"));

        std::cout << "  push_back #2:\n";
        vec.push_back(Widget("Beta"));

        std::cout << "  push_back #3（觸發擴容，搬移既有元素）:\n";
        vec.push_back(Widget("Gamma"));
        // 因為 Widget 的移動建構有 noexcept → 用移動搬 Alpha 和 Beta
        // 若沒有 noexcept → 會用拷貝（慢）
    }
    std::cout << "\n";

    // ============================================================
    // 5. 自動生成規則
    // ============================================================
    std::cout << "===== 5. 自動生成規則 =====\n";
    std::cout << std::boolalpha;

    std::cout << "  AutoMove（什麼都沒寫）:\n";
    std::cout << "    可移動建構？ " << std::is_move_constructible_v<AutoMove> << "\n";
    std::cout << "    nothrow？   " << std::is_nothrow_move_constructible_v<AutoMove> << "\n";

    std::cout << "  NoAutoMove（自訂了解構函數）:\n";
    std::cout << "    可移動建構？ " << std::is_move_constructible_v<NoAutoMove> << "\n";
    std::cout << "    nothrow？   " << std::is_nothrow_move_constructible_v<NoAutoMove> << "\n";
    // NoAutoMove 仍「可移動建構」但退回拷貝，沒有真正的移動建構
    std::cout << "\n";

    // ============================================================
    // 6. 效能比較
    // ============================================================
    std::cout << "===== 6. 效能比較 =====\n";
    {
        const int N = 200000;
        std::vector<std::string> source;
        source.reserve(N);
        for (int i = 0; i < N; ++i)
            source.push_back(std::string(500, 'A' + (i % 26)));

        // 拷貝
        auto t1 = std::chrono::high_resolution_clock::now();
        std::vector<std::string> copied;
        copied.reserve(N);
        for (const auto& s : source) copied.push_back(s);
        auto t2 = std::chrono::high_resolution_clock::now();

        // 移動
        auto t3 = std::chrono::high_resolution_clock::now();
        std::vector<std::string> moved;
        moved.reserve(N);
        for (auto& s : source) moved.push_back(std::move(s));
        auto t4 = std::chrono::high_resolution_clock::now();

        auto copy_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        auto move_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
        std::cout << "  拷貝 " << N << " 個 500 字元字串：" << copy_ms << " ms\n";
        std::cout << "  移動 " << N << " 個 500 字元字串：" << move_ms << " ms\n";
        if (move_ms > 0)
            std::cout << "  加速比：約 " << copy_ms / move_ms << " 倍\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  移動三步驟：偷指標 → 歸零源物件 → 複製基本型別\n";
    std::cout << "  noexcept 讓 vector 擴容時用移動而非拷貝\n";
    std::cout << "  自訂解構/拷貝 → 移動不自動生成 → 需要 Rule of Five\n";
    std::cout << "  移動是 O(1)，拷貝是 O(n)，差異巨大\n";

    return 0;
}
