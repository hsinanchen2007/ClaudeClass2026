// lesson34_rule_of_five.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson34 lesson34_rule_of_five.cpp
// 驗證：valgrind --leak-check=full ./lesson34

#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

class ManagedArray {
private:
    int*        m_data;
    std::size_t m_size;

public:
    // ════════════════════════════════════════════
    //  建構函數
    // ════════════════════════════════════════════
    explicit ManagedArray(std::size_t size = 0)
        : m_size(size)
        , m_data(size > 0 ? new int[size]{} : nullptr)
    {
        std::cout << "  [建構] size=" << m_size << "\n";
    }

    // ════════════════════════════════════════════
    //  Rule of Five：以下五個必須同時定義
    // ════════════════════════════════════════════

    // ── 1. 解構函數 ──
    ~ManagedArray() {
        std::cout << "  [解構] size=" << m_size << "\n";
        delete[] m_data;
    }

    // ── 2. 拷貝建構函數 ──
    ManagedArray(const ManagedArray& other)
        : m_size(other.m_size)
        , m_data(other.m_size > 0 ? new int[other.m_size] : nullptr)
    {
        if (m_data) {
            std::copy(other.m_data, other.m_data + m_size, m_data);
        }
        std::cout << "  [拷貝建構] size=" << m_size << "\n";
    }

    // ── 3. 拷貝賦值運算子 ──
    ManagedArray& operator=(const ManagedArray& other) {
        std::cout << "  [拷貝賦值] size=" << other.m_size << "\n";
        if (this == &other) return *this;

        // 先配置再釋放（例外安全）
        int* newData = nullptr;
        if (other.m_size > 0) {
            newData = new int[other.m_size];
            std::copy(other.m_data, other.m_data + other.m_size, newData);
        }

        delete[] m_data;
        m_data = newData;
        m_size = other.m_size;

        return *this;
    }

    // ── 4. 移動建構函數 ──
    ManagedArray(ManagedArray&& other) noexcept
        : m_data(other.m_data)
        , m_size(other.m_size)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        std::cout << "  [移動建構] size=" << m_size << "\n";
    }

    // ── 5. 移動賦值運算子 ──
    ManagedArray& operator=(ManagedArray&& other) noexcept {
        std::cout << "  [移動賦值] size=" << other.m_size << "\n";
        if (this == &other) return *this;

        delete[] m_data;

        m_data = other.m_data;
        m_size = other.m_size;

        other.m_data = nullptr;
        other.m_size = 0;

        return *this;
    }

    // ════════════════════════════════════════════
    //  輔助函數
    // ════════════════════════════════════════════

    void swap(ManagedArray& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
    }

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

// ──────── 測試用函數 ────────

// 按值返回 → 觸發移動（或拷貝省略）
ManagedArray createArray(std::size_t n) {
    ManagedArray arr(n);
    for (std::size_t i = 0; i < n; ++i) {
        arr[i] = static_cast<int>((i + 1) * 100);
    }
    return arr;
}

// 按值傳入 → 觸發拷貝建構（左值）或移動建構（右值）
void consumeArray(ManagedArray arr) {
    arr.print("consumed");
}

int main() {
    std::cout << "===== 1. 一般建構 =====\n";
    ManagedArray a(4);
    for (std::size_t i = 0; i < a.size(); ++i) a[i] = static_cast<int>(i + 1);
    a.print("a");
    std::cout << "\n";

    std::cout << "===== 2. 拷貝建構（左值）=====\n";
    ManagedArray b = a;
    b.print("b");
    std::cout << "\n";

    std::cout << "===== 3. 移動建構（右值）=====\n";
    ManagedArray c = std::move(a);
    c.print("c");
    a.print("a (moved)");
    std::cout << "\n";

    std::cout << "===== 4. 拷貝賦值（左值）=====\n";
    ManagedArray d(2);
    d[0] = 99; d[1] = 88;
    d.print("d before");
    d = b;  // b 是左值 → 拷貝賦值
    d.print("d after");
    std::cout << "\n";

    std::cout << "===== 5. 移動賦值（右值）=====\n";
    ManagedArray e(1);
    e[0] = 77;
    e.print("e before");
    e = std::move(c);  // std::move(c) 是右值 → 移動賦值
    e.print("e after");
    c.print("c (moved)");
    std::cout << "\n";

    std::cout << "===== 6. 從函數返回值（移動或拷貝省略）=====\n";
    ManagedArray f = createArray(3);
    f.print("f");
    std::cout << "\n";

    std::cout << "===== 7. 按值傳入左值（拷貝建構）=====\n";
    consumeArray(b);
    b.print("b (still alive)");
    std::cout << "\n";

    std::cout << "===== 8. 按值傳入右值（移動建構）=====\n";
    consumeArray(std::move(b));
    b.print("b (moved)");
    std::cout << "\n";

    std::cout << "===== 開始解構 =====\n";
    return 0;
}
