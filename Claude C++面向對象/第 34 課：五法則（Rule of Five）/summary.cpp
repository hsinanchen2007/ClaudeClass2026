// ============================================================
// 第 34 課 總結：五法則（Rule of Five）
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【五法則 = 三法則 + 移動建構 + 移動賦值】
//   1. 解構函數           ~ClassName()
//   2. 拷貝建構函數       ClassName(const ClassName&)
//   3. 拷貝賦值運算子     ClassName& operator=(const ClassName&)
//   4. 移動建構函數       ClassName(ClassName&&) noexcept
//   5. 移動賦值運算子     ClassName& operator=(ClassName&&) noexcept
//
// 若使用統一賦值（pass-by-value + swap），只需寫 4 個：
//   解構 + 拷貝建構 + 移動建構 + 統一賦值
//
// 【Rule of Zero】
//   如果類別只用 RAII 成員（string, vector, unique_ptr...）
//   → 不需要自訂任何一個，全部用編譯器預設的
//   → 這是最推薦的做法！
//
// 【type_traits 檢查】
//   is_copy_constructible_v<T>        是否可拷貝建構
//   is_move_constructible_v<T>        是否可移動建構
//   is_nothrow_move_constructible_v<T> 移動建構是否 noexcept
//   is_copy_assignable_v<T>           是否可拷貝賦值
//   is_move_assignable_v<T>           是否可移動賦值
// ============================================================

#include <iostream>
#include <algorithm>  // std::copy
#include <utility>    // std::swap, std::move
#include <string>
#include <vector>
#include <memory>     // std::unique_ptr, std::make_unique
#include <type_traits>

// ============================================================
// 完整的 Rule of Five 類別：ManagedArray
// ============================================================
class ManagedArray {
    int* m_data;
    std::size_t m_size;

public:
    // 建構函數
    explicit ManagedArray(std::size_t size = 0)
        : m_size(size), m_data(size > 0 ? new int[size]{} : nullptr) {
        std::cout << "  [建構] size=" << m_size << "\n";
    }

    // ═══════════════════════════════════════════
    // ★ Rule of Five（使用統一賦值，實際只寫 4 個）★
    // ═══════════════════════════════════════════

    // ① 解構函數
    ~ManagedArray() {
        std::cout << "  [解構] size=" << m_size << "\n";
        delete[] m_data;
    }

    // ② 拷貝建構函數（深拷貝）
    ManagedArray(const ManagedArray& other)
        : m_size(other.m_size),
          m_data(other.m_size > 0 ? new int[other.m_size] : nullptr) {
        if (m_data) std::copy(other.m_data, other.m_data + m_size, m_data);
        std::cout << "  [拷貝建構] size=" << m_size << "\n";
    }

    // ③ 移動建構函數（偷資源）
    ManagedArray(ManagedArray&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size) {
        other.m_data = nullptr;
        other.m_size = 0;
        std::cout << "  [移動建構] size=" << m_size << "\n";
    }

    // ④⑤ 統一賦值運算子（同時處理拷貝和移動）
    void swap(ManagedArray& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
    }

    ManagedArray& operator=(ManagedArray other) {  // 傳值
        std::cout << "  [統一賦值] swap\n";
        swap(other);
        return *this;
    }

    // ═══════════════════════════════════════════

    int& operator[](std::size_t i) { return m_data[i]; }
    const int& operator[](std::size_t i) const { return m_data[i]; }
    std::size_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }

    void print(const char* label) const {
        std::cout << "  " << label << " (size=" << m_size << "): [";
        for (std::size_t i = 0; i < m_size; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << m_data[i];
        }
        std::cout << "]\n";
    }
};

// ============================================================
// Rule of Five（分開寫 5 個）— 對照版本
// ============================================================
class FullFive {
    int* m_data;
public:
    FullFive() : m_data(new int(0)) {}

    ~FullFive() { delete m_data; }                                        // ①

    FullFive(const FullFive& o) : m_data(new int(*o.m_data)) {}          // ②

    FullFive& operator=(const FullFive& o) {                              // ③
        if (this != &o) *m_data = *o.m_data;
        return *this;
    }

    FullFive(FullFive&& o) noexcept : m_data(o.m_data) {                 // ④
        o.m_data = nullptr;
    }

    FullFive& operator=(FullFive&& o) noexcept {                          // ⑤
        if (this != &o) { delete m_data; m_data = o.m_data; o.m_data = nullptr; }
        return *this;
    }
};

// ============================================================
// Rule of Zero 示範
// ============================================================
struct SafeClass {
    std::string name;       // RAII 成員
    std::vector<int> data;  // RAII 成員
    // 不需要寫任何解構/拷貝/移動！編譯器預設的就是正確的
};

// ============================================================
// 只能移動的類別（MoveOnly）
// ============================================================
class MoveOnly {
    std::unique_ptr<int> m_data;
public:
    MoveOnly() : m_data(std::make_unique<int>(0)) {}
    // unique_ptr 讓拷貝被隱式刪除，移動自動生成
};

// ============================================================
// type_traits 檢查
// ============================================================
template <typename T>
void checkTraits(const char* name) {
    std::cout << "  " << name << ":\n";
    std::cout << "    可解構？           " << std::is_destructible_v<T> << "\n";
    std::cout << "    可拷貝建構？       " << std::is_copy_constructible_v<T> << "\n";
    std::cout << "    可拷貝賦值？       " << std::is_copy_assignable_v<T> << "\n";
    std::cout << "    可移動建構？       " << std::is_move_constructible_v<T> << "\n";
    std::cout << "    nothrow 移動建構？ " << std::is_nothrow_move_constructible_v<T> << "\n";
    std::cout << "    可移動賦值？       " << std::is_move_assignable_v<T> << "\n";
    std::cout << "    nothrow 移動賦值？ " << std::is_nothrow_move_assignable_v<T> << "\n\n";
}

// 按值回傳 → 移動或 NRVO
ManagedArray createArray(std::size_t n) {
    ManagedArray arr(n);
    for (std::size_t i = 0; i < n; ++i) arr[i] = static_cast<int>((i + 1) * 100);
    return arr;
}

// 按值傳入 → 拷貝（左值）或移動（右值）
void consumeArray(ManagedArray arr) {
    arr.print("consumed");
}

int main() {
    // ============================================================
    // 1. 完整 Rule of Five 使用
    // ============================================================
    std::cout << "===== 1. Rule of Five 完整使用 =====\n";
    {
        ManagedArray a(4);
        for (std::size_t i = 0; i < a.size(); ++i) a[i] = static_cast<int>(i + 1);
        a.print("a");

        std::cout << "\n  拷貝建構：\n";
        ManagedArray b = a;
        b.print("b");

        std::cout << "\n  移動建構：\n";
        ManagedArray c = std::move(a);
        c.print("c");
        a.print("a (moved)");

        std::cout << "\n  拷貝賦值（統一賦值，左值路徑）：\n";
        ManagedArray d(2);
        d = b;
        d.print("d");

        std::cout << "\n  移動賦值（統一賦值，右值路徑）：\n";
        ManagedArray e(1);
        e = std::move(c);
        e.print("e");
        c.print("c (moved)");
    }
    std::cout << "\n";

    // ============================================================
    // 2. 從函數回傳 / 傳入函數
    // ============================================================
    std::cout << "===== 2. 函數回傳與傳入 =====\n";
    {
        ManagedArray f = createArray(3);
        f.print("f (from function)");

        std::cout << "  傳左值（拷貝）：\n";
        consumeArray(f);

        std::cout << "  傳右值（移動）：\n";
        consumeArray(std::move(f));
        f.print("f (moved)");
    }
    std::cout << "\n";

    // ============================================================
    // 3. Rule of Zero
    // ============================================================
    std::cout << "===== 3. Rule of Zero =====\n";
    {
        SafeClass s1{"Hero", {1, 2, 3}};
        SafeClass s2 = s1;              // 安全拷貝
        SafeClass s3 = std::move(s1);   // 安全移動
        std::cout << "  s2.name=" << s2.name << " s3.name=" << s3.name << "\n";
        std::cout << "  s1.name=\"" << s1.name << "\" (已被移動)\n";
        std::cout << "  只用 RAII 成員 → 不需要寫任何特殊函數！\n";
    }
    std::cout << "\n";

    // ============================================================
    // 4. type_traits 檢查
    // ============================================================
    std::cout << "===== 4. type_traits 檢查 =====\n";
    std::cout << std::boolalpha;
    checkTraits<ManagedArray>("ManagedArray (Rule of Five)");
    checkTraits<MoveOnly>("MoveOnly (只能移動)");
    checkTraits<SafeClass>("SafeClass (Rule of Zero)");
    checkTraits<std::string>("std::string (標準庫)");

    // ============================================================
    // 重點整理
    // ============================================================
    std::cout << "=== 重點整理 ===\n";
    std::cout << "  Rule of Five：解構 + 拷貝建構 + 拷貝賦值 + 移動建構 + 移動賦值\n";
    std::cout << "  統一賦值（傳值 + swap）可以把拷貝賦值和移動賦值合為一個\n";
    std::cout << "  Rule of Zero：只用 RAII 成員 → 什麼都不用寫（最佳）\n";
    std::cout << "  用 type_traits 可以在編譯期檢查類別的能力\n";

    return 0;
}
